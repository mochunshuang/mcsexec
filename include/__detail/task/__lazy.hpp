#pragma once

#include "./__allocator.hpp"
#include "./__inline_scheduler.hpp"
#include "./__scheduler_of.hpp"
#include "./__stop_source.hpp"
#include "../queries/__get_allocator.hpp"

#include "../awaitables/__as_awaitable.hpp"
#include "../adapt/__continues_on.hpp"

#include <optional>
#include <variant>

namespace mcs::execution::__task
{

    template <std::size_t Start, typename Fun, typename Var, std::size_t... I>
    void sub_visit_helper(Fun &fun, Var &var, std::index_sequence<I...> /*unused*/)
    {
        using thunk_t = void (*)(Fun &, Var &); // NOLINTNEXTLINE
        static constexpr thunk_t thunks[]{
            (+[](Fun &f, Var &v) { f(std::get<Start + I>(v)); })...};
        thunks[var.index() - Start](fun, var);
    }

    template <std::size_t Start, typename... T>
    void sub_visit(auto &&fun, std::variant<T...> &v)
    {
        if (v.index() < Start)
            return;
        sub_visit_helper<Start>(fun, v, std::make_index_sequence<sizeof...(T) - Start>{});
    }

    template <typename Awaiter>
    concept awaiter = snd::sender<Awaiter> && requires(Awaiter &&awaiter) {
        { awaiter.await_ready() } -> std::same_as<bool>;
        awaiter.disabled(); // remove this to get an awaiter unfriendly coroutine
    };

    template <typename E>
    struct with_error
    {
        E error;

        // the members below are only need for co_await with_error{...}
        static constexpr bool await_ready() noexcept // NOLINT
        {
            return false;
        }
        template <typename Promise>
            requires requires(Promise p, E e) {
                p.result.template emplace<E>(std::move(e));
                p.state->complete(p.result);
            }
        void await_suspend(std::coroutine_handle<Promise> handle) noexcept( // NOLINT
            noexcept(handle.promise().result.template emplace<E>(std::move(this->error))))
        {
            handle.promise().result.template emplace<E>(std::move(this->error));
            handle.promise().state->complete(handle.promise().result);
        }
        static constexpr void await_resume() noexcept {} // NOLINT
    };
    template <typename E>
    with_error(E &&) -> with_error<std::remove_cvref_t<E>>;

    struct default_context
    {
    };

    template <typename R>
    struct lazy_completion
    {
        using type = set_value_t(R);
    };
    template <>
    struct lazy_completion<void>
    {
        using type = set_value_t();
    };

    template <typename R>
    struct lazy_promise_base
    {
        using type = std::remove_cvref_t<R>;
        using result_t =
            std::variant<std::monostate, type, std::exception_ptr, std::error_code>;
        result_t result;
        template <typename T>
        void return_value(T &&value) // NOLINT
        {
            this->result.template emplace<type>(std::forward<T>(value));
        }
        template <typename E>
        void return_value(with_error<E> with) // NOLINT
        {
            this->result.template emplace<E>(with.error);
        }
    };
    template <>
    struct lazy_promise_base<void>
    {
        struct void_t
        {
        };
        using result_t =
            std::variant<std::monostate, void_t, std::exception_ptr, std::error_code>;
        result_t result;   // NOLINT
        void return_void() // NOLINT
        {
            this->result.template emplace<void_t>(void_t{});
        }
    };

    template <typename T = void, typename C = default_context>
    struct lazy
    {
        using allocator_type = allocator_of_t<C>;
        using scheduler_type = scheduler_of_t<C>;
        using stop_source_type = stop_source_of_t<C>;
        using stop_token_type = decltype(std::declval<stop_source_type>().get_token());

        using sender_concept = sender_t;
        using completion_signatures = cmplsigs::completion_signatures<
            typename lazy_completion<T>::type, set_error_t(std::exception_ptr),
            set_error_t(std::error_code), set_stopped_t()>;

        struct state_base // NOLINT
        {
            virtual void complete(
                typename lazy_promise_base<std::remove_cvref_t<T>>::result_t &) = 0;
            virtual stop_token_type get_stop_token() = 0; // NOLINT
            virtual C &get_context() = 0;                 // NOLINT

          protected:
            virtual ~state_base() = default;
        };

        struct promise_type : lazy_promise_base<std::remove_cvref_t<T>>
        {
            template <typename... A>
            void *operator new(std::size_t size, const A &...a)
            {
                return coroutine_allocate<C>(size, a...);
            }
            void operator delete(void *ptr, std::size_t size)
            {
                return coroutine_deallocate<C>(ptr, size);
            }

            template <typename... A>
            explicit promise_type(const A &...a)
                : allocator(find_allocator<allocator_type>(a...))
            {
            }

            struct final_awaiter
            {
                promise_type *promise;                       // NOLINT
                static constexpr bool await_ready() noexcept // NOLINT
                {
                    return false;
                }
                void await_suspend(std::coroutine_handle<> /*unused*/) noexcept // NOLINT
                {
                    promise->state->complete(promise->result);
                }
                static constexpr void await_resume() noexcept {} // NOLINT
            };

            std::suspend_always constexpr initial_suspend() noexcept // NOLINT
            {
                return {};
            }
            final_awaiter constexpr final_suspend() noexcept // NOLINT
            {
                return {this};
            }
            void unhandled_exception() // NOLINT
            {
                this->result.template emplace<std::exception_ptr>(
                    std::current_exception());
            }
            lazy get_return_object() // NOLINT
            {
                return lazy{std::coroutine_handle<promise_type>::from_promise(*this)};
            }

            template <typename E>
            auto await_transform(with_error<E> with) noexcept // NOLINT
            {
                return std::move(with);
            }
            template <snd::sender Sender>
            auto await_transform(Sender &&sndr) noexcept // NOLINT
            {
                if constexpr (std::same_as<inline_scheduler, scheduler_type>)
                    return awaitables::as_awaitable(std::forward<Sender>(sndr), *this);
                else
                    return awaitables::as_awaitable(
                        adapt::continues_on(std::forward<Sender>(sndr),
                                            *(this->scheduler)),
                        *this);
            }
            template <awaiter Awaiter>
            auto await_transform(Awaiter &&) noexcept = delete;

            template <typename E>
            final_awaiter yield_value(with_error<E> with) noexcept // NOLINT
            {
                this->result.template emplace<E>(with.error);
                return {this};
            }
#if defined(_MSC_VER)
#define NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif
            NO_UNIQUE_ADDRESS allocator_type allocator; // NOLINT
            std::optional<scheduler_type> scheduler{};  // NOLINT
            state_base *state{};                        // NOLINT

            std::coroutine_handle<> unhandled_stopped() // NOLINT
            {
                this->state->complete(this->result);
                return std::noop_coroutine();
            }

            struct env
            {
                const promise_type *promise; // NOLINT

                scheduler_type query(queries::get_scheduler_t /*unused*/) const noexcept
                {
                    return *promise->scheduler;
                }
                allocator_type query(queries::get_allocator_t /*unused*/) const noexcept
                {
                    return promise->allocator;
                }
                stop_token_type query(queries::get_stop_token_t /*unused*/) const noexcept
                {
                    return promise->state->get_stop_token();
                }
                snd::default_domain query(queries::get_domain_t /*unused*/) const noexcept
                {
                    return {};
                }
                template <typename Q, typename... A>
                    requires requires(const C &c, Q q, A &&...a) {
                        q(c, std::forward<A>(a)...);
                    }
                auto query(Q q, A &&...a) const noexcept
                {
                    return q(promise->state->get_context(), std::forward<A>(a)...);
                }
            };

            auto get_env() const noexcept -> queryable auto // NOLINT
            {
                return env{this};
            }
        };

        template <typename Receiver>
        struct state_rep
        {
            std::remove_cvref_t<Receiver> receiver;
            C context;
            template <typename R>
            explicit state_rep(R &&r) : receiver(std::forward<R>(r)), context() // NOLINT
            {
            }
        };
        template <typename Receiver>
            requires requires {
                C(queries::get_env(std::declval<std::remove_cvref_t<Receiver> &>()));
            } && (not requires(const Receiver &receiver) {
                         typename C::template env_type<decltype(queries::get_env(
                             receiver))>;
                     })
        struct state_rep<Receiver>
        {
            std::remove_cvref_t<Receiver> receiver;
            C context;
            template <typename R>
            explicit state_rep(R &&r) // NOLINT
                : receiver(std::forward<R>(r)), context(queries::get_env(this->receiver))
            {
            }
        };
        template <typename Receiver>
            requires requires(const Receiver &receiver) {
                typename C::template env_type<decltype(queries::get_env(receiver))>;
            }
        struct state_rep<Receiver>
        {
            using upstream_env = decltype(queries::get_env(
                std::declval<std::remove_cvref_t<Receiver> &>()));
            std::remove_cvref_t<Receiver> receiver;
            typename C::template env_type<upstream_env> own_env;
            C context;
            template <typename R>
            explicit state_rep(R &&r) // NOLINT
                : receiver(std::forward<R>(r)), own_env(queries::get_env(this->receiver)),
                  context(this->own_env)
            {
            }
        };

        template <typename Receiver>
        struct state : state_base, state_rep<Receiver> // NOLINT
        {
            using operation_state_concept = operation_state_t;
            using stop_token_t = decltype(queries::get_stop_token(
                queries::get_env(std::declval<Receiver>())));
            struct stop_link
            {
                stop_source_type &source; // NOLINT
                void operator()() const noexcept
                {
                    source.request_stop();
                }
            };
            using stop_callback_t = stop_callback_for_t<stop_token_t, stop_link>;
            template <typename R, typename H>
            state(R &&r, H h)
                : state_rep<Receiver>(std::forward<R>(r)), handle(std::move(h))
            {
            }
            ~state()
            {
                if (this->handle)
                {
                    this->handle.destroy();
                }
            }
            std::coroutine_handle<promise_type> handle;   // NOLINT
            stop_source_type source;                      // NOLINT
            std::optional<stop_callback_t> stop_callback; // NOLINT

            void start() & noexcept
            {
                handle.promise().scheduler.emplace(
                    queries::get_scheduler(queries::get_env(this->receiver)));
                handle.promise().state = this;
                handle.resume();
            }
            void complete(typename lazy_promise_base<std::remove_cvref_t<T>>::result_t
                              &result) override
            {
                switch (result.index())
                {
                case 0: // set_stopped
                    this->reset_handle();
                    recv::set_stopped(std::move(this->receiver));
                    break;
                case 1: // set_value
                    if constexpr (std::same_as<void, T>)
                    {
                        reset_handle();
                        recv::set_value(std::move(this->receiver));
                    }
                    else
                    {
                        auto r(std::move(std::get<1>(result)));
                        this->reset_handle();
                        recv::set_value(std::move(this->receiver), std::move(r));
                    }
                    break;
                default:
                    sub_visit<2>(
                        [this](auto &&r) {
                            this->reset_handle();
                            recv::set_error(std::move(this->receiver),
                                            std::forward<decltype(r)>(r));
                        },
                        result);
                    break;
                }
            }
            stop_token_type get_stop_token() override
            {
                if (this->source.stop_possible() && not this->stop_callback)
                {
                    this->stop_callback.emplace(
                        queries::get_stop_token(queries::get_env(this->receiver)),
                        stop_link{this->source});
                }
                return this->source.get_token();
            }
            C &get_context() override
            {
                return this->context;
            }
            void reset_handle() // NOLINT
            {
                this->handle.destroy();
                this->handle = {};
            }
        };

        std::coroutine_handle<promise_type> handle; // NOLINT

        auto get_env() const noexcept // NOLINT
        {
            return handle.promise().get_env();
        };

      private:
        explicit lazy(std::coroutine_handle<promise_type> h) : handle(std::move(h)) {}

      public:
        lazy(const lazy &other) = delete;
        lazy(lazy &&other) noexcept : handle(std::exchange(other.handle, {})) {}
        ~lazy()
        {
            if (this->handle)
            {
                this->handle.destroy();
            }
        }
        lazy &operator=(const lazy &) = delete;
        lazy &operator=(lazy &&) = delete;

        template <recv::receiver Receiver>
        state<Receiver> connect(Receiver receiver) noexcept
        {
            return state<Receiver>(std::forward<Receiver>(receiver),
                                   std::exchange(this->handle, {}));
        }
    };
}; // namespace mcs::execution::__task