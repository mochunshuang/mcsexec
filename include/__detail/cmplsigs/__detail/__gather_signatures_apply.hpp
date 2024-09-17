#pragma once

#include "./__META_APPLY.hpp"

namespace mcs::execution::cmplsigs::__detail
{
    template <typename, template <typename...> class>
    struct gather_signatures_apply;

    template <typename R, typename... A, template <typename...> class Transform>
        requires requires { typename META_APPLY<Transform, A...>; }
    struct gather_signatures_apply<R(A...), Transform>
    {
        using type = META_APPLY<Transform, A...>;
    };
}; // namespace mcs::execution::cmplsigs::__detail