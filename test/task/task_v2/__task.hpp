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
            // TODO: SCHED(*this). scheduler_type 在 St
            static constexpr auto initial_suspend() noexcept
            {
                return std::suspend_always{};
            }

            // TODO STATE(*this) 有关
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
                // TODO RCVR(*this)
                //  Completes the asynchronous operation associated with STATE(*this) by
                //  invoking set_stopped(std::move(RCVR(*this))).
                state_->unhandled_stopped(state_);
                return std::noop_coroutine();
            }

            // TODO RCVR(*this) 有关
            template <class E>
            constexpr auto yield_value(with_error<E> error) noexcept // NOLINT
                requires requires() {
                    this->errors_.template emplace<E>(std::move(error));
                }
            {
                errors_.template emplace<E>(std::move(error));
            }

            template <sender Sender>
            auto await_transform(Sender &&sndr) noexcept -> decltype(auto)
            {
                if constexpr (std::same_as<inline_scheduler, scheduler_type>)
                {
                    return awaitables::as_awaitable(std::forward<Sender>(sndr), *this);
                }
                else
                {
                    // TODO SCHED(*this)
                    // NOTE: affine_on 并没有具体实现要做什么。 sched 是空 如何处理？
                    // NOTE: 暂时别动。并没有设计好
                    // NOTE: 在那启动在哪结束不保证的化，很难处理的
                    return awaitables::as_awaitable(
                        affine_on(std::forward<Sender>(sndr), SCHED(*this)), *this);
                }
            }
            // NOLINTEND
            // NOTE: co_await change_coroutine_scheduler<Sch>{};的表达式处理
            template <class Sch>
            auto await_transform(change_coroutine_scheduler<Sch> sch) -> decltype(auto)
            {
                // TODO(mcs): SCHED(*this)
                return awaitables::as_awaitable(
                    factories::just(
                        std::exchange(SCHED(*this), scheduler_type(sch.scheduler))),
                    *this);
            }

            auto get_env() const noexcept
            {
                struct env
                {
                    // env.query(get_scheduler) returns scheduler_type(SCHED(*this))
                    // env.query(get_allocator) returns alloc.
                    // env.query(get_stop_token) returns token

                    // For any other query q and arguments a... a call to env.query(q,
                    // a...) returns STATE(*this).environment.query(q, a...) if this
                    // expression is well-formed and forwarding_query(q) is well-formed
                    // and is true. Otherwise env.query(q, a...) is ill-formed.
                    // STATE(*this).environment 在 STATE 中。
                    // NOTE: 直觉是类型转化。 promise_type 也是异步操作对象
                    // NOTE: 切片如何解决？
                    // NOTE: 这里要返回 join_able 的对象。 CONNECT的时候需要确定
                    // NOTE: connect是最后的机会。所有的操作也在这个时候定型了
                    // NOTE: STATE 是根据模板参数 R 构建的

                    // NOTE: 类型信息无法跨越

                    // NOTE: 仅仅是 R 不可知。
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
            scheduler_type scheduler_;
            Environment environment_; // exposition only

            friend struct final_awaiter;

            template <recv::receiver R>
            friend struct state;
        };

        // [task.state]
        template <recv::receiver R>
        struct state
        {
          public:
            using operation_state_concept = operation_state_t;

            template <class Rcvr>
            constexpr state(std::coroutine_handle<promise_type> h, Rcvr &&rcvr) noexcept
                : handle_(std::move(h)), rcvr_(std::forward<Rcvr>(rcvr)),
                  own_env_{[](auto &rcvr) constexpr noexcept {
                      if constexpr (requires() { own_env_t{queries::get_env(rcvr)}; })
                          return own_env_t{queries::get_env(rcvr)};
                      else if constexpr (requires() { own_env_t{}; })
                          return own_env_t{};
                      else
                          static_assert(false, "the own_env_t is ill-formed");
                  }(rcvr_)}
            {
                h.promise().environment_ = [](auto &rcvr,
                                              auto &own_env) constexpr noexcept {
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
                }(rcvr_, own_env_);
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
                static_assert(
                    std::is_same_v<decltype(prom), std::coroutine_handle<promise_type>>);
                prom.state_ = this;

                if constexpr (requires {
                                  prom.scheduler_ = scheduler_type{
                                      queries::get_scheduler(queries::get_env(rcvr_))};
                              })
                    prom.scheduler_ =
                        scheduler_type{queries::get_scheduler(queries::get_env(rcvr_))};
                else if constexpr (requires { prom.scheduler_ = scheduler_type{}; })
                    prom.scheduler_ = scheduler_type{};
                else
                    static_assert(false, "the scheduler_type is ill-formed");

                on_stop_.emplace(
                    std::move(queries::get_stop_token(queries::get_env(rcvr_))),
                    forward_stop_request{prom.source_});
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

            stop_callback on_stop_; // NOLINT
#if 0
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
#endif
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
            // NOTE: 最后的类型全部已知的机会。 state<R> 和 promise_type 是相关联的
            // NOTE: promise_type 目前的 get_env() 依赖 R 的构建。
            // NOTE: 如何让两个类型有机组合像一个类一样？
            assert(bool(handle_)); // Preconditions: bool(handle) is true.
            return {std::exchange(handle_, {}), std::forward<R>(recv)};
        }

      private:
        std::coroutine_handle<promise_type> handle_; // exposition only
    };

}; // namespace mcs::execution::task_v2