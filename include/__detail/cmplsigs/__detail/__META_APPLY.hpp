#pragma once

namespace mcs::execution::cmplsigs::__detail
{
    ////////////////////////////////////
    // [exec.utils.cmplsigs]
    template <bool>
    struct indirect_meta_apply
    {
        template <template <typename...> typename T, typename... As>
        using meta_apply = T<As...>;
    };

    template <typename...>
    concept always_true = true;

    template <template <typename...> typename T, typename... As>
    using META_APPLY =
        typename indirect_meta_apply<always_true<As...>>::template meta_apply<T, As...>;

}; // namespace mcs::execution::cmplsigs::__detail