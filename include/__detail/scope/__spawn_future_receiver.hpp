#pragma once
#include "./__spawn_future_state_base.hpp"
#include "../recv/__set_value.hpp"
#include "../recv/__set_error.hpp"
#include "../recv/__set_stopped.hpp"

namespace mcs::execution::scope
{
    template <class Sigs>
    struct spawn_future_receiver
    {
        using receiver_concept = receiver_t;

        spawn_future_state_base<Sigs> *state; // NOLINT

        template <class... T>
        void set_value(T &&...t) && noexcept // NOLINT
        {
            constexpr bool nothrow = // NOLINT
                (std::is_nothrow_constructible_v<std::decay_t<T>, T> && ...);

            if constexpr (nothrow)
            {
                state->result.template emplace<decayed_tuple<set_value_t, T...>>(
                    recv::set_value_t{}, std::forward<T>(t)...);
            }
            else
            {
                try
                {
                    state->result.template emplace<decayed_tuple<set_value_t, T...>>(
                        recv::set_value_t{}, std::forward<T>(t)...);
                }
                catch (...)
                {
                    state->result
                        .template emplace<decayed_tuple<set_error_t, std::exception_ptr>>(
                            recv::set_error_t{}, std::current_exception());
                }
            }
            state->complete();
        }

        template <class E>
        void set_error(E &&e) && noexcept // NOLINT
        {
            // NOLINTNEXTLINE
            constexpr bool nothrow = std::is_nothrow_constructible_v<std::decay_t<E>, E>;

            if constexpr (nothrow)
            {
                state->result.template emplace<decayed_tuple<set_error_t, E>>(
                    recv::set_error_t{}, std::forward<E>(e));
            }
            else
            {
                try
                {
                    state->result.template emplace<decayed_tuple<set_error_t, E>>(
                        recv::set_error_t{}, std::forward<E>(e));
                }
                catch (...)
                {
                    state->result
                        .template emplace<decayed_tuple<set_error_t, std::exception_ptr>>(
                            recv::set_error_t{}, std::current_exception());
                }
            }

            state->complete();
        }

        void set_stopped() && noexcept // NOLINT
        {
            state->result.template emplace<decayed_tuple<set_stopped_t>>(
                recv::set_stopped_t{});
            state->complete();
        }
    };

}; // namespace mcs::execution::scope