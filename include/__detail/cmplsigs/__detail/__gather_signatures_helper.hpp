#pragma once

#include "../__valid_completion_signatures.hpp"
#include "__gather_signatures_apply.hpp"

namespace mcs::execution::cmplsigs::__detail
{
    template <valid_completion_signatures,
              template <typename...> class, //
              template <typename...> class>
    struct gather_signatures_helper;

    template <typename... Signatures, template <typename...> class Tuple,
              template <typename...> class Variant>
        requires requires {
            typename META_APPLY<
                Variant, typename gather_signatures_apply<Signatures, Tuple>::type...>;
        }
    struct gather_signatures_helper<cmplsigs::completion_signatures<Signatures...>, Tuple,
                                    Variant>
    {
        using type =
            META_APPLY<Variant,
                       typename gather_signatures_apply<Signatures, Tuple>::type...>;
    };

}; // namespace mcs::execution::cmplsigs::__detail