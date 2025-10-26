#pragma once

namespace mcs::execution::task_v2::__detail
{
    struct state_base
    {
        using completion_type = void(state_base *) noexcept;
        completion_type *completion;
        completion_type *unhandled_stopped;
    };

}; // namespace mcs::execution::task_v2::__detail