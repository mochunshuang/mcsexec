#pragma once

#include "../__completion_signatures.hpp"
#include "./__concat_same_list.hpp"
namespace mcs::execution::cmplsigs::__detail
{
    // Note: 不再依赖std::tuple
    //  template <typename Completion, typename Sigs>
    //  using filter_sigs_by_completion = tpl_param_trnsfr_t<
    //      completion_signatures,
    //      typename filter_tuple<select_tag<Completion>::template predicate,
    //                            tpl_param_trnsfr_t<std::tuple, Sigs>>::type>;

    template <typename Tag, typename T>
    struct same_completion
    {
        constexpr static bool value = false; // NOLINT
    };

    template <typename Tag, typename... T>
    struct same_completion<Tag, Tag(T...)>
    {
        constexpr static bool value = true; // NOLINT
    };

    template <typename Tag, typename T>
    struct filter_sigs_by_completion;

    template <typename Tag, typename Cur, typename... T>
    struct filter_sigs_by_completion<Tag, cmplsigs::completion_signatures<Cur, T...>>
    {
        using FilteredCur = std::conditional_t<same_completion<Tag, Cur>::value,
                                               cmplsigs::completion_signatures<Cur>,
                                               cmplsigs::completion_signatures<>>;

        using FilteredRest = typename filter_sigs_by_completion<
            Tag, cmplsigs::completion_signatures<T...>>::type;
        using type = typename cmplsigs::__detail::concat_same_list<
            cmplsigs::completion_signatures, FilteredCur, FilteredRest>::type;
    };

    // 递归终止条件
    template <typename Tag>
    struct filter_sigs_by_completion<Tag, cmplsigs::completion_signatures<>>
    {
        using type = cmplsigs::completion_signatures<>;
    };

    template <typename Tag, typename T>
    struct skip_sigs_by_completion;

    template <typename Tag, typename Cur, typename... T>
    struct skip_sigs_by_completion<Tag, cmplsigs::completion_signatures<Cur, T...>>
    {
        using SkippedCur = std::conditional_t<same_completion<Tag, Cur>::value,
                                              cmplsigs::completion_signatures<>,
                                              cmplsigs::completion_signatures<Cur>>;

        using SkippedRest =
            typename skip_sigs_by_completion<Tag,
                                             cmplsigs::completion_signatures<T...>>::type;

        using type = typename concat_same_list<cmplsigs::completion_signatures,
                                               SkippedCur, SkippedRest>::type;
    };

    template <typename Tag>
    struct skip_sigs_by_completion<Tag, cmplsigs::completion_signatures<>>
    {
        using type = cmplsigs::completion_signatures<>;
    };

}; // namespace mcs::execution::cmplsigs::__detail