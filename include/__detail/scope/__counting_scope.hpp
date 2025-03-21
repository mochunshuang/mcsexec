#pragma once

#include <algorithm>
#include <atomic>
#include <exception>

#include "../__stoptoken/__inplace_stop_source.hpp"
#include "../snd/__sender.hpp"
#include "../snd/__make_sender.hpp"
#include "../snd/general/__impls_for.hpp"

#include "../tool/SafeIntrusiveForwardList.hpp"

#include "./__scope_state.hpp"
#include "./__state_type.hpp"
#include "./__stop_when.hpp"

namespace mcs::execution
{
    namespace scope
    {
        struct counting_scope
        {
          public:
            // [exec.counting.token], token
            struct token
            {
                template <snd::sender Sndr>
                snd::sender auto wrap(Sndr &&sndr) noexcept
                {
                    return stop_when(std::forward<Sndr>(sndr),
                                     scope->s_source.get_token());
                }
                /**
                * @brief Effects: A invocation of this member function has the following
                atomic effect:
                (2.1)If scope->state is not one of unused, open, or open-and-joining the
                    operation has no effect;
                (2.2)otherwise increment scope->count and if scope->state == unused change
                this value to open.
                3 Returns: true if scope->count was incremented, false  otherwise.
                *
                * @return true
                * @return false
                */
                bool try_associate() const noexcept // NOLINT
                {
                    auto state = scope->state.load(std::memory_order_acquire);
                    if (state != unused && state != open && state != open_and_joining)
                    {
                        return false;
                    }
                    scope->count++;
                    if (state == unused)
                    {
                        scope->state.store(open, std::memory_order_release);
                    }
                    return true;
                }

                /**
                 * @brief Decrements scope->count. If scope->count is zero after
                 * decrementing and scope->state is open-and-joining or
                 * closed-and-joining, changes the state of *scope to joined and calls
                 * complete() on all objects registered with *scope.
                 *
                 */
                void disassociate() const
                {
                    if (scope->count.fetch_sub(1) == 1)
                    {
                        if (auto state = scope->state.load(std::memory_order_acquire);
                            state == open_and_joining || state == closed_and_joining)
                        {
                            scope->state.store(joined, std::memory_order_release);
                        }
                        while (!scope->registers.empty())
                        {
                            auto *op = scope->registers.front();
                            scope->registers.popFront();
                            op->execute(op);
                        }
                    }
                }

              private:
                counting_scope *scope; // exposition-only // NOLINT
                friend counting_scope;
                explicit token(counting_scope *s) : scope(s) {}
            };

            // [exec.counting.ctor], constructor and destructor
            counting_scope(counting_scope &&) = delete;
            counting_scope(const counting_scope &) = delete;
            counting_scope &operator=(counting_scope &&) = delete;
            counting_scope &operator=(const counting_scope &) = delete;

            // Postcondtions: count is 0 and state is unused
            counting_scope() noexcept = default;
            // Effects: If state is not one of joined, unused, or unused-and-closed,
            // invokes terminate (14.6.2 [except.terminate]). Otherwise, has no effects.
            ~counting_scope()
            {
                if (state != joined && state != unused && state != unused_and_closed)
                    std::terminate();
            }

            // NOTE: Calls to member functions get_token, close, join, and request_stop do
            // not introduce data races.
            //  [exec.counting.mem], members
            token get_token() noexcept // NOLINT
            {
                return token(this);
            }
            void close() noexcept
            {
                switch (state.load(std::memory_order_acquire))
                {
                case unused: // unused changes state to unused-and-closed;
                    state.store(unused_and_closed, std::memory_order_release);
                    break;
                case open: // open changes state to closed;
                    state.store(closed, std::memory_order_release);
                    break; // open-and-joining changes state to closed-and-joining;
                case open_and_joining:
                    state.store(closed_and_joining, std::memory_order_release);
                    break;
                default:
                    break;
                };
            }

            void request_stop() noexcept // NOLINT
            {
                s_source.request_stop();
            }

            struct join_t
            {

            }; // exposition-only
            snd::sender auto join() noexcept;

            std::atomic<state_type> state{unused};                      // NOLINT
            std::atomic<std::size_t> count{0};                          // NOLINT
            stoptoken::inplace_stop_source s_source;                    // NOLINT
            tool::SafeIntrusiveForwardList<scope_state_base> registers; // NOLINT
        };
    } // namespace scope

    template <>
    struct snd::general::impls_for<scope::counting_scope::join_t>
        : snd::__detail::default_impls
    {

        static constexpr auto get_state = // NOLINT
            []<class Receiver>(auto &&sender, Receiver &receiver) noexcept {
                auto [_, self] = sender;
                return scope::scope_state(self, receiver);
            };
        /**
         * @brief If state is
         * unused, unused-and-closed, or joined, s.complete-inline() is invoked and
         * changes the state of *s.scope to joined;
         * open, changes the state of *s.scope to open-and-joining;
         * closed, changes the state of *s.scope to closed-and-joining;
         * If s.complete-inline() was not invoked, registers s with *s.scope to have
         * s.complete() invoked when s.scope->count becomes zero.
         */
        static constexpr auto start = [](auto &s, auto &rcvr) noexcept { // NOLINT
            using enum scope::state_type;
            auto state = s.scope->state.load(std::memory_order_acquire);
            switch (state)
            {
            case unused:
            case unused_and_closed:
            case joined: {
                if (state != joined)
                    s.scope->state.store(joined, std::memory_order_release);
                s.complete_inline();
                return;
            }
            break;
            case open: {
                s.scope->state.store(open_and_joining, std::memory_order_release);
                if (s.scope->count.load(std::memory_order_acquire) == 0)
                {
                    // open -> open_and_joining -> joined
                    // NOTE: 修正的地方；直接转移
                    s.scope->state.store(joined, std::memory_order_release);
                    s.complete();
                    return;
                }
                s.scope->registers.pushFront(&s);
            }
            break;
            case closed: {
                s.scope->state.store(closed_and_joining, std::memory_order_release);
                if (s.scope->count.load(std::memory_order_acquire) == 0)
                {
                    // open -> closed_and_joining -> joined
                    // NOTE: 修正的地方；直接转移
                    s.scope->state.store(joined, std::memory_order_release);
                    s.complete();
                    return;
                }
                s.scope->registers.pushFront(&s);
            }
            break;
            default:
                break;
            }
        };
    };

    template <typename Scope, typename... Env>
    struct cmplsigs::completion_signatures_for_impl<
        snd::__detail::basic_sender<scope::counting_scope::join_t, Scope>, Env...>
    {
        using type = cmplsigs::completion_signatures<
            set_value_t(), set_error_t(std::exception_ptr), set_stopped_t()>;
    };

    inline snd::sender auto scope::counting_scope::join() noexcept
    {
        return snd::make_sender(join_t{}, this);
    }

}; // namespace mcs::execution