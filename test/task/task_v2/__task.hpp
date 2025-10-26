#pragma once

#include "./__detail/__get_allocator_type.hpp"
#include "./__detail/__get_scheduler_type.hpp"
#include "./__detail/__get_stop_source_type.hpp"
#include "./__detail/__get_error_types.hpp"
#include "./__detail/__state_base.hpp"
#include "./__detail/__pomise_return.hpp"

#include "./__with_error.hpp"
#include "./__change_coroutine_scheduler.hpp"
#include "./__affine_on.hpp"
#include "./__inline_scheduler.hpp"

#include <algorithm>
#include <coroutine>
#include <type_traits>
#include <utility>
#include <variant>

namespace mcs::execution::task_v2
{

    // [exec.task]
    template <class T, class Environment>
    struct task
    {
        using sender_concept = sender_t;

        using allocator_type = __detail::get_allocator_type<Environment>;
        using scheduler_type = __detail::get_scheduler_type<Environment>;
        using stop_source_type = __detail::get_stop_source_type<Environment>;
        using stop_token_type = decltype(std::declval<stop_source_type>().get_token());
        using error_types = __detail::get_error_types<Environment>;
        using completion_signatures =
            decltype(cmplsigs::completion_signatures<
                         std::conditional_t<std::is_void_v<T>, recv::set_value_t(),
                                            recv::set_value_t(T)>,
                         recv::set_stopped_t()>{} +
                     error_types{});

        // [task.promise]
        struct promise_type : __detail::pomise_return<T>
        {
            template <class... Args>
            constexpr explicit promise_type(const Args &...args) noexcept
                : alloc_{[](const Args &...args) constexpr noexcept -> decltype(auto) {
                      // Mandates: The first parameter of type allocator_arg_t (if any) is
                      // not the last parameter.
                      if constexpr (sizeof...(Args) > 1 &&
                                    std::is_same_v<
                                        std::remove_cvref_t<decltype(std::get<0>(
                                            std::forward_as_tuple(args...)))>,
                                        std::allocator_arg_t>)
                          // forward_as_tuple remian cvref
                          return std::get<1>(std::forward_as_tuple(args...));
                      else
                          return allocator_type{};
                  }(args...)}
            {
            }
            // NOLINTBEGIN
            constexpr task get_return_object() noexcept
            {
                return std::coroutine_handle<promise_type>::from_promise(*this);
            }
            static constexpr auto initial_suspend() noexcept
            {
                return std::suspend_always{};
            }

            static constexpr auto final_suspend() noexcept
            {
                struct final_awaiter
                {
                    static constexpr auto await_ready() noexcept -> bool
                    {
                        return false;
                    }
                    static auto await_suspend(
                        std::coroutine_handle<promise_type> handle) noexcept
                    {
                        handle.promise().state_->completion(handle.promise().state_);
                    }
                    static constexpr void await_resume() noexcept {}
                };
                return final_awaiter{};
            }

            constexpr void unhandled_exception() noexcept
            {
                if constexpr (requires {
                                  errors_.template emplace<std::exception_ptr>(
                                      std::current_exception());
                              })
                    errors_.template emplace<std::exception_ptr>(
                        std::current_exception());
                else
                    std::terminate();
            }
            constexpr std::coroutine_handle<> unhandled_stopped() noexcept
            {
                state_->unhandled_stopped(state_);
                return std::noop_coroutine();
            }

            template <class E>
            constexpr auto yield_value(with_error<E> error) noexcept // NOLINT
                requires requires() {
                    this->errors_.template emplace<E>(std::move(error));
                }
            {
                errors_.template emplace<E>(std::move(error));
            }

            template <sender Sender>
            auto await_transform(Sender &&sndr) noexcept
            {
                if constexpr (std::same_as<inline_scheduler, scheduler_type>)
                {
                    return awaitables::as_awaitable(std::forward<Sender>(sndr), *this);
                }
                else
                {
                    // TODO SCHED form interface_base class cant 
                    return awaitables::as_awaitable(
                        affine_on(std::forward<Sender>(sndr), SCHED(*this)), *this);
                }
            }
            // NOLINTEND
            template <class Sch>
            auto await_transform(change_coroutine_scheduler<Sch> sch)
            {
                return awaitables::as_awaitable(
                    factories::just(
                        std::exchange(SCHED(*this), scheduler_type(sch.scheduler))),
                    *this);
            }

            auto get_env() const noexcept
            {
                struct env
                {
                };
                return env{};
            }

            template <class... Args>
            void *operator new(size_t size, Args &&...args);

            void operator delete(void *pointer, size_t size) noexcept;

          private:
            using error_variant = typename decltype([] consteval {
                // variant<monostate, remove_cvref_t<E>...>
                using Errs = completion_signatures::template filter_sigs<set_error_t>();
                auto *value = []<typename... E>(
                                  cmplsigs::completion_signatures<set_error_t(E)...> *) {
                    using V = std::variant<std::monostate, std::remove_cvref_t<E>...>;
                    return static_cast<V *>(nullptr);
                }(static_cast<Errs *>(nullptr))();
                return std::type_identity<std::remove_pointer_t<decltype(value)>>{};
            }())::type; // exposition only

            allocator_type alloc_;    // exposition only
            stop_source_type source_; // exposition only
            stop_token_type token_;   // exposition only
            error_variant errors_;    // exposition only
            __detail::state_base *state_{};

            friend struct final_awaiter;

            template <recv::receiver R>
            friend struct state;
        };

        // [task.state]
        template <recv::receiver R>
        struct state : __detail::state_base
        {
          public:
            using operation_state_concept = operation_state_t;

            template <class Rcvr>
            constexpr state(std::coroutine_handle<promise_type> h, Rcvr &&rcvr) noexcept
                : state_base{.completion = &do_completion}, handle_(std::move(h)),
                  rcvr_(std::forward<Rcvr>(rcvr)),
                  own_env_{[](auto &rcvr) constexpr noexcept {
                      if constexpr (requires() { own_env_t{queries::get_env(rcvr)}; })
                          return own_env_t{queries::get_env(rcvr)};
                      else if constexpr (requires() { own_env_t{}; })
                          return own_env_t{};
                      else
                          static_assert(false, "the own_env_t is ill-formed");
                  }(rcvr_)},
                  environment_{[](auto &rcvr, auto &own_env) constexpr noexcept {
                      if constexpr (requires() { Environment{own_env}; })
                          return Environment{own_env};
                      else if constexpr (requires() {
                                             Environment{queries::get_env(rcvr)};
                                         })
                          return Environment{queries::get_env(rcvr)};
                      else if constexpr (requires() { Environment{}; })
                          return Environment{};
                      else
                          static_assert(false, "the Environment is ill-formed");
                  }(rcvr_, own_env_)}
            {
            }
            constexpr ~state() noexcept
            {
                if (handle_)
                    handle_.destroy();
            }
            state(const state &) = delete;
            state(state &&) = delete;
            state &operator=(const state &) = delete;
            state &operator=(state &&) = delete;

            struct forward_stop_request // TODO(mcs): 猜的 when_all 有类似的
            {
                stop_source_type &stop_src; // NOLINT
                void operator()() noexcept
                {
                    stop_src.request_stop();
                }
            };
            // Note: 模板+具体类型 => 生成确定的类型
            // stop_callback == token + CallbackFn
            using stop_callback = typename stoptoken::stop_callback_of_t<
                queries::stop_token_of_t<queries::env_of_t<R>>, forward_stop_request>;

            void start() & noexcept
            {
                auto &prom = handle_.promise();
                prom.state_ = this;

                if constexpr (requires {
                                  scheduler_ = scheduler_type{
                                      queries::get_scheduler(queries::get_env(rcvr_))};
                              })
                    scheduler_ =
                        scheduler_type{queries::get_scheduler(queries::get_env(rcvr_))};
                else if constexpr (requires { scheduler_ = scheduler_type{}; })
                    scheduler_ = scheduler_type{};
                else
                    static_assert(false, "the scheduler_type is ill-formed");
#if 0 // TODO(mcs): 不理解 如何代理
      // stoptoken::stoppable_callback_for 概念
                on_stop_.emplace(
                    std::move(queries::get_stop_token(queries::get_env(rcvr_))),
                    forward_stop_request{prom.source_});
#endif
                // NOTE: set promise的一些成员
                handle_.resume();
            }

          private:
            using own_env_t = typename decltype([] consteval {
                if constexpr (requires {
                                  typename Environment::template env_type<
                                      decltype(queries::get_env(std::declval<R>()))>;
                              })
                    return std::type_identity<typename Environment::template env_type<
                        decltype(queries::get_env(std::declval<R>()))>>{};
                else
                    return std::type_identity<env<>>{};
            }())::type;

            std::coroutine_handle<promise_type> handle_; // exposition only
            std::remove_cvref_t<R> rcvr_;                // exposition only
            own_env_t own_env_;                          // exposition only// NOLINT
            Environment environment_;                    // exposition only
            scheduler_type scheduler_;
            stop_callback on_stop_; // NOLINT

            constexpr static void do_completion(state_base *ptr) noexcept // NOLINT
            {
                auto &self = *static_cast<state<R> *>(ptr);
                auto &prom = self.handle_.promise();
                if constexpr (std::variant_size_v<decltype(prom.errors_)> > 1)
                {
                    if (prom.errors_.index() > 0)
                    {
                        std::visit(
                            [&](auto &e) constexpr noexcept {
                                recv::set_error(std::move(self.rcvr_), std::move(e));
                            },
                            prom.errors_);
                        return;
                    }
                }

                if constexpr (std::is_void_v<T>)
                    recv::set_value(std::move(self.rcvr_));
                else
                    recv::set_value(std::move(self.rcvr_), *(prom.result_));
            }
            // unhandled_stopped
            constexpr static void do_unhandled_stopped(state_base *ptr) noexcept // NOLINT
            {
                auto &self = *static_cast<state<R> *>(ptr);
                recv::set_stopped(std::move(self.rcvr_));
            }
        };

        task(const task &) = delete;
        task &operator=(const task &) = delete;
        task &operator=(task &&) = delete;
        // Effects: Initializes handle with exchange(other.handle, {})
        constexpr task(task &&other) noexcept : handle_{std::exchange(other.handle_, {})}
        {
        }
        constexpr ~task() noexcept
        {
            if (handle_)
                handle_.destroy();
        }
        constexpr explicit task(std::coroutine_handle<promise_type> h) noexcept
            : handle_{h}
        {
        }

        template <recv::receiver R>
        constexpr state<R> connect(R &&recv) noexcept
        {
            assert(bool(handle_)); // Preconditions: bool(handle) is true.
            return {std::exchange(handle_, {}), std::forward<R>(recv)};
        }

      private:
        std::coroutine_handle<promise_type> handle_; // exposition only
    };

}; // namespace mcs::execution::task_v2