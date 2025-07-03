#pragma once

#include "../recv/__set_value.hpp"

namespace mcs::execution::__task
{

    template <typename R>
    struct task_value_sig
    {
        using type = recv::set_value_t(R);
    };
    template <>
    struct task_value_sig<void>
    {
        using type = recv::set_value_t();
    };

}; // namespace mcs::execution::__task