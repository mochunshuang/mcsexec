#pragma once

#include <type_traits>

namespace mcs::execution::cmplsigs
{

    struct undefine_completion_signatures_for
    {
    };

    template <class Sndr, class Env>
    struct completion_signatures_for_impl;

    namespace __detail
    {
        template <template <class...> class T, class... Args>
        concept has_completion_type = requires { typename T<Args...>::type; };

        template <class Sndr, class Env>
        struct __completion_signatures_for;

        template <class Sndr, class Env>
            requires(not has_completion_type<completion_signatures_for_impl, Sndr, Env>)
        struct __completion_signatures_for<Sndr, Env>
        {
            using type = undefine_completion_signatures_for;
        };

        template <class Sndr, class Env>
            requires(has_completion_type<completion_signatures_for_impl, Sndr, Env>)
        struct __completion_signatures_for<Sndr, Env>
        {
            using type = typename completion_signatures_for_impl<Sndr, Env>::type;
        };

    }; // namespace __detail

    template <class Sndr, class Env>
    using completion_signatures_for = // exposition only
        typename __detail::__completion_signatures_for<Sndr, Env>::type;
    ;

}; // namespace mcs::execution::cmplsigs