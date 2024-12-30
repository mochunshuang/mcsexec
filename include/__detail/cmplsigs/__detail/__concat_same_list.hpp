
#pragma once

namespace mcs::execution::cmplsigs::__detail
{
    template <template <typename...> class _Template, typename Tag, typename T>
    struct concat_same_list;

    template <template <typename...> class _Template, typename... T>
    struct concat_same_list<_Template, _Template<>, _Template<T...>>
    {
        using type = _Template<T...>;
    };

    template <template <typename...> class _Template, typename... T>
    struct concat_same_list<_Template, _Template<T...>, _Template<>>
    {
        using type = _Template<T...>;
    };

    template <template <typename...> class _Template>
    struct concat_same_list<_Template, _Template<>, _Template<>>
    {
        using type = _Template<>;
    };

    template <template <typename...> class _Template, typename... First,
              typename... Second>
    struct concat_same_list<_Template, _Template<First...>, _Template<Second...>>
    {
        using type = _Template<First..., Second...>;
    };
}; // namespace mcs::execution::cmplsigs::__detail