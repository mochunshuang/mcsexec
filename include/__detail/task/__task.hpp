#pragma once

#include <coroutine>

#include "./__pomise_base.hpp"
#include "./__task_value_sig.hpp"
#include "../awaitables/__as_awaitable.hpp"

namespace mcs::execution::__task
{

    // NOTE: sndr make operation_state_task by conn::connect(...)
    // NOTE: operation_state_task body defined by connect_awaitable(...)
    // NOTE: operation_state_task handle it operation of co_await std::move(sndr);
    // NOTE: std::move(sndr) -> await_transform -> [ task<T> ] -> task::await_transform
    // NOTE: -> sender_awaitable -> await_suspend(task::handle) -> start and resume
    // NOTE: awaitable_receiver make continue resume()
    template <typename T = void>
    struct task
    {
        struct state_base // NOLINT
        {
            using result_type = typename pomise_base<std::remove_cvref_t<T>>::result_type;
            using complete_callback_type = void(state_base *base,
                                                result_type &variant_result) noexcept;
            complete_callback_type *complete{nullptr};
        };

        struct promise_type : pomise_base<std::remove_cvref_t<T>>
        {
            using result_type = pomise_base<std::remove_cvref_t<T>>::result_type;

            struct final_awaiter
            {
                promise_type *promise;                       // NOLINT
                static constexpr bool await_ready() noexcept // NOLINT
                {
                    return false;
                }
                void await_suspend(std::coroutine_handle<> /*unused*/) noexcept // NOLINT
                {
                    // NOTE: this promise operation last
                    promise->state->complete(promise->state, promise->result);
                }
                static constexpr void await_resume() noexcept {} // NOLINT
            };
            task get_return_object() noexcept // NOLINT
            {
                return task{std::coroutine_handle<promise_type>::from_promise(*this)};
            }
            static constexpr std::suspend_always initial_suspend() noexcept // NOLINT
            {
                return {};
            }
            constexpr final_awaiter final_suspend() noexcept // NOLINT
            {
                return {this};
            }
            void unhandled_exception() noexcept // NOLINT
            {
                // NOTE: 不能手动完成
                this->result.template emplace<std::exception_ptr>(
                    std::current_exception());
            }

            // NOTE: 变成 sender_awaitable 或者 它本身。
            // NOTE: 不要求是 Sndr 更通用
            // NOTE: 核心是理解这里。 将当前协程注入到 函数体传递是Sndr 中
            // NOTE: 其他协程如何让这stop? 依赖 unhandled_exception()
            template <typename Expr>
            auto await_transform(Expr &&expr) noexcept // NOLINT
            {
                // TODO(mcs): 调度器线程和当前线程不能够一样，否则死锁
                return awaitables::as_awaitable(std::forward<Expr>(expr), *this);
            }

            // unhandled_stopped => awaitable_sender
            std::coroutine_handle<> unhandled_stopped() // NOLINT
            {
                this->state->complete(this->state, this->result);
                return std::noop_coroutine();
            }

            struct env
            {
                const promise_type *promise; // NOLINT

                [[nodiscard]] snd::default_domain query(
                    queries::get_domain_t /*unused*/) const noexcept
                {
                    return {};
                }
            };

            [[nodiscard]] auto get_env() const noexcept -> queryable auto // NOLINT
            {
                return env{this};
            }

            state_base *state{}; // NOLINT
        };

        template <typename Rcvr>
        struct state : state_base
        {
            state(Rcvr &&r, std::coroutine_handle<promise_type> h) noexcept
                : state_base{.complete = &complete_impl}, rcvr{std::forward<Rcvr>(r)},
                  handle{h}
            {
                handle.promise().state = this;
            }
            ~state() noexcept
            {
                if (this->handle)
                {
                    this->handle.destroy();
                }
            }
            state(const state &) = delete;
            state(state &&) = delete;
            state &operator=(const state &) = delete;
            state &operator=(state &&) = delete;

            static constexpr void complete_impl( // NOLINT
                state_base *base, state_base::result_type &variant_result) noexcept
            {
                // NOTE: 正常路径是由 final_awaiter 设置
                auto *self = static_cast<state *>(base);
                switch (variant_result.index())
                {
                case 0: // set_stopped
                    recv::set_stopped(std::move(self->rcvr));
                    break;
                case 1: // set_value
                    if constexpr (std::same_as<void, T>)
                    {
                        recv::set_value(std::move(self->rcvr));
                    }
                    else
                    {
                        auto r(std::move(std::get<1>(variant_result)));
                        recv::set_value(std::move(self->rcvr), std::move(r));
                    }
                    break;
                case 2: // NOTE: tow type error //TODO(mcs) 如果多个，则无法解决
                    recv::set_error(std::move(self->rcvr), std::get<2>(variant_result));
                    break;
                case 3:
                    recv::set_error(std::move(self->rcvr), std::get<3>(variant_result));
                    break;
                }
            }

            // NOTE: operation_concept
            using operation_state_concept = operation_state_t;
            void start() & noexcept
            {
                handle.resume();
            }

            Rcvr rcvr;                                           // NOLINT
            std::coroutine_handle<promise_type> handle{nullptr}; // NOLINT
        };
        task(const task &) = delete;
        task &operator=(const task &) = delete;
        task &operator=(task &&) = delete;
        explicit task(std::coroutine_handle<promise_type> h) noexcept : handle_{h} {}
        ~task() noexcept
        {
            if (this->handle_)
                this->handle_.destroy();
        }
        task(task &&other) noexcept : handle_(std::exchange(other.handle_, nullptr)) {};

        // NOTE: sender: std::error_code 待续
        using sender_concept = sender_t;
        using completion_signatures = cmplsigs::completion_signatures<
            typename task_value_sig<T>::type, set_error_t(std::exception_ptr),
            set_error_t(std::error_code), set_stopped_t()>;
        template <recv::receiver Rcvr>
        state<Rcvr> connect(Rcvr rcvr) noexcept
        {
            return state<Rcvr>(std::forward<Rcvr>(rcvr),
                               std::exchange(this->handle_, {}));
        }

      private:
        std::coroutine_handle<promise_type> handle_{}; // NOLINT
    };

}; // namespace mcs::execution::__task