#pragma once

#include <atomic>
#include <cassert>
#include <cstddef>
#include <thread>

#include "./__invocable_destructible.hpp"

namespace mcs::execution::stoptoken
{
    template <__detail::invocable_destructible CB>
    class finite_inplace_stop_callback_base; // NOLINT

    class finite_inplace_stop_source_base // NOLINT
    {
      protected:
        template <__detail::invocable_destructible CB>
        friend class finite_inplace_stop_callback_base;

        struct callback_base
        {
            // void (*execute)(callback_base *self) noexcept;
            using callback_fun_t = void (*)(callback_base *self) noexcept;
            callback_fun_t execute;
        };

        static void *stop_requested_state(std::atomic<void *> *states) noexcept // NOLINT
        {
            return states;
        }
        void *stop_requested_callback_done_state() const noexcept // NOLINT
        {
            return &thread_requesting_stop;
        }
        constexpr static void *no_callback_state() noexcept // NOLINT
        {
            return nullptr;
        }

        bool is_stop_requested_state(std::atomic<void *> *states, // NOLINT
                                     void *state) const noexcept
        {
            bool result = (state == stop_requested_state(states));
            result |= (state == stop_requested_callback_done_state());
            return result;
        }

        // NOLINTNEXTLINE
        bool try_register_callback(std::size_t /*count*/, std::atomic<void *> *states,
                                   std::size_t idx, callback_base *cb) const noexcept
        {
            auto &state = states[idx];
            void *old_state = state.load(std::memory_order_acquire);
            if (is_stop_requested_state(states, old_state))
            {
                return false;
            }

            assert(old_state == no_callback_state());

            if (state.compare_exchange_strong(old_state, static_cast<void *>(cb),
                                              std::memory_order_release,
                                              std::memory_order_acquire))
            {
                // Successfully registered callback.
                return true;
            }

            // Stop request arrived while we were trying to register
            assert(old_state == stop_requested_state(states));

            return false;
        }

        // NOLINTNEXTLINE
        void deregister_callback(std::size_t /*count*/, std::atomic<void *> *states,
                                 std::size_t idx, callback_base *cb) const noexcept
        {
            // Initially assume that the callback has not been invoked and that the state
            // still points to the registered callback_base structure.
            auto &state = states[idx];

            void *old_state = static_cast<void *>(cb);
            if (state.compare_exchange_strong(old_state, no_callback_state(),
                                              std::memory_order_relaxed,
                                              std::memory_order_acquire))
            {
                // Successfully deregistered the callback before it could be invoked.
                return;
            }

            // Otherwise, a call to request_stop() is invoking the callback.
            if (old_state == stop_requested_state(states))
            {
                // Callback not finished executing yet.
                if (thread_requesting_stop.load(std::memory_order_relaxed) ==
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
                state.wait(old_state, std::memory_order_acquire);
            }
        }
        // NOLINTNEXTLINE
        bool request_stop_impl(std::size_t count, std::atomic<void *> *states) noexcept
        {
            assert(count >= 1);
            auto &first_state = states[0];
            void *old_state = first_state.load(std::memory_order_relaxed);
            do // NOLINT
            {
                if (is_stop_requested_state(states, old_state))
                {
                    return false;
                }
            } while (!first_state.compare_exchange_weak(
                old_state, stop_requested_state(states), std::memory_order_acq_rel,
                std::memory_order_relaxed));

            thread_requesting_stop.store(std::this_thread::get_id(),
                                         std::memory_order_relaxed);

            if (old_state != no_callback_state())
            {
                auto *callback = static_cast<callback_base *>(old_state);
                callback->execute(callback);
                first_state.store(stop_requested_callback_done_state(),
                                  std::memory_order_release);
                first_state.notify_one();
            }

            for (std::size_t i = 1; i < count; ++i)
            {
                old_state = states[i].exchange(stop_requested_state(states),
                                               std::memory_order_acq_rel);
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

        mutable std::atomic<std::thread::id> thread_requesting_stop; // NOLINT
    };

}; // namespace mcs::execution::stoptoken