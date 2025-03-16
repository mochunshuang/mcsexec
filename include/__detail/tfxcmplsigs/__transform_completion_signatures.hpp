#pragma once

#include "../diagnostics/__check.hpp"
#include "../cmplsigs/__valid_completion_signatures.hpp"

namespace mcs::execution::tfxcmplsigs
{

    ////////////////////////////////////
    // [exec.utils.tfxcmplsigs]

    // NOLINTNEXTLINE
    inline constexpr auto value_transform_default = []<class... As>() {
        return cmplsigs::completion_signatures<set_value_t(As...)>();
    };
    // NOLINTNEXTLINE
    inline constexpr auto error_transform_default = []<class Error>() {
        return cmplsigs::completion_signatures<set_error_t(Error)>();
    };

    namespace __detail
    {
        template <class... As, class Fn> // NOLINTNEXTLINE
        inline consteval auto __apply_transform(const Fn &fn)
        {
            if constexpr (not requires {
                              {
                                  fn.template operator()<As...>()
                              } -> cmplsigs::valid_completion_signatures;
                          })
                return diagnostics::invalid_completion_signature<
                    NOTE_INFO(struct apply_transform), WITH_FUNCTION(Fn),
                    WITH_ARGUMENTS(As...)>("__apply_transform ill format"); // see below
            else
                return fn.template operator()<As...>();
        }

        template <class Fn, class... Ts>
        concept callable_with = requires(Fn &&fn, Ts &&...ts) {
            static_cast<Fn &&>(fn)(static_cast<Ts &&>(ts)...);
        };

        template <class Fn, class... Ts>
        concept nothrow_callable_with = requires(Fn &&fn, Ts &&...ts) {
            { static_cast<Fn &&>(fn)(static_cast<Ts &&>(ts)...) } noexcept;
        };
        template <class... Sigs, callable_with<Sigs *...> Fn>
        constexpr decltype(auto) __apply(
            Fn fn, cmplsigs::completion_signatures<
                       Sigs...> /*unused*/) noexcept(nothrow_callable_with<Fn, Sigs *...>)
        {
            return fn(static_cast<Sigs *>(nullptr)...);
        }
    }; // namespace __detail

    template <cmplsigs::valid_completion_signatures Completions,
              cmplsigs::valid_completion_signatures OtherCompletions =
                  cmplsigs::completion_signatures<>,
              class ValueTransform = decltype(value_transform_default),
              class ErrorTransform = decltype(error_transform_default),
              cmplsigs::valid_completion_signatures StoppedCompletions =
                  cmplsigs::completion_signatures<set_stopped_t()>>
    inline consteval auto transform_completion_signatures( // NOLINT
        Completions completions = {}, OtherCompletions other_completions = {},
        ValueTransform value_transform = {}, // NOLINT // NOLINTNEXTLINE
        ErrorTransform error_transform = {}, StoppedCompletions stopped_completions = {})
        -> cmplsigs::valid_completion_signatures auto
    {
        auto transform1 = [=]<class Tag, class... As>(Tag (*)(As...)) {
            if constexpr (std::is_same_v<Tag, set_value_t>) // see "Completion tag
                                                            // comparison" below
                return __detail::__apply_transform<As...>(value_transform);
            else if constexpr (std::is_same_v<Tag, set_error_t>)
                return __detail::__apply_transform<As...>(error_transform);
            else
                return stopped_completions;
        };
        auto transform_all = [=](auto *...sigs) {
            return (transform1(sigs) + ... + cmplsigs::completion_signatures<>{});
        };

        return __detail::__apply(transform_all, completions) + other_completions;
    }
}; // namespace mcs::execution::tfxcmplsigs