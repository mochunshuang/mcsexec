#pragma once

#include <type_traits>
#include "./completion_signatures.h"
#include "./invalid_completion_signature.h"

// NOLINTNEXTLINE
inline constexpr auto value_transform_default = []<class... As>() {
    return completion_signatures<set_value_t(As...)>();
};
// NOLINTNEXTLINE
inline constexpr auto error_transform_default = []<class Error>() {
    return completion_signatures<set_error_t(Error)>();
};

template <class... As, class Fn> // NOLINTNEXTLINE
consteval auto __apply_transform(const Fn &fn)
{
    if constexpr (not requires {
                      { fn.template operator()<As...>() } -> valid_completion_signatures;
                  })
        return invalid_completion_signature<WITH_INFO(struct apply_transform),
                                            WITH_FUNCTION(Fn), WITH_ARGUMENTS(As...)>(
            "__apply_transform ill format"); // see below
    else
        return fn.template operator()<As...>();
}

template <class Fn, class... Ts>
concept callable_with =
    requires(Fn &&fn, Ts &&...ts) { static_cast<Fn &&>(fn)(static_cast<Ts &&>(ts)...); };

template <class Fn, class... Ts>
concept nothrow_callable_with = requires(Fn &&fn, Ts &&...ts) {
    { static_cast<Fn &&>(fn)(static_cast<Ts &&>(ts)...) } noexcept;
};
template <class... Sigs, callable_with<Sigs *...> Fn>
constexpr decltype(auto) __apply(
    Fn fn,
    completion_signatures<Sigs...> /*unused*/) noexcept(nothrow_callable_with<Fn,
                                                                              Sigs *...>)
{
    return fn(static_cast<Sigs *>(nullptr)...);
}

template <valid_completion_signatures Completions,
          class ValueTransform = decltype(value_transform_default),
          class ErrorTransform = decltype(error_transform_default),
          valid_completion_signatures StoppedCompletions =
              completion_signatures<set_stopped_t()>,
          valid_completion_signatures OtherCompletions = completion_signatures<>>
consteval auto transform_completion_signatures( // NOLINT
    Completions completions,
    ValueTransform value_transform = {}, // NOLINT // NOLINTNEXTLINE
    ErrorTransform error_transform = {}, StoppedCompletions stopped_completions = {},
    OtherCompletions other_completions = {}) -> valid_completion_signatures auto
{
    auto transform1 = [=]<class Tag, class... As>(Tag (*)(As...)) {
        if constexpr (std::is_same_v<Tag, set_value_t>) // see "Completion tag
                                                        // comparison" below
            return __apply_transform<As...>(value_transform);
        else if constexpr (std::is_same_v<Tag, set_error_t>)
            return __apply_transform<As...>(error_transform);
        else
            return stopped_completions;
    };
    auto transform_all = [=](auto *...sigs) {
        return (completion_signatures<>{} + ... + transform1(sigs));
    };

    return __apply(transform_all, completions) + other_completions;
}