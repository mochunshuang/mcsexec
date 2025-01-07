#pragma once

#include "./__concat_same_list.hpp"
namespace mcs::execution::cmplsigs::__detail
{
    template <template <typename...> class TypeList, typename... Lists>
    struct merge_type_lists;

    template <template <typename...> class TypeList>
    struct merge_type_lists<TypeList>
    {
        using type = TypeList<>;
    };

    // 特化：处理单个 type_list
    template <template <typename...> class TypeList, typename List>
    struct merge_type_lists<TypeList, List>
    {
        using type = List;
    };

    // 特化：处理多个 type_list
    template <template <typename...> class TypeList, typename List1, typename List2,
              typename... Rest>
    struct merge_type_lists<TypeList, List1, List2, Rest...>
    {
        using merged_first_two = typename concat_same_list<TypeList, List1, List2>::type;
        using type = typename merge_type_lists<TypeList, merged_first_two, Rest...>::type;
    };

}; // namespace mcs::execution::cmplsigs::__detail