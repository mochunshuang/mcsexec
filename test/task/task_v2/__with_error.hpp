#pragma once

#include <type_traits>

namespace mcs::execution::task_v2
{
    template <class E>
    struct with_error
    {
        using type = std::remove_cvref_t<E>;
        type error;
    };
    template <class E>
    with_error(E &&) -> with_error<E>;
}; // namespace mcs::execution::task_v2