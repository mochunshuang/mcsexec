#pragma once

#include <utility>

namespace mcs::execution::snd::general
{

    template <class Tag, class Env, class Default>
    constexpr decltype(auto) query_with_default(
        Tag tag, const Env &env, Default && /*value*/) noexcept(noexcept(tag(env)))
        requires requires() { tag(env); }
    {
        return tag(env);
    }

    template <class Tag, class Env, class Default>
    constexpr decltype(auto) query_with_default(
        Tag tag, const Env &env,
        Default &&
            value) noexcept(noexcept(static_cast<Default>(std::forward<Default>(value))))
        requires(not requires() { tag(env); })
    {
        return static_cast<Default>(std::forward<Default>(value));
    }

}; // namespace mcs::execution::snd::general