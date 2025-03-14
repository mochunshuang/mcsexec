#pragma once

#include <type_traits>
#include <utility>

#include "./general/__get_domain_late.hpp"

#include "./__transform_sender.hpp"
#include "../cmplsigs/__completion_signatures.hpp"

#include "../cmplsigs/__valid_completion_signatures.hpp"
#include "./__has_constexpr_completions.hpp"

#include "./__dependent_sender_error.hpp"

namespace mcs::execution::snd
{

    namespace __detail
    {
        template <class Sndr, class... Env>
        using completion_signatures_result_t = // exposition only
            decltype(std::remove_reference_t<Sndr>::template get_completion_signatures<
                     Sndr, Env...>());

        template <class Sndr, class... Env>
        concept dependent_sender_without_env = // exposition only
            (sizeof...(Env) == 0) &&
            not requires { typename completion_signatures_result_t<Sndr>; };

        template <typename Ret>
        concept valid_cs = std::is_same_v<Ret, dependent_sender_error> ||
                           cmplsigs::valid_completion_signatures<Ret>;

        template <class Sndr, class... Env>
        consteval auto get_completion_signatures_impl() -> valid_cs auto
        {
            using sndr_type = std::remove_reference_t<Sndr>;

            if constexpr (has_constexpr_get_completion_signatures<Sndr, Env...>)
            {
                // In the happy case where Sndr's customization is well-formed, a constant
                // expression, and has a completion_signatures<> type, just return the
                // result of calling the customization.
                return sndr_type::template get_completion_signatures<Sndr, Env...>();
            }
            else if constexpr (sizeof...(Env) == 1 &&
                               has_constexpr_get_completion_signatures<Sndr>)
            {
                return sndr_type::template get_completion_signatures<Sndr>();
            }
            // Otherwise, remove_cvref_t<NewSndr>::completion_signatures if that type is
            // well-formed,
            else if constexpr (has_completion_signatures_type<Sndr>)
            {
                using CS = typename sndr_type::completion_signatures;
                return CS{};
            }
            // Otherwise, (throw dependent-sender-error(), completion_signatures()) if
            // dependent-sender-without-env<Sndr, Env...> is true
            else if constexpr (dependent_sender_without_env<Sndr, Env...>)
            {
                // throw dependent_sender_error();
                return dependent_sender_error();
                // return cmplsigs::completion_signatures<>{};
            }
            else if constexpr (requires {
                                   typename completion_signatures_result_t<Sndr, Env...>;
                               })
            {
                struct unspecified
                {
                };
                throw unspecified{};
                return cmplsigs::completion_signatures<>{};
            }
            else
            {
                struct unspecified
                {
                };
                // Otherwise, we reach here under the following conditions:
                //_The call to Sndr's customization cannot be constant-evaluated
                // (possibly
                //   because it throws), or
                //_Its return type is not a completion_signatures type.
                //
                // We want to call the call the Sndr's customization so that if it throws
                // an exception, that exception's information will appear in the
                // diagnostic. If it doesn't throw, _we_ should throw to let the developer
                // know that their customization returned an invalid type. And again,
                // ensure that the return type is a completion_signatures type.
                return (throw unspecified{}, cmplsigs::completion_signatures());
            }
        }

    }; // namespace __detail

    // [exec.getcomplsigs]
    template <class Sndr, class... Env>
        requires(sizeof...(Env) <= 1)
    consteval auto get_completion_signatures()
    {
        if constexpr (sizeof...(Env) == 0)
        {
            return __detail::get_completion_signatures_impl<Sndr>();
        }
        else
        {
            // Apply a late sender transform:
            using NewSndr = decltype(transform_sender(
                decltype(general::get_domain_late(std::declval<Sndr>(),
                                                  std::declval<Env>()...)){},
                std::declval<Sndr>(), std::declval<Env>()...));
            return __detail::get_completion_signatures_impl<NewSndr, Env...>();
        }
    }

}; // namespace mcs::execution::snd
