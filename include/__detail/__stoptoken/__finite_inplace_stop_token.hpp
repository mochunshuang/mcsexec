#pragma once

#include "./__finite_inplace_stop_source.hpp"

namespace mcs::execution::stoptoken
{
    template <std::size_t N, std::size_t Idx, __detail::invocable_destructible CB>
    class finite_inplace_stop_callback;

    // [stoptoken.finite], class template finite_inplace_stop_token
    template <std::size_t N, std::size_t Idx>
    class finite_inplace_stop_token // NOLINT
    {
        static_assert(Idx < N, "Mandates: Idx < N is true.");

      public:
        template <typename CB>
        using callback_type = finite_inplace_stop_callback<N, Idx, CB>;

        finite_inplace_stop_token() noexcept = default;

        [[nodiscard]] bool stop_possible() const noexcept // NOLINT
        {
            return stop_source != nullptr;
        }
        // should call before the start of the destructor of the associated
        // finite_inplace_stop_source object else undefined
        [[nodiscard]] bool stop_requested() const noexcept // NOLINT
        {
            return stop_possible() && stop_source->stop_requested();
        }
        // Effects: Exchanges the values of stop-source and rhs.stop-source.
        void swap(finite_inplace_stop_token &rhs) noexcept
        {
            std::swap(stop_source, rhs.stop_source);
        }

        bool operator==(const finite_inplace_stop_token &a) const noexcept = default;

      private:
        friend finite_inplace_stop_source<N>;
        template <std::size_t, std::size_t, __detail::invocable_destructible CB>
        friend class finite_inplace_stop_callback;

        explicit finite_inplace_stop_token(
            const finite_inplace_stop_source<N> *source) noexcept
            : stop_source(source)
        {
        }

        const finite_inplace_stop_source<N> *stop_source; // exposition-only // NOLINT
    };
}; // namespace mcs::execution::stoptoken