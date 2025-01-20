/*
 * Copyright (c) 2021-2022 Facebook, Inc. and its affiliates
 * Copyright (c) 2021-2024 NVIDIA Corporation
 *
 * Licensed under the Apache License Version 2.0 with LLVM Exceptions
 * (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 *
 *   https://llvm.org/LICENSE.txt
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <version>
#include <cstdint>
#include <utility>
#include <type_traits>
#include <atomic>
#include <thread>
#include <concepts>
#include <cassert>

#include <xmmintrin.h>
// NOLINTBEGIN
// NOTE: std::inplace_stop_token implementation taken from
// https://github.com/NVIDIA/stdexec
//
// Some minor modifications applied:
// - added template deduction guide to inplace_stop_callback
// - added optimisation to request_stop() to avoid retaking lock after calling
//   the last stop-callback
// - added _mm_pause() call inside the __spin_wait() implementation

namespace std
{
    struct inplace_stop_source;
    struct inplace_stop_token;

    namespace __stok
    {
        struct __inplace_stop_callback_base
        {
            void __execute() noexcept
            {
                this->__execute_(this);
            }

          protected:
            using __execute_fn_t = void(__inplace_stop_callback_base *) noexcept;

            explicit __inplace_stop_callback_base(   //
                const inplace_stop_source *__source, //
                __execute_fn_t *__execute) noexcept
                : __source_(__source), __execute_(__execute)
            {
            }

            void __register_callback_() noexcept;

            friend inplace_stop_source;

            const inplace_stop_source *__source_;
            __execute_fn_t *__execute_;
            __inplace_stop_callback_base *__next_ = nullptr;
            __inplace_stop_callback_base **__prev_ptr_ = nullptr;
            bool *__removed_during_callback_ = nullptr;
            std::atomic<bool> __callback_completed_{false};
        };

        struct __spin_wait
        {
            __spin_wait() noexcept = default;

            void __wait() noexcept
            {
                if (__count_++ < __yield_threshold_)
                {
                    for (std::uint32_t i = 0; i < __count_; ++i)
                        _mm_pause();
                }
                else
                {
                    if (__count_ == 0)
                        __count_ = __yield_threshold_;
                    std::this_thread::yield();
                }
            }

          private:
            static constexpr uint32_t __yield_threshold_ = 20;
            uint32_t __count_ = 0;
        };
    } // namespace __stok

    // [stoptoken.never], class never_stop_token
    struct never_stop_token
    {
      private:
        struct __callback_type
        {
            explicit __callback_type(never_stop_token, auto &&) noexcept {}
        };

      public:
        template <class>
        using callback_type = __callback_type;

        static constexpr auto stop_requested() noexcept -> bool
        {
            return false;
        }

        static constexpr auto stop_possible() noexcept -> bool
        {
            return false;
        }

        auto operator==(const never_stop_token &) const noexcept -> bool = default;
    };

    template <class _Callback>
    class inplace_stop_callback;

    // [stopsource.inplace], class inplace_stop_source
    class inplace_stop_source
    {
      public:
        inplace_stop_source() noexcept = default;
        ~inplace_stop_source();
        inplace_stop_source(inplace_stop_source &&) = delete;

        auto get_token() const noexcept -> inplace_stop_token;

        auto request_stop() noexcept -> bool;

        auto stop_requested() const noexcept -> bool
        {
            return (__state_.load(std::memory_order_acquire) & __stop_requested_flag_) !=
                   0;
        }

      private:
        friend inplace_stop_token;
        friend __stok::__inplace_stop_callback_base;
        template <class>
        friend class inplace_stop_callback;

        auto __lock_() const noexcept -> uint8_t;
        void __unlock_(uint8_t) const noexcept;

        auto __try_lock_unless_stop_requested_(bool) const noexcept -> bool;

        auto __try_add_callback_(__stok::__inplace_stop_callback_base *) const noexcept
            -> bool;

        void __remove_callback_(__stok::__inplace_stop_callback_base *) const noexcept;

        static constexpr uint8_t __stop_requested_flag_ = 1;
        static constexpr uint8_t __locked_flag_ = 2;

        mutable std::atomic<uint8_t> __state_{0};
        mutable __stok::__inplace_stop_callback_base *__callbacks_ = nullptr;
        std::thread::id __notifying_thread_;
    };

    // [stoptoken.inplace], class inplace_stop_token
    class inplace_stop_token
    {
      public:
        template <class _Fun>
        using callback_type = inplace_stop_callback<_Fun>;

        inplace_stop_token() noexcept : __source_(nullptr) {}

        inplace_stop_token(const inplace_stop_token &__other) noexcept = default;

        inplace_stop_token(inplace_stop_token &&__other) noexcept
            : __source_(std::exchange(__other.__source_, {}))
        {
        }

        auto operator=(const inplace_stop_token &__other) noexcept
            -> inplace_stop_token & = default;

        auto operator=(inplace_stop_token &&__other) noexcept -> inplace_stop_token &
        {
            __source_ = std::exchange(__other.__source_, nullptr);
            return *this;
        }

        [[nodiscard]] auto stop_requested() const noexcept -> bool
        {
            return __source_ != nullptr && __source_->stop_requested();
        }

        [[nodiscard]] auto stop_possible() const noexcept -> bool
        {
            return __source_ != nullptr;
        }

        void swap(inplace_stop_token &__other) noexcept
        {
            std::swap(__source_, __other.__source_);
        }

        auto operator==(const inplace_stop_token &) const noexcept -> bool = default;

      private:
        friend inplace_stop_source;
        template <class>
        friend class inplace_stop_callback;

        explicit inplace_stop_token(const inplace_stop_source *__source) noexcept
            : __source_(__source)
        {
        }

        const inplace_stop_source *__source_;
    };

    inline auto inplace_stop_source::get_token() const noexcept -> inplace_stop_token
    {
        return inplace_stop_token{this};
    }

    // [stopcallback.inplace], class template inplace_stop_callback
    template <class _Fun>
    class inplace_stop_callback : __stok::__inplace_stop_callback_base
    {
      public:
        template <class _Fun2>
            requires std::constructible_from<_Fun, _Fun2>
        explicit inplace_stop_callback(inplace_stop_token __token,
                                       _Fun2 &&__fun) //
            noexcept(std::is_nothrow_constructible_v<_Fun, _Fun2>)
            : __stok::__inplace_stop_callback_base(
                  __token.__source_, &inplace_stop_callback::__execute_impl_),
              __fun_(static_cast<_Fun2 &&>(__fun))
        {
            __register_callback_();
        }

        ~inplace_stop_callback()
        {
            if (__source_ != nullptr)
                __source_->__remove_callback_(this);
        }

      private:
        static void __execute_impl_(__stok::__inplace_stop_callback_base *cb) noexcept
        {
            std::move(static_cast<inplace_stop_callback *>(cb)->__fun_)();
        }

        [[no_unique_address]] _Fun __fun_;
    };

    namespace __stok
    {
        inline void __inplace_stop_callback_base::__register_callback_() noexcept
        {
            if (__source_ != nullptr)
            {
                if (!__source_->__try_add_callback_(this))
                {
                    __source_ = nullptr;
                    // Callback not registered because stop_requested() was true.
                    // Execute inline here.
                    __execute();
                }
            }
        }
    } // namespace __stok

    inline inplace_stop_source::~inplace_stop_source()
    {
        assert((__state_.load(std::memory_order_relaxed) & __locked_flag_) == 0);
        assert(__callbacks_ == nullptr);
    }

    inline auto inplace_stop_source::request_stop() noexcept -> bool
    {
        if (!__try_lock_unless_stop_requested_(true))
            return false;

        __notifying_thread_ = std::this_thread::get_id();

        // We are responsible for executing callbacks.
        while (__callbacks_ != nullptr)
        {
            auto *__callbk = __callbacks_;
            __callbk->__prev_ptr_ = nullptr;
            __callbacks_ = __callbk->__next_;
            if (__callbacks_ != nullptr)
                __callbacks_->__prev_ptr_ = &__callbacks_;

            bool __any_more_callbacks = (__callbacks_ != nullptr);

            __state_.store(__stop_requested_flag_, std::memory_order_release);

            bool __removed_during_callback = false;
            __callbk->__removed_during_callback_ = &__removed_during_callback;

            __callbk->__execute();

            if (!__removed_during_callback)
            {
                __callbk->__removed_during_callback_ = nullptr;
                __callbk->__callback_completed_.store(true, std::memory_order_release);
            }

            if (!__any_more_callbacks)
                return true;
            __lock_();
        }

        __state_.store(__stop_requested_flag_, std::memory_order_release);
        return true;
    }

    inline auto inplace_stop_source::__lock_() const noexcept -> uint8_t
    {
        __stok::__spin_wait __spin;
        auto __old_state = __state_.load(std::memory_order_relaxed);
        do
        {
            while ((__old_state & __locked_flag_) != 0)
            {
                __spin.__wait();
                __old_state = __state_.load(std::memory_order_relaxed);
            }
        } while (!__state_.compare_exchange_weak(
            __old_state, __old_state | __locked_flag_, std::memory_order_acquire,
            std::memory_order_relaxed));

        return __old_state;
    }

    inline void inplace_stop_source::__unlock_(uint8_t __old_state) const noexcept
    {
        (void)__state_.store(__old_state, std::memory_order_release);
    }

    inline auto inplace_stop_source::__try_lock_unless_stop_requested_(
        bool __set_stop_requested) const noexcept -> bool
    {
        __stok::__spin_wait __spin;
        auto __old_state = __state_.load(std::memory_order_relaxed);
        do
        {
            while (true)
            {
                if ((__old_state & __stop_requested_flag_) != 0)
                {
                    // Stop already requested.
                    return false;
                }
                else if (__old_state == 0)
                {
                    break;
                }
                else
                {
                    __spin.__wait();
                    __old_state = __state_.load(std::memory_order_relaxed);
                }
            }
        } while (!__state_.compare_exchange_weak(
            __old_state,
            __set_stop_requested ? (__locked_flag_ | __stop_requested_flag_)
                                 : __locked_flag_,
            std::memory_order_acq_rel, std::memory_order_relaxed));

        // Lock acquired successfully
        return true;
    }

    inline auto inplace_stop_source::__try_add_callback_(
        __stok::__inplace_stop_callback_base *__callbk) const noexcept -> bool
    {
        if (!__try_lock_unless_stop_requested_(false))
        {
            return false;
        }

        __callbk->__next_ = __callbacks_;
        __callbk->__prev_ptr_ = &__callbacks_;
        if (__callbacks_ != nullptr)
        {
            __callbacks_->__prev_ptr_ = &__callbk->__next_;
        }
        __callbacks_ = __callbk;

        __unlock_(0);

        return true;
    }

    inline void inplace_stop_source::__remove_callback_(
        __stok::__inplace_stop_callback_base *__callbk) const noexcept
    {
        auto __old_state = __lock_();

        if (__callbk->__prev_ptr_ != nullptr)
        {
            // Callback has not been executed yet.
            // Remove from the list.
            *__callbk->__prev_ptr_ = __callbk->__next_;
            if (__callbk->__next_ != nullptr)
            {
                __callbk->__next_->__prev_ptr_ = __callbk->__prev_ptr_;
            }
            __unlock_(__old_state);
        }
        else
        {
            auto __notifying_thread = __notifying_thread_;
            __unlock_(__old_state);

            // Callback has either already been executed or is
            // currently executing on another thread.
            if (std::this_thread::get_id() == __notifying_thread)
            {
                if (__callbk->__removed_during_callback_ != nullptr)
                {
                    *__callbk->__removed_during_callback_ = true;
                }
            }
            else
            {
                // Concurrently executing on another thread.
                // Wait until the other thread finishes executing the callback.
                __stok::__spin_wait __spin;
                while (!__callbk->__callback_completed_.load(std::memory_order_acquire))
                {
                    __spin.__wait();
                }
            }
        }
    }

    template <typename CB>
    inplace_stop_callback(inplace_stop_token, CB) -> inplace_stop_callback<CB>;

} // namespace std

namespace std
{
    template <std::size_t N, std::size_t Idx>
    class finite_inplace_stop_token;
    template <std::size_t N, std::size_t Idx, typename CB>
    class finite_inplace_stop_callback;

    template <typename CB>
    class finite_inplace_stop_callback_base;

    class finite_inplace_stop_source_base
    {
      protected:
        template <typename CB>
        friend class finite_inplace_stop_callback_base;

        struct callback_base
        {
            void (*execute)(callback_base *self) noexcept;
        };

        void *stop_requested_state(std::atomic<void *> *states) const noexcept
        {
            return states;
        }
        void *stop_requested_callback_done_state() const noexcept
        {
            return &thread_requesting_stop_;
        }
        static void *no_callback_state() noexcept
        {
            return nullptr;
        }

        bool is_stop_requested_state(std::atomic<void *> *states,
                                     void *state) const noexcept
        {
            bool result = (state == stop_requested_state(states));
            result |= (state == stop_requested_callback_done_state());
            return result;
        }

        bool try_register_callback(std::size_t count, atomic<void *> *states,
                                   std::size_t idx, callback_base *cb) const noexcept
        {
            auto &state = states[idx];
            void *old_state = state.load(memory_order_acquire);
            if (is_stop_requested_state(states, old_state))
            {
                return false;
            }

            assert(old_state == no_callback_state());

            if (state.compare_exchange_strong(old_state, static_cast<void *>(cb),
                                              memory_order_release, memory_order_acquire))
            {
                // Successfully registered callback.
                return true;
            }

            // Stop request arrived while we were trying to register
            assert(old_state == stop_requested_state(states));

            return false;
        }

        void deregister_callback(std::size_t count, atomic<void *> *states,
                                 std::size_t idx, callback_base *cb) const noexcept
        {
            // Initially assume that the callback has not been invoked and that the state
            // still points to the registered callback_base structure.
            auto &state = states[idx];

            void *old_state = static_cast<void *>(cb);
            if (state.compare_exchange_strong(old_state, no_callback_state(),
                                              memory_order_relaxed, memory_order_acquire))
            {
                // Successfully deregistered the callback before it could be invoked.
                return;
            }

            // Otherwise, a call to request_stop() is invoking the callback.
            if (old_state == stop_requested_state(states))
            {
                // Callback not finished executing yet.
                if (thread_requesting_stop_.load(std::memory_order_relaxed) ==
                    std::this_thread::get_id())
                {
                    // Deregistering from the same thread that is invoking the callback.
                    // Either the invocation of the callback has completed and the thread
                    // has gone on to do other things (in which case it's safe to destroy)
                    // or we are still in the middle of executing the callback (in which
                    // case we can't block as it would cause a deadlock).
                    return;
                }

                // Otherwise, callback is being called from another thread.
                // Wait for callback to finish (state changes from stop_requested_state()
                // to stop_requested_callback_done_state()).
                state.wait(old_state, memory_order_acquire);
            }
        }

        bool request_stop_impl(std::size_t count, atomic<void *> *states) noexcept
        {
            assert(count >= 1);
            auto &first_state = states[0];
            void *old_state = first_state.load(std::memory_order_relaxed);
            do
            {
                if (is_stop_requested_state(states, old_state))
                {
                    return false;
                }
            } while (!first_state.compare_exchange_weak(
                old_state, stop_requested_state(states), memory_order_acq_rel,
                memory_order_relaxed));

            thread_requesting_stop_.store(this_thread::get_id(), memory_order_relaxed);

            if (old_state != no_callback_state())
            {
                auto *callback = static_cast<callback_base *>(old_state);
                callback->execute(callback);
                first_state.store(stop_requested_callback_done_state(),
                                  memory_order_release);
                first_state.notify_one();
            }

            for (std::size_t i = 1; i < count; ++i)
            {
                old_state = states[i].exchange(stop_requested_state(states),
                                               memory_order_acq_rel);
                assert(!is_stop_requested_state(states, old_state));
                if (old_state != no_callback_state())
                {
                    auto *callback = static_cast<callback_base *>(old_state);
                    callback->execute(callback);
                    states[i].store(stop_requested_callback_done_state(),
                                    std::memory_order_release);
                    states[i].notify_one();
                }
            }

            return true;
        }

        mutable atomic<thread::id> thread_requesting_stop_;
    };

    template <typename CB>
    struct finite_inplace_stop_callback_base
        : protected finite_inplace_stop_source_base::callback_base
    {
      protected:
        template <typename Initializer>
        finite_inplace_stop_callback_base(Initializer &&init) noexcept(
            std::is_nothrow_constructible_v<CB, Initializer>)
            : callback_(std::forward<Initializer>(init))
        {
            this->execute = &execute_impl;
        }

      private:
        static void execute_impl(
            finite_inplace_stop_source_base::callback_base *base) noexcept
        {
            auto &self = *static_cast<finite_inplace_stop_callback_base *>(base);
            self.callback_();
        }

        [[no_unique_address]] CB callback_;
    };

    template <std::size_t N>
    class finite_inplace_stop_source : private finite_inplace_stop_source_base
    {
        static_assert(N > 0);

        template <std::size_t... Indices>
        explicit finite_inplace_stop_source(std::index_sequence<Indices...>) noexcept
            : states_{((void)Indices, no_callback_state())...}
        {
        }

      public:
        finite_inplace_stop_source() noexcept
            : finite_inplace_stop_source(std::make_index_sequence<N>{})
        {
        }

        bool request_stop() noexcept;
        bool stop_requested() const noexcept;

        template <std::size_t Idx>
        finite_inplace_stop_token<N, Idx> get_token() const noexcept;

      private:
        template <std::size_t, std::size_t, typename>
        friend class finite_inplace_stop_callback;

        bool try_register_callback(std::size_t idx, callback_base *cb) const noexcept;
        void deregister_callback(std::size_t idx, callback_base *cb) const noexcept;

        // nullptr                 - no stop-request or stop-callback
        // &states_                - stop-requested
        // &thread_requesting_stop - stop-requested, callback-done
        // other                   - pointer to callback_base
        mutable std::array<atomic<void *>, N> states_;
    };

    template <std::size_t N>
    inline bool finite_inplace_stop_source<N>::stop_requested() const noexcept
    {
        void *state = states_[0].load(std::memory_order_acquire);
        return is_stop_requested_state(states_.data(), state);
    }

    template <std::size_t N, std::size_t Idx>
    class finite_inplace_stop_token
    {
      public:
        template <typename CB>
        using callback_type = finite_inplace_stop_callback<N, Idx, CB>;

        finite_inplace_stop_token() noexcept : source_(nullptr) {}

        bool stop_possible() noexcept
        {
            return source_ != nullptr;
        }
        bool stop_requested() noexcept
        {
            return stop_possible() && source_->stop_requested();
        }

      private:
        friend finite_inplace_stop_source<N>;
        template <std::size_t, std::size_t, typename CB>
        friend class finite_inplace_stop_callback;

        explicit finite_inplace_stop_token(
            const finite_inplace_stop_source<N> *source) noexcept
            : source_(source)
        {
        }

        const finite_inplace_stop_source<N> *source_;
    };

    template <std::size_t N, std::size_t Idx, typename CB>
    class finite_inplace_stop_callback : private finite_inplace_stop_callback_base<CB>
    {
      public:
        template <typename Init>
            requires std::constructible_from<CB, Init>
        finite_inplace_stop_callback(
            finite_inplace_stop_token<N, Idx> st,
            Init &&init) noexcept(is_nothrow_constructible_v<CB, Init>)
            : finite_inplace_stop_callback_base<CB>(std::forward<Init>(init)),
              source_(st.source_)
        {
            if (source_ != nullptr)
            {
                if (!source_->try_register_callback(Idx, this))
                {
                    source_ = nullptr;
                    this->execute(this);
                }
            }
        }

        ~finite_inplace_stop_callback()
        {
            if (source_ != nullptr)
            {
                source_->deregister_callback(Idx, this);
            }
        }

        finite_inplace_stop_callback(finite_inplace_stop_callback &&) = delete;
        finite_inplace_stop_callback(const finite_inplace_stop_callback &) = delete;
        finite_inplace_stop_callback &operator=(finite_inplace_stop_callback &&) = delete;
        finite_inplace_stop_callback &operator=(const finite_inplace_stop_callback &) =
            delete;

      private:
        const finite_inplace_stop_source<N> *source_;
    };

    template <std::size_t N, std::size_t Idx, typename CB>
    finite_inplace_stop_callback(finite_inplace_stop_token<N, Idx>, CB)
        -> finite_inplace_stop_callback<N, Idx, CB>;

    template <std::size_t N>
    template <std::size_t Idx>
    inline finite_inplace_stop_token<N, Idx> finite_inplace_stop_source<N>::get_token()
        const noexcept
    {
        return finite_inplace_stop_token<N, Idx>{this};
    }

    template <std::size_t N>
    inline bool finite_inplace_stop_source<N>::request_stop() noexcept
    {
        return finite_inplace_stop_source_base::request_stop_impl(N, states_.data());
    }

    template <std::size_t N>
    inline bool finite_inplace_stop_source<N>::try_register_callback(
        std::size_t idx, callback_base *base) const noexcept
    {
        return finite_inplace_stop_source_base::try_register_callback(N, states_.data(),
                                                                      idx, base);
    }

    template <std::size_t N>
    inline void finite_inplace_stop_source<N>::deregister_callback(
        std::size_t idx, callback_base *base) const noexcept
    {
        finite_inplace_stop_source_base::deregister_callback(N, states_.data(), idx,
                                                             base);
    }

    using single_inplace_stop_token = finite_inplace_stop_token<1, 0>;

    template <typename CB>
    using single_inplace_stop_callback = finite_inplace_stop_callback<1, 0, CB>;

    class single_inplace_stop_source : private finite_inplace_stop_source<1>
    {
      public:
        single_inplace_stop_source() noexcept = default;

        bool request_stop() noexcept
        {
            return finite_inplace_stop_source<1>::request_stop();
        }
        bool stop_requested() noexcept
        {
            return finite_inplace_stop_source<1>::stop_requested();
        }

        single_inplace_stop_token get_token() const noexcept
        {
            return finite_inplace_stop_source<1>::get_token<0>();
        }
    };

    template <typename T, typename CB>
    using stop_callback_for_t = typename T::template callback_type<CB>;

} // namespace std

/////////////////////////////////////////////////////////////////////////////////
// Benchmarking Code Below Here
/////////////////////////////////////////////////////////////////////////////////

#include <chrono>
// #include <print>
#include <algorithm>
#include <numeric>
#include <vector>

// Define some helper functions

constexpr std::uint32_t iteration_count = 100'000;
constexpr std::uint32_t pass_count = 20;

template <typename F>
void timed_invoke(const char *label, F f)
{
    auto start = std::chrono::steady_clock::now();
    f();
    auto end = std::chrono::steady_clock::now();
    auto time = (end - start);
    auto time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(time).count();

    // std::print("{} took {: >4}.{:03}us\n", label, time_ns / 1000, time_ns % 1000);
    std::cout << label << " took " << std::setw(4) << std::setfill(' ')
              << (time_ns / 1000) << "." << std::setw(3) << std::setfill('0')
              << (time_ns % 1000) << "us\n";
}

void print_times(const char *label, std::vector<std::chrono::nanoseconds> &times)
{
    std::sort(times.begin(), times.end());

    auto min_time_ns = times.front().count();
    auto max_time_ns = times.back().count();
    auto total_time =
        std::accumulate(times.begin(), times.end(), std::chrono::nanoseconds{});
    auto avg_time = total_time / times.size();
    auto avg_time_ns = avg_time.count();
    auto p50_time = ((times.size() % 2) == 0 && (times.size() >= 2))
                        ? (times[times.size() / 2] + times[times.size() / 2 + 1]) / 2
                        : times[times.size() / 2];
    auto p50_time_ns = p50_time.count();

    // std::print(
    //     "{} {: >4}.{:03} - {: >4}.{:03}us (avg {: >4}.{:03}us, p50 {: >4}.{:03}us)\n",
    //     label, min_time_ns / 1000, min_time_ns % 1000, max_time_ns / 1000,
    //     max_time_ns % 1000, avg_time_ns / 1000, avg_time_ns % 1000, p50_time_ns / 1000,
    //     p50_time_ns % 1000);
    std::cout << label << " " << std::setw(4) << std::setfill(' ') << (min_time_ns / 1000)
              << "." << std::setw(3) << std::setfill('0') << (min_time_ns % 1000) << " - "
              << std::setw(4) << std::setfill(' ') << (max_time_ns / 1000) << "."
              << std::setw(3) << std::setfill('0') << (max_time_ns % 1000) << "us (avg "
              << std::setw(4) << std::setfill(' ') << (avg_time_ns / 1000) << "."
              << std::setw(3) << std::setfill('0') << (avg_time_ns % 1000) << "us, p50 "
              << std::setw(4) << std::setfill(' ') << (p50_time_ns / 1000) << "."
              << std::setw(3) << std::setfill('0') << (p50_time_ns % 1000) << "us)\n";
}

template <typename F>
void timed_invoke_multi(const char *label, std::uint32_t count, F f)
{
    if (count == 0)
        return;

    std::vector<std::chrono::nanoseconds> times;
    times.reserve(count);

    for (std::uint32_t i = 0; i < count; ++i)
    {
        auto start = std::chrono::steady_clock::now();
        f();
        auto end = std::chrono::steady_clock::now();
        times.push_back(
            std::chrono::duration_cast<std::chrono::nanoseconds>(end - start));
    }

    print_times(label, times);
}

//
// Single-Thread Regsiter/Unregister Callback
//

void single_thread_register_unregister_1()
{
    std::inplace_stop_source ss;
    auto cb = [x = 1] noexcept {
    };
    for (std::uint32_t i = 0; i < iteration_count; ++i)
    {
        std::inplace_stop_callback scb{ss.get_token(), cb};
    }
}

void single_thread_register_unregister_2()
{
    std::single_inplace_stop_source ss;
    auto cb = [x = 1] noexcept {
    };
    for (std::uint32_t i = 0; i < iteration_count; ++i)
    {
        std::single_inplace_stop_callback scb{ss.get_token(), cb};
    }
}

//
// Single-Thread No Callback + request_stop
//

void single_thread_no_callback_stop_1()
{
    for (std::uint32_t i = 0; i < iteration_count; ++i)
    {
        std::inplace_stop_source ss;
        ss.request_stop();
    }
}

template <std::size_t MaxCallbacks>
void single_thread_no_callback_stop_2()
{
    for (std::uint32_t i = 0; i < iteration_count; ++i)
    {
        std::array<std::single_inplace_stop_source, MaxCallbacks> ss;
        for (auto &s : ss)
        {
            s.request_stop();
        }
    }
}

template <std::size_t MaxCallbacks>
void single_thread_no_callback_stop_3()
{
    for (std::uint32_t i = 0; i < iteration_count; ++i)
    {
        std::finite_inplace_stop_source<MaxCallbacks> ss;
        ss.request_stop();
    }
}

//
// Single-Thread Register/Unregister Nx Callback + request_stop
//

template <std::size_t CallbackCount>
void single_thread_register_multiple_with_stop_1()
{
    for (std::uint32_t i = 0; i < iteration_count; ++i)
    {
        std::inplace_stop_source ss;
        std::size_t count = 0;
        auto cb = [&] {
            ++count;
        };

        using stop_callback_t = std::inplace_stop_callback<decltype(cb)>;

        auto cbs = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return std::array<stop_callback_t, CallbackCount>{
                ((void)Is, stop_callback_t{ss.get_token(), cb})...};
        }(std::make_index_sequence<CallbackCount>{});

        ss.request_stop();

        if (count != CallbackCount)
        {
            std::terminate();
        }
    }
}

template <std::size_t Count, std::size_t CallbackCount>
void single_thread_register_multiple_with_stop_2()
{
    for (std::uint32_t i = 0; i < iteration_count; ++i)
    {
        std::array<std::single_inplace_stop_source, Count> ss;
        std::size_t count = 0;
        auto cb = [&] {
            ++count;
        };

        using stop_callback_t = std::single_inplace_stop_callback<decltype(cb)>;

        auto cbs = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return std::array<stop_callback_t, CallbackCount>{
                stop_callback_t{ss[Is].get_token(), cb}...};
        }(std::make_index_sequence<CallbackCount>{});

        for (std::size_t j = 0; j < Count; ++j)
        {
            ss[j].request_stop();
        }

        if (count != CallbackCount)
        {
            std::terminate();
        }
    }
}

template <std::size_t Count, typename CB, std::size_t... Is>
struct finite_stop_callback_tuple : std::finite_inplace_stop_callback<Count, Is, CB>...
{
    finite_stop_callback_tuple(std::finite_inplace_stop_source<Count> &ss, const CB &cb)
        : std::finite_inplace_stop_callback<Count, Is, CB>{ss.template get_token<Is>(),
                                                           cb}...
    {
    }
};

template <std::size_t Count, std::size_t CallbackCount>
void single_thread_register_multiple_with_stop_3()
{
    for (std::uint32_t i = 0; i < iteration_count; ++i)
    {
        std::finite_inplace_stop_source<Count> ss;
        std::size_t count = 0;
        auto cb = [&] {
            ++count;
        };
        using cb_t = decltype(cb);

        auto cbs = [&]<std::size_t... Is>(std::index_sequence<Is...>) {
            return finite_stop_callback_tuple<Count, cb_t, Is...>{ss, cb};
        }(std::make_index_sequence<CallbackCount>{});

        ss.request_stop();

        if (count != CallbackCount)
        {
            std::terminate();
        }
    }
}

//
// Two-Threads Register/Unregister Callback
//

template <typename Func1, typename Func2>
std::vector<std::chrono::nanoseconds> run_two_threads_concurrently(Func1 func1,
                                                                   Func2 func2)
{

    std::atomic<bool> t1_ready{false};
    std::atomic<bool> t2_ready{false};

    auto compute_times = [&](auto &func, std::atomic<bool> &ready) {
        std::vector<std::chrono::nanoseconds> times;
        times.reserve(pass_count);

        for (std::uint32_t pass = 0; pass < pass_count; ++pass)
        {
            ready.store(true);
            while (ready.load())
            {
            }
            auto start = std::chrono::steady_clock::now();

            func();

            auto end = std::chrono::steady_clock::now();
            auto time = (end - start);
            times.push_back(time);
        }
        return times;
    };

    std::vector<std::chrono::nanoseconds> t1_times;
    std::vector<std::chrono::nanoseconds> t2_times;

    std::thread t1{[&] {
        t1_times = compute_times(func1, t1_ready);
    }};
    std::thread t2{[&] {
        t2_times = compute_times(func2, t2_ready);
    }};

    for (std::uint32_t pass = 0; pass < pass_count; ++pass)
    {
        while (!t1_ready.load())
        {
        }
        while (!t2_ready.load())
        {
        }
        t1_ready.store(false);
        t2_ready.store(false);
    }

    t1.join();
    t2.join();

    std::vector<std::chrono::nanoseconds> all_times;
    all_times.reserve(t1_times.size() + t2_times.size());
    all_times.insert(all_times.end(), t1_times.begin(), t1_times.end());
    all_times.insert(all_times.end(), t2_times.begin(), t2_times.end());
    return all_times;
}

void two_threads_register_unregister_1()
{
    std::inplace_stop_source ss;

    auto register_callbacks = [&] {
        auto cb = [x = 1] {
        };
        for (std::uint32_t i = 0; i < iteration_count; ++i)
        {
            std::inplace_stop_callback scb{ss.get_token(), cb};
        }
    };

    auto times = run_two_threads_concurrently(register_callbacks, register_callbacks);

    print_times("  inplace_stop_source                              : ", times);
}

void two_threads_register_unregister_2()
{
    std::single_inplace_stop_source ss1;
    std::single_inplace_stop_source ss2;

    auto register_callbacks_1 = [&] {
        auto cb = [x = 1] {
        };
        for (std::uint32_t i = 0; i < iteration_count; ++i)
        {
            std::single_inplace_stop_callback scb{ss1.get_token(), cb};
        }
    };

    auto register_callbacks_2 = [&] {
        auto cb = [x = 1] {
        };
        for (std::uint32_t i = 0; i < iteration_count; ++i)
        {
            std::single_inplace_stop_callback scb{ss2.get_token(), cb};
        }
    };

    auto times = run_two_threads_concurrently(register_callbacks_1, register_callbacks_2);

    print_times("  2x single_inplace_stop_source                    : ", times);
}

void two_threads_register_unregister_2a()
{
    alignas(64) std::single_inplace_stop_source ss1;
    alignas(64) std::single_inplace_stop_source ss2;

    auto register_callbacks_1 = [&] {
        auto cb = [x = 1] {
        };
        for (std::uint32_t i = 0; i < iteration_count; ++i)
        {
            std::single_inplace_stop_callback scb{ss1.get_token(), cb};
        }
    };

    auto register_callbacks_2 = [&] {
        auto cb = [x = 1] {
        };
        for (std::uint32_t i = 0; i < iteration_count; ++i)
        {
            std::single_inplace_stop_callback scb{ss2.get_token(), cb};
        }
    };

    auto times = run_two_threads_concurrently(register_callbacks_1, register_callbacks_2);

    print_times("  2x single_inplace_stop_source (no false sharing) : ", times);
}

void two_threads_register_unregister_3()
{
    std::finite_inplace_stop_source<2> ss;

    auto register_callbacks_1 = [&] {
        auto cb = [x = 1] {
        };
        for (std::uint32_t i = 0; i < iteration_count; ++i)
        {
            std::finite_inplace_stop_callback scb{ss.get_token<0>(), cb};
        }
    };

    auto register_callbacks_2 = [&] {
        auto cb = [x = 1] {
        };
        for (std::uint32_t i = 0; i < iteration_count; ++i)
        {
            std::finite_inplace_stop_callback scb{ss.get_token<1>(), cb};
        }
    };

    auto times = run_two_threads_concurrently(register_callbacks_1, register_callbacks_2);

    print_times("  finite_inplace_stop_source<2>                    : ", times);
}

int main()
{

    // std::print("Register/unregister stop-callbacks single-threaded (100k times)\n");
    std::cout << "Register/unregister stop-callbacks single-threaded (100k times)\n";
    {
        std::inplace_stop_source ss;
        timed_invoke_multi("  inplace_stop_source        : ", pass_count,
                           [&] { single_thread_register_unregister_1(); });
    }

    {
        std::single_inplace_stop_source ss;
        timed_invoke_multi("  single_inplace_stop_source : ", pass_count,
                           [&] { single_thread_register_unregister_2(); });
    }

    // std::print("\nCall request_stop() with no callbacks (100k times)\n");
    std::cout << "\nCall request_stop() with no callbacks (100k times)\n";

    timed_invoke_multi("  inplace_stop_source            : ", pass_count,
                       [&] { single_thread_no_callback_stop_1(); });

    timed_invoke_multi("  1x single_inplace_stop_source  : ", pass_count,
                       [&] { single_thread_no_callback_stop_2<1>(); });

    timed_invoke_multi("  2x single_inplace_stop_source  : ", pass_count,
                       [&] { single_thread_no_callback_stop_2<2>(); });

    timed_invoke_multi("  finite_inplace_stop_source<2>  : ", pass_count,
                       [&] { single_thread_no_callback_stop_3<2>(); });

    timed_invoke_multi("  3x single_inplace_stop_source  : ", pass_count,
                       [&] { single_thread_no_callback_stop_2<3>(); });

    timed_invoke_multi("  finite_inplace_stop_source<3>  : ", pass_count,
                       [&] { single_thread_no_callback_stop_3<3>(); });

    timed_invoke_multi("  10x single_inplace_stop_source : ", pass_count,
                       [&] { single_thread_no_callback_stop_2<10>(); });

    timed_invoke_multi("  finite_inplace_stop_source<10> : ", pass_count,
                       [&] { single_thread_no_callback_stop_3<10>(); });

    // std::print("\nCall request_stop() with 1/1 callback (100k times)\n");
    std::cout << "\nCall request_stop() with 1/1 callback (100k times)\n";
    timed_invoke_multi("  inplace_stop_source           : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_1<1>(); });

    timed_invoke_multi("  single_inplace_stop_source    : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_2<1, 1>(); });

    // std::print("\nCall request_stop() with 1/2 callbacks (100k times)\n");
    std::cout << "\nCall request_stop() with 1/2 callbacks (100k times)\n";
    timed_invoke_multi("  2x single_inplace_stop_source : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_2<2, 1>(); });

    timed_invoke_multi("  finite_inplace_stop_source<2> : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_3<2, 1>(); });

    // std::print("\nCall request_stop() with 1/3 callbacks (100k times)\n");
    std::cout << "\nCall request_stop() with 1/3 callbacks (100k times)\n";
    timed_invoke_multi("  3x single_inplace_stop_source : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_2<3, 1>(); });

    timed_invoke_multi("  finite_inplace_stop_source<3> : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_3<3, 1>(); });

    // std::print("\nCall request_stop() with 2/2 callbacks (100k times)\n");
    std::cout << "\nCall request_stop() with 2/2 callbacks (100k times)\n";
    timed_invoke_multi("  inplace_stop_source           : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_1<2>(); });

    timed_invoke_multi("  2x single_inplace_stop_source : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_2<2, 2>(); });

    timed_invoke_multi("  finite_inplace_stop_source<2> : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_3<2, 2>(); });

    // std::print("\nCall request_stop() with 3/3 callbacks (100k times)\n");
    std::cout << "\nCall request_stop() with 3/3 callbacks (100k times)\n";
    timed_invoke_multi("  inplace_stop_source           : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_1<3>(); });

    timed_invoke_multi("  3x single_inplace_stop_source : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_2<3, 3>(); });

    timed_invoke_multi("  finite_inplace_stop_source<3> : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_3<3, 3>(); });

    // std::print("\nCall request_stop() with 10/10 callbacks (100k times)\n");
    std::cout << "\nCall request_stop() with 10/10 callbacks (100k times)\n";
    timed_invoke_multi("  inplace_stop_source            : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_1<10>(); });

    timed_invoke_multi("  10x single_inplace_stop_source : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_2<10, 10>(); });

    timed_invoke_multi("  finite_inplace_stop_source<10> : ", pass_count,
                       [&] { single_thread_register_multiple_with_stop_3<10, 10>(); });

    // std::print("\nRegister/unregister callbacks from two threads concurrently\n");
    std::cout << "\nRegister/unregister callbacks from two threads concurrently\n";
    two_threads_register_unregister_1();
    two_threads_register_unregister_2();
    two_threads_register_unregister_2a();
    two_threads_register_unregister_3();

    // Display information about sizes
    {
        // std::print("\n"
        //            "Data-Structure Sizes\n"
        //            "--------------------\n");
        // std::print("inplace_stop_source                 : {} bytes\n",
        //            sizeof(std::inplace_stop_source));
        // std::print("single_inplace_stop_source          : {} bytes\n",
        //            sizeof(std::single_inplace_stop_source));
        // std::print("finite_inplace_stop_source<2>       : {} bytes\n",
        //            sizeof(std::finite_inplace_stop_source<2>));
        // std::print("finite_inplace_stop_source<3>       : {} bytes\n\n",
        //            sizeof(std::finite_inplace_stop_source<3>));
        std::cout << "\n"
                  << "Data-Structure Sizes\n"
                  << "--------------------\n";
        std::cout << "inplace_stop_source                 : "
                  << sizeof(std::inplace_stop_source) << " bytes\n";

        std::cout << "single_inplace_stop_source          : "
                  << sizeof(std::single_inplace_stop_source) << " bytes\n";

        std::cout << "finite_inplace_stop_source<2>       : "
                  << sizeof(std::finite_inplace_stop_source<2>) << " bytes\n";

        std::cout << "finite_inplace_stop_source<3>       : "
                  << sizeof(std::finite_inplace_stop_source<3>) << " bytes\n\n";

        int x = 0;
        auto cb = [&x] {
            ++x;
        };

        // std::print("inplace_stop_callback               : {} bytes\n",
        //            sizeof(std::inplace_stop_callback<decltype(cb)>));
        std::cout << "inplace_stop_callback               : "
                  << sizeof(std::inplace_stop_callback<decltype(cb)>) << " bytes\n ";
        std::cout << "single_inplace_stop_callback               : "
                  << sizeof(std::single_inplace_stop_callback<decltype(cb)>)
                  << " bytes\n ";
        std::cout << "finite_inplace_stop_callback               : "
                  << sizeof(std::finite_inplace_stop_callback<2, 0, decltype(cb)>)
                  << " bytes\n ";
        std::cout << "inplace_stop_source + 1x callbacks               : "
                  << sizeof(std::inplace_stop_source) +
                         sizeof(std::inplace_stop_callback<decltype(cb)>)
                  << " bytes\n ";
        std::cout << "inplace_stop_source + 2x callbacks               : "
                  << sizeof(std::inplace_stop_source) +
                         2 * sizeof(std::inplace_stop_callback<decltype(cb)>)
                  << " bytes\n ";
        std::cout << "finite_inplace_stop_source<2> + 2x callbacks               : "
                  << sizeof(std::finite_inplace_stop_source<2>) +
                         sizeof(std::finite_inplace_stop_callback<2, 0, decltype(cb)>) +
                         sizeof(std::finite_inplace_stop_callback<2, 1, decltype(cb)>)
                  << " bytes\n ";

        // 输出第一组数据
        std::cout << "inplace_stop_source + 3x callbacks           : "
                  << sizeof(std::inplace_stop_source) +
                         3 * sizeof(std::inplace_stop_callback<decltype(cb)>)
                  << " bytes\n";

        std::cout << "3x single_inplace_stop_source + 3x callbacks : "
                  << 3 * sizeof(std::single_inplace_stop_source) +
                         3 * sizeof(std::single_inplace_stop_callback<decltype(cb)>)
                  << " bytes\n";

        std::cout << "finite_inplace_stop_source<3> + 3x callbacks : "
                  << sizeof(std::finite_inplace_stop_source<3>) +
                         sizeof(std::finite_inplace_stop_callback<3, 0, decltype(cb)>) +
                         sizeof(std::finite_inplace_stop_callback<3, 1, decltype(cb)>) +
                         sizeof(std::finite_inplace_stop_callback<3, 2, decltype(cb)>)
                  << " bytes\n\n";

        // 输出第二组数据
        std::cout << "inplace_stop_source + 10x callbacks            : "
                  << sizeof(std::inplace_stop_source) +
                         10 * sizeof(std::inplace_stop_callback<decltype(cb)>)
                  << " bytes\n";

        std::cout << "10x single_inplace_stop_source + 10x callbacks : "
                  << 10 * sizeof(std::single_inplace_stop_source) +
                         10 * sizeof(std::single_inplace_stop_callback<decltype(cb)>)
                  << " bytes\n";

        std::cout << "finite_inplace_stop_source<10> + 10x callbacks : "
                  << sizeof(std::finite_inplace_stop_source<10>) +
                         sizeof(std::finite_inplace_stop_callback<10, 0, decltype(cb)>) +
                         sizeof(std::finite_inplace_stop_callback<10, 1, decltype(cb)>) +
                         sizeof(std::finite_inplace_stop_callback<10, 2, decltype(cb)>) +
                         sizeof(std::finite_inplace_stop_callback<10, 3, decltype(cb)>) +
                         sizeof(std::finite_inplace_stop_callback<10, 4, decltype(cb)>) +
                         sizeof(std::finite_inplace_stop_callback<10, 5, decltype(cb)>) +
                         sizeof(std::finite_inplace_stop_callback<10, 6, decltype(cb)>) +
                         sizeof(std::finite_inplace_stop_callback<10, 7, decltype(cb)>) +
                         sizeof(std::finite_inplace_stop_callback<10, 8, decltype(cb)>) +
                         sizeof(std::finite_inplace_stop_callback<10, 9, decltype(cb)>)
                  << " bytes\n";
#if false
        std::print("finite_inplace_stop_callback<2>     : {} bytes\n\n",
                   sizeof(std::finite_inplace_stop_callback<2, 0, decltype(cb)>));
        std::print("inplace_stop_source + 1x callbacks           : {} bytes\n",
                   sizeof(std::inplace_stop_source) +
                       sizeof(std::inplace_stop_callback<decltype(cb)>));
        std::print("1x single_inplace_stop_source + 1x callbacks : {} bytes\n\n",
                   sizeof(std::single_inplace_stop_source) +
                       sizeof(std::single_inplace_stop_callback<decltype(cb)>));

        std::print("inplace_stop_source + 2x callbacks           : {} bytes\n",
                   sizeof(std::inplace_stop_source) +
                       2 * sizeof(std::inplace_stop_callback<decltype(cb)>));
        std::print("2x single_inplace_stop_source + 2x callbacks : {} bytes\n",
                   2 * sizeof(std::single_inplace_stop_source) +
                       2 * sizeof(std::single_inplace_stop_callback<decltype(cb)>));
        std::print("finite_inplace_stop_source<2> + 2x callbacks : {} bytes\n\n",
                   sizeof(std::finite_inplace_stop_source<2>) +
                       sizeof(std::finite_inplace_stop_callback<2, 0, decltype(cb)>) +
                       sizeof(std::finite_inplace_stop_callback<2, 1, decltype(cb)>));

        std::print("inplace_stop_source + 3x callbacks           : {} bytes\n",
                   sizeof(std::inplace_stop_source) +
                       3 * sizeof(std::inplace_stop_callback<decltype(cb)>));
        std::print("3x single_inplace_stop_source + 3x callbacks : {} bytes\n",
                   3 * sizeof(std::single_inplace_stop_source) +
                       3 * sizeof(std::single_inplace_stop_callback<decltype(cb)>));
        std::print("finite_inplace_stop_source<3> + 3x callbacks : {} bytes\n\n",
                   sizeof(std::finite_inplace_stop_source<3>) +
                       sizeof(std::finite_inplace_stop_callback<3, 0, decltype(cb)>) +
                       sizeof(std::finite_inplace_stop_callback<3, 1, decltype(cb)>) +
                       sizeof(std::finite_inplace_stop_callback<3, 2, decltype(cb)>));

        std::print("inplace_stop_source + 10x callbacks            : {} bytes\n",
                   sizeof(std::inplace_stop_source) +
                       10 * sizeof(std::inplace_stop_callback<decltype(cb)>));
        std::print("10x single_inplace_stop_source + 10x callbacks : {} bytes\n",
                   10 * sizeof(std::single_inplace_stop_source) +
                       10 * sizeof(std::single_inplace_stop_callback<decltype(cb)>));
        std::print("finite_inplace_stop_source<10> + 10x callbacks : {} bytes\n",
                   sizeof(std::finite_inplace_stop_source<10>) +
                       sizeof(std::finite_inplace_stop_callback<10, 0, decltype(cb)>) +
                       sizeof(std::finite_inplace_stop_callback<10, 1, decltype(cb)>) +
                       sizeof(std::finite_inplace_stop_callback<10, 2, decltype(cb)>) +
                       sizeof(std::finite_inplace_stop_callback<10, 3, decltype(cb)>) +
                       sizeof(std::finite_inplace_stop_callback<10, 4, decltype(cb)>) +
                       sizeof(std::finite_inplace_stop_callback<10, 5, decltype(cb)>) +
                       sizeof(std::finite_inplace_stop_callback<10, 6, decltype(cb)>) +
                       sizeof(std::finite_inplace_stop_callback<10, 7, decltype(cb)>) +
                       sizeof(std::finite_inplace_stop_callback<10, 8, decltype(cb)>) +
                       sizeof(std::finite_inplace_stop_callback<10, 9, decltype(cb)>));
#endif
    }
}
// NOLINTEND