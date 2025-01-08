#pragma once
#include "__run_loop.hpp"

namespace mcs::execution::ctx
{
    struct thread_context : public run_loop
    {
        using run_loop::state;
        using run_loop::head;
        using run_loop::tail;
        using run_loop::cv;
        using run_loop::mtx;
    };
}; // namespace mcs::execution::ctx