#pragma once
#include "./__declarations.hpp"

namespace mcs::execution::stoptoken
{
    // [stoptoken.never], class never_stop_token
    class never_stop_token // NOLINT
    {
      private:
        struct __callback_type
        {
            explicit __callback_type(never_stop_token /*unused*/,
                                     auto && /*unused*/) noexcept
            {
            }
        };

      public:
        template <class>
        using callback_type = __callback_type;

        static constexpr auto stop_requested() noexcept -> bool // NOLINT
        {
            return false;
        }

        static constexpr auto stop_possible() noexcept -> bool // NOLINT
        {
            return false;
        }
        auto operator==(const never_stop_token &) const noexcept -> bool = default;
    };
}; // namespace mcs::execution::stoptoken