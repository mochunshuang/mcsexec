#pragma once
#include "./../__stop_token.hpp"

namespace mcs::execution::stoptoken
{

    // Note: the stop_state init by [stop_source]
    // stop-state refers to the stop_token's associated stop state.
    // A stop_token object is disengaged when stop-state is empty.
    inline stop_token::stop_token(const std::shared_ptr<unspecified> &init_state) noexcept
        : stop_state(init_state)
    {
    }

    // [stoptoken.mem], Member functions
    // Equivalent to: stop_state.swap(rhs.stop_state)
    inline void stop_token::swap(stop_token &rhs) noexcept
    {
        stop_state.swap(rhs.stop_state);
    }

    // true if this stop_token has received a stop request; otherwise, false.
    inline bool stop_token::stop_requested() const noexcept
    {
        if (stop_state != nullptr)
            return stop_state->stop_requested.load();

        return false;
    }

    // false if *this is disengaged ,
    //      or a stop request was not made and there are no associated stop_source objects
    //  otherwise, true.
    inline bool stop_token::stop_possible() const noexcept
    {
        // *this is disengaged
        if (stop_state == nullptr)
        {
            return false;
        }
        // a stop request was not made and there are no associated stop_source objects
        if (not stop_state->stop_requested && stop_state->sources == 0)
        {
            return false;
        }

        return true;
    }



}; // namespace mcs::execution::stoptoken