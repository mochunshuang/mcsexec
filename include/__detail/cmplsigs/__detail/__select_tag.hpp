#pragma once

#include <concepts>
namespace mcs::execution::cmplsigs::__detail
{
    template <typename, typename>
    struct same_tag;
    template <typename Tag, typename R, typename... A>
    struct same_tag<Tag, R(A...)>
    {
        static constexpr bool value = std::same_as<Tag, R>; // NOLINT
    };

    template <typename Tag>
    struct select_tag
    {
        template <typename Fn>
        using predicate = same_tag<Tag, Fn>;
    };
}; // namespace mcs::execution::cmplsigs::__detail