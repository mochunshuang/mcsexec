#pragma once

#include <cassert>
#include <memory>

#include "../__stop_source.hpp"

namespace mcs::execution::stoptoken
{
    // 0. Effects: Initialises stop-state with a pointer to a new stop state.
    inline stop_source::stop_source() : stop_state(std::make_shared<unspecified>())
    {
        // 1. Postconditions: stop_possible() is true and stop_requested() is false
        assert(stop_possible());
        assert(not stop_requested());
        // 2. Throws: bad_alloc if memory cannot be allocated for the stop state.
    }

    inline void stop_source::swap(stop_source &rhs) noexcept
    {
        stop_state.swap(rhs.stop_state);
    }

    // Returns:
    //  stop_token() if stop_possible() is false;
    //  otherwise a new associated stop_token object ;
    //      i.e., its stop-state member is equal to the stop-state member of *this
    inline stop_token stop_source::get_token() const noexcept
    {
        if (not stop_possible())
        {
            return {};
        }
        return stop_token(stop_state);
    }

    // Returns: stop-state != nullptr .
    inline bool stop_source::stop_possible() const noexcept
    {
        return stop_state != nullptr;
    }

    // Returns:
    // true if stop-state refers to a stop state that has received a stop request;
    // otherwise, false.
    inline bool stop_source::stop_requested() const noexcept
    {
        return stop_state != nullptr && stop_state->stop_requested.load();
    }

    // Effects: Executes a stop request operation ([stoptoken.concepts]) on the
    // associated stop state
    inline bool stop_source::request_stop() noexcept
    {
        if (stop_state)
        {
            stop_state->stop_requested.store(true);
            return true;
        }
        return false;
    }

}; // namespace mcs::execution::stoptoken