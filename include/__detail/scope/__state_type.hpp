#pragma once

#include <cstdint>

namespace mcs::execution::scope
{

    /**
     * @brief A counting_scope maintains a count of outstanding operations.
     * Let s be an object of type counting_scope,
     * t be an object of type counting_scope::token obtained from s.get_token()
     * let j be a sender obtained from s.join(), and
     * le to be an operation state obtained from connecting j to a receiver.
     * During its life-time s goes through different states which govern what
     * operations are allowed and the result of these operations:
     *
     */
    enum state_type : std::uint8_t // NOLINTBEGIN
    {
        // a newly constructed object starts in the unused state.
        unused,
        // when t.try_associate() is called while s is in unused state, s moves to
        // the open state.
        open,
        // when the operation state o is started while the s is in unused or open
        // state, s moves to the open-and-joining state.
        open_and_joining,
        // when s.close() is called while s is in open state, s moves to the
        // closed state.
        closed,
        // when s.close() is called while s is in unused state, s moves to the
        // unused-and-closed state.
        unused_and_closed,
        // when s.close() is called while s is in open-and-joining state or the
        // operation state o is started while s is in closed or unused-and-closed
        // state, s moves to the closed-and-joining state.
        closed_and_joining,
        // when the count of associated objects drops to zero while s is in
        // open-and-joining or closed-and-joining state, s moves to the joined
        // state.
        joined,
    }; // NOLINTEND

}; // namespace mcs::execution::scope