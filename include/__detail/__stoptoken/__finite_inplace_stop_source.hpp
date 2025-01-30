#pragma once
#include <array>
#include "./__finite_inplace_stop_source_base.hpp"

namespace mcs::execution::stoptoken
{
    template <std::size_t N, std::size_t Idx>
    class finite_inplace_stop_token;

    template <std::size_t N>
    class finite_inplace_stop_source : private finite_inplace_stop_source_base // NOLINT
    {
        static_assert(N > 0);

        template <std::size_t... Indices>
        explicit finite_inplace_stop_source(
            std::index_sequence<Indices...> /*unused*/) noexcept
            : states{((void)Indices, no_callback_state())...}
        {
        }

      public:
        finite_inplace_stop_source() noexcept
            : finite_inplace_stop_source(std::make_index_sequence<N>{})
        {
        }
        static constexpr bool stop_possible() noexcept // NOLINT
        {
            return true;
        }
        bool request_stop() noexcept // NOLINT
        {
            return finite_inplace_stop_source_base::request_stop_impl(N, states.data());
        }
        bool stop_requested() const noexcept // NOLINT
        {
            void *state = states[0].load(std::memory_order_acquire);
            return is_stop_requested_state(states.data(), state);
        }

        template <std::size_t Idx>
        finite_inplace_stop_token<N, Idx> get_token() const noexcept // NOLINT
        {
            return finite_inplace_stop_token<N, Idx>{this};
        }

      private:
        template <std::size_t, std::size_t, __detail::invocable_destructible>
        friend class finite_inplace_stop_callback;

        bool try_register_callback(std::size_t idx, callback_base *cb) const noexcept
        {
            return finite_inplace_stop_source_base::try_register_callback(
                N, states.data(), idx, cb);
        }
        void deregister_callback(std::size_t idx, callback_base *cb) const noexcept
        {
            finite_inplace_stop_source_base::deregister_callback(N, states.data(), idx,
                                                                 cb);
        }

        // nullptr                 - no stop-request or stop-callback
        // &states_                - stop-requested
        // &thread_requesting_stop - stop-requested, callback-done
        // other                   - pointer to callback_base
        mutable std::array<std::atomic<void *>, N> states; // NOLINT
    };

}; // namespace mcs::execution::stoptoken