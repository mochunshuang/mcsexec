#pragma once

#include "../snd/__sender.hpp"
#include "../snd/__make_sender.hpp"
#include "../snd/general/__impls_for.hpp"
#include "../tool/SafeIntrusiveForwardList.hpp"
#include "../tool/CasSpinLock.hpp"
#include "../tool/ScopedLock.hpp"

#include "./__scope_state.hpp"
#include "./__state_type.hpp"

namespace mcs::execution
{

    namespace scope
    {
        struct simple_counting_scope // NOLINT
        {

            struct token
            {
                template <snd::sender Sender>
                Sender &&wrap(Sender &&snd) const noexcept
                {
                    return std::forward<Sender>(snd);
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
                    tool::ScopedLock lock(scope->spin_lock);
                    auto &state = scope->state;
                    // If scope->state is not one of unused, open, or open-and-joining
                    // the operation has no effect
                    if (state != unused && state != open && state != open_and_joining)
                    {
                        return false;
                    }
                    // otherwise increment scope->count and if scope->state == unused
                    // change this value to open.
                    scope->count++;
                    if (state == unused)
                    {
                        state = open;
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
                void disassociate() const noexcept
                {
                    tool::ScopedLock lock(scope->spin_lock);
                    if (1 == (scope->count--))
                    {
                        if (auto &state = scope->state;
                            state == open_and_joining || state == closed_and_joining)
                        {
                            state = joined;
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
                simple_counting_scope *scope; // exposition-only // NOLINT
                friend simple_counting_scope;
                explicit token(simple_counting_scope *s) : scope(s) {}
            };

            // simple_counting_scope is immovable and uncopyable
            simple_counting_scope(simple_counting_scope &&) = delete;
            simple_counting_scope(const simple_counting_scope &) = delete;
            simple_counting_scope &operator=(simple_counting_scope &&) = delete;
            simple_counting_scope &operator=(const simple_counting_scope &) = delete;

            // Postcondtions: count is 0 and state is unused is  default
            simple_counting_scope() noexcept = default;
            ~simple_counting_scope() noexcept
            {
                // If state is not one of joined, unused, or unused-and-closed, invokes
                // terminate Otherwise, has no effects.
                if (state != joined && state != unused && state != unused_and_closed)
                    std::terminate(); // TODO(mcs): terminate 没有有效的错误信息
            }

            // Returns: An object t of type simple_counting_scope::token such that
            // t.scope == this is true.
            token get_token() noexcept // NOLINT
            {
                return token(this);
            }
            /**
            * @brief Effects: If state is
            (2.1)unused changes state to unused-and-closed;
            (2.2)open changes state to closed;
            (2.3)open-and-joining changes state to closed-and-joining;
            *
            */
            void close() noexcept
            {
                tool::ScopedLock lock(spin_lock);
                switch (state)
                {
                case unused:
                    state = unused_and_closed;
                    break;
                case open:
                    state = closed;
                    break;
                case open_and_joining:
                    state = closed_and_joining;
                    break;
                default:
                    break;
                };
            }

            struct join_t
            {
            };
            snd::sender auto join() noexcept;
            state_type state{unused};                                   // NOLINT
            std::size_t count{0};                                       // NOLINT
            tool::SafeIntrusiveForwardList<scope_state_base> registers; // NOLINT
            tool::CasSpinLock spin_lock;                                // NOLINT
        };
    }; // namespace scope

    template <>
    struct snd::general::impls_for<scope::simple_counting_scope::join_t>
        : snd::__detail::default_impls
    {

        static constexpr auto get_state = // NOLINT
            []<class Receiver>(auto &&sender, Receiver &receiver) noexcept {
                auto [_, self] = sender;
                return scope::scope_state(self, receiver);
            };

        /**
         * @brief If state is
            (7.1)unused, unused-and-closed, or joined, s.complete-inline() is invoked and
            changes the state of *s.scope to joined;
            (7.2)open, changes the state of *s.scope to open-and-joining;
            (7.3)closed, changes the state of *s.scope to closed-and-joining;
            If s.complete-inline() was not invoked, registers s with *s.scope to have
            s.complete() invoked when s.scope->count becomes zero.
         *
         */
        static constexpr auto start = [](auto &s, auto &) noexcept { // NOLINT
            using enum scope::state_type;
            tool::ScopedLock lock(s.scope->spin_lock);
            auto &state = s.scope->state;
            auto &count = s.scope->count;
            switch (state)
            {
            case unused:
            case unused_and_closed:
            case joined: {
                if (state != joined)
                    state = joined;
                s.complete_inline();
                return;
            }
            break;
            case open: {
                state = open_and_joining;
                if (count == 0)
                {
                    // open -> open_and_joining -> joined
                    // NOTE: 修正的地方；直接转移
                    state = joined;
                    s.complete();
                    return;
                }
                s.scope->registers.pushFront(&s);
            }
            break;
            case closed: {
                state = closed_and_joining;
                if (count == 0)
                {
                    // open -> closed_and_joining -> joined
                    // NOTE: 修正的地方；直接转移
                    state = joined;
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
        snd::__detail::basic_sender<scope::simple_counting_scope::join_t, Scope>, Env...>
    {
        using type = cmplsigs::completion_signatures<set_value_t(), set_stopped_t()>;
    };

    inline snd::sender auto scope::simple_counting_scope::join() noexcept
    {
        return snd::make_sender(join_t{}, this);
    }

}; // namespace mcs::execution