#pragma once

#include "./__finite_inplace_stop_source.hpp"
#include "./__single_inplace_stop_token.hpp"

namespace mcs::execution::stoptoken
{

    // [stopsource.single], class single_inplace_stop_source
    class single_inplace_stop_source : private finite_inplace_stop_source<1> // NOLINT
    {
      public:
        single_inplace_stop_source() noexcept = default;

        bool request_stop() noexcept
        {
            return finite_inplace_stop_source<1>::request_stop();
        }
        bool stop_requested() noexcept
        {
            return finite_inplace_stop_source<1>::stop_requested();
        }

        single_inplace_stop_token get_token() const noexcept
        {
            return finite_inplace_stop_source<1>::get_token<0>();
        }
    };
}; // namespace mcs::execution::stoptoken