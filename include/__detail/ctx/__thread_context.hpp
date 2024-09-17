#pragma once
#include "__run_loop.hpp"

namespace mcs::execution::ctx
{
    struct thread_context : public run_loop
    {
        using run_loop::pop_front;
        using run_loop::count;
        using run_loop::state;
    };
}; // namespace mcs::execution::ctx