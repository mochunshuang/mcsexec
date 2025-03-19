#pragma once

#include <optional>
#include <system_error>
#include "../__stoptoken/__inplace_stop_token.hpp"
#include "./__detail/__poly.hpp"

#include "../queries/__get_stop_token.hpp"
#include "../recv/__receiver.hpp"
#include "../snd/__sender.hpp"
#include "../conn/__connect.hpp"

namespace mcs::execution::task
{
    /**
     * @brief Type-erasing scheduler
     * The class `any_scheduler` is used to type-erase any scheduler class.
     * Any error produced by the underlying scheduler except `std::error_code` is turned
     * into an `std::exception_ptr`. `std::error_code` is forwarded as is. The
     * `any_scheduler` forwards stop requests reported by the stop token obtained from the
     * `connect`ed receiver to the sender used by the underlying scheduler.
     *
     * Completion signatures:
     *
     * - `ex::set_value_t()`
     * - `ex::set_error_t(std::error_code)`
     * - `ex::set_error_t(std::exception_ptr)`
     * - `ex::set_stopped()`
     *
     * Usage:
     *
     *     any_scheduler sched(other_scheduler);
     *     auto sender{ex::schedule(sched) | some_sender};
     */
    class any_scheduler // NOLINT
    {
        struct state_base // NOLINT
        {
            virtual ~state_base() = default;
            virtual void complete_value() = 0;                          // NOLINT
            virtual void complete_error(::std::error_code) = 0;         // NOLINT
            virtual void complete_error(::std::exception_ptr) = 0;      // NOLINT
            virtual void complete_stopped() = 0;                        // NOLINT
            virtual stoptoken::inplace_stop_token get_stop_token() = 0; // NOLINT
        };

        struct inner_state
        {
            struct receiver;
            struct env
            {
                state_base *state; // NOLINT
                [[nodiscard]] auto query(
                    queries::get_stop_token_t /*unused*/) const noexcept
                {
                    return this->state->get_stop_token();
                }
            };
            struct receiver
            {
                using receiver_concept = receiver_t;
                state_base *state;           // NOLINT
                void set_value() && noexcept // NOLINT
                {
                    this->state->complete_value();
                }
                void set_error(std::error_code err) && noexcept // NOLINT
                {
                    this->state->complete_error(err);
                }
                void set_error(std::exception_ptr ptr) && noexcept // NOLINT
                {
                    this->state->complete_error(std::move(ptr));
                }
                template <typename E>
                void set_error(E e) && noexcept // NOLINT
                {
                    this->state->complete_error(std::make_exception_ptr(std::move(e)));
                }
                void set_stopped() && noexcept // NOLINT
                {
                    this->state->complete_stopped();
                }
                [[nodiscard]] env get_env() const noexcept // NOLINT
                {
                    return {this->state};
                }
            };
            static_assert(recv::receiver<receiver>);

            struct base // NOLINT
            {
                virtual ~base() = default;
                virtual void start() = 0;
            };
            template <snd::sender Sender>
            struct concrete : base
            {
                using state_t = decltype(conn::connect(std::declval<Sender>(),
                                                       std::declval<receiver>()));
                state_t state; // NOLINT
                template <snd::sender S>
                concrete(S &&s, state_base *b)
                    : state(connect(std::forward<S>(s), receiver{b}))
                {
                }
                void start() override
                {
                    opstate::start(state);
                }
            };
            __detail::poly<base, 16U * sizeof(void *)> state; // NOLINT
            template <snd::sender S>
            inner_state(S &&s, state_base *b)
                : state(static_cast<concrete<S> *>(nullptr), std::forward<S>(s), b)
            {
            }
            void start()
            {
                this->state->start();
            }
        };

        template <receiver Receiver>
        struct state : state_base
        {
            using operation_state_concept = operation_state_t;
            struct stopper
            {
                state *st; // NOLINT
                void operator()() noexcept
                {
                    state *self = this->st;
                    self->callback.reset();
                    self->source.request_stop();
                }
            };
            using token_t =
                decltype(queries::get_stop_token(get_env(std::declval<Receiver>())));
            using callback_t = stoptoken::stop_callback_for_t<token_t, stopper>;

            std::remove_cvref_t<Receiver> receiver; // NOLINT
            inner_state s;                          // NOLINT
            stoptoken::inplace_stop_source source;  // NOLINT
            std::optional<callback_t> callback;     // NOLINT

            template <recv::receiver R, typename PS>
            state(R &&r, PS &ps) : receiver(std::forward<R>(r)), s(ps->connect(this))
            {
            }
            void start() & noexcept
            {
                this->s.start();
            }
            void complete_value() override
            {
                recv::set_value(std::move(this->receiver));
            }
            void complete_error(std::error_code err) override
            {
                recv::set_error(std::move(receiver), err);
            }
            void complete_error(std::exception_ptr ptr) override
            {
                recv::set_error(std::move(receiver), std::move(ptr));
            }
            void complete_stopped() override
            {
                recv::set_stopped(std::move(this->receiver));
            }
            stoptoken::inplace_stop_token get_stop_token() override
            {
                if constexpr (::std::same_as<token_t, stoptoken::inplace_stop_token>)
                {
                    return queries::get_stop_token(queries::get_env(this->receiver));
                }
                else
                {
                    if constexpr (not::std::same_as<token_t, never_stop_token>)
                    {
                        if (not this->callback)
                        {
                            this->callback.emplace(
                                queries::get_stop_token(get_env(this->receiver)),
                                stopper{this});
                        }
                    }
                    return this->source.get_token();
                }
            }
        };

        class sender; // NOLINT
        class env     // NOLINT
        {
            friend class sender;

          private:
            const sender *sndr; // NOLINT
            explicit env(const sender *s) : sndr(s) {}

          public:
            [[nodiscard]] any_scheduler query(
                const queries::get_completion_scheduler_t<set_value_t> & /*unused*/)
                const noexcept
            {
                return this->sndr->inner_sender->get_completion_scheduler();
            }
        };

        // sender implementation
        class sender // NOLINT
        {
            friend class env;

          private:
            struct base // NOLINT
            {
                virtual ~base() = default;
                virtual base *move(void *) = 0;
                virtual base *clone(void *) const = 0;
                virtual inner_state connect(state_base *) noexcept = 0; // NOLINTNEXTLINE
                [[nodiscard]] virtual any_scheduler get_completion_scheduler() const = 0;
            };
            template <sched::scheduler Scheduler>
            struct concrete : base
            {
                using sender_t = decltype(factories::schedule(std::declval<Scheduler>()));
                sender_t sender; // NOLINT

                template <sched::scheduler S>
                explicit concrete(S &&s) : sender(factories::schedule(std::forward<S>(s)))
                {
                }
                base *move(void *buffer) override
                {
                    return new (buffer) concrete(std::move(*this));
                }
                base *clone(void *buffer) const override
                {
                    return new (buffer) concrete(*this);
                }
                inner_state connect(state_base *b) noexcept override
                {
                    return inner_state(::std::move(sender), b);
                }
                [[nodiscard]] any_scheduler get_completion_scheduler() const override
                {
                    return any_scheduler(queries::get_completion_scheduler<set_value_t>(
                        queries::get_env(this->sender)));
                }
            };
            __detail::poly<base, 4 * sizeof(void *)> inner_sender; // NOLINT

          public:
            using sender_concept = sender_t;
            using completion_signatures = cmplsigs::completion_signatures<
                set_value_t(), set_error_t(std::error_code),
                set_error_t(std::exception_ptr), set_stopped_t()>;

            template <sched::scheduler S>
            explicit sender(S &&s)
                : inner_sender(static_cast<concrete<S> *>(nullptr), std::forward<S>(s))
            {
            }
            sender(sender &&) = default;
            sender(const sender &) = default;

            template <receiver R>
            state<R> connect(R &&r) noexcept
            {
                return state<R>(std::forward<R>(r), this->inner_sender);
            }

            [[nodiscard]] env get_env() const noexcept // NOLINT
            {
                return env(this);
            }
        };

        // scheduler implementation
        struct base // NOLINT
        {
            virtual ~base() = default;
            virtual sender schedule() = 0;
            virtual base *move(void *buffer) = 0;
            virtual base *clone(void *) const = 0;
            virtual bool equals(const base *) const = 0;
        };
        template <sched::scheduler Scheduler>
        struct concrete : base
        {
            Scheduler scheduler; // NOLINT
            template <sched::scheduler S>
            explicit concrete(S &&s) : scheduler(std::forward<S>(s))
            {
            }
            sender schedule() override
            {
                return sender(this->scheduler);
            }
            base *move(void *buffer) override
            {
                return new (buffer) concrete(std::move(*this));
            }
            base *clone(void *buffer) const override
            {
                return new (buffer) concrete(*this);
            }
            bool equals(const base *o) const override
            {
                auto other{dynamic_cast<const concrete *>(o)};
                return other ? this->scheduler == other->scheduler : false;
            }
        };

        __detail::poly<base, 4 * sizeof(void *)> scheduler; // NOLINT

      public:
        using scheduler_concept = scheduler_t;

        template <typename S>
            requires(not std::same_as<any_scheduler, std::remove_cvref_t<S>>)
        explicit any_scheduler(S &&s)
            : scheduler(static_cast<concrete<std::decay_t<S>> *>(nullptr),
                        std::forward<S>(s))
        {
        }

        sender schedule()
        {
            return this->scheduler->schedule();
        }
        bool operator==(const any_scheduler &) const = default;
    };
    static_assert(sched::scheduler<any_scheduler>);

}; // namespace mcs::execution::task