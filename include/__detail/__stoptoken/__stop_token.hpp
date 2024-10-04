#pragma once

#include <memory>

#include "./__invocable_destructible.hpp"
#include "./__stop_state.hpp"
#include "__stop_callback_base.hpp"
#include "__stop_source.hpp"

namespace mcs::execution::stoptoken
{
    // stop_callback Dependent on
    template <__detail::invocable_destructible CallbackFn>
    class stop_callback; // NOLINT

    // friend
    class stop_source;
    class stop_callback_base;

    // [stoptoken.general]
    // Note: The class stop_token models the concept [stoppable_token]
    class stop_token // NOLINT
    {
      public:
        template <class CallbackFn>
        using callback_type = stop_callback<CallbackFn>;

        stop_token() noexcept = default;

        void swap(stop_token &) noexcept;

        bool stop_requested() const noexcept; // NOLINT

        bool stop_possible() const noexcept; // NOLINT

        bool operator==(const stop_token &rhs) const noexcept = default;

      private:
        using unspecified = ::mcs::execution::stoptoken::stop_state;
        // stop-state refers to the stop_token's associated stop state.
        // A stop_token object is disengaged when stop-state is empty.
        std::shared_ptr<unspecified> stop_state{nullptr}; // NOLINT // exposition only

        friend ::mcs::execution::stoptoken::stop_source;
        friend ::mcs::execution::stoptoken::stop_callback_base;
        explicit stop_token(const std::shared_ptr<unspecified> &init_state) noexcept;
    };

}; // namespace mcs::execution::stoptoken