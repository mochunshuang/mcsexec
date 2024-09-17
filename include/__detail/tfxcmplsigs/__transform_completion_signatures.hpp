#pragma once

#include "./__unique_variadic_template.hpp"
#include "./__default_set_value.hpp"
#include "./__default_set_error.hpp"

#include "../cmplsigs/__valid_completion_signatures.hpp"
#include "../cmplsigs/__gather_signatures.hpp"
#include <type_traits>

namespace mcs::execution::tfxcmplsigs
{

    ////////////////////////////////////
    // [exec.utils.tfxcmplsigs]

    namespace __detail
    {
        template <template <class...> class SetError>
        struct error_list
        {
            template <typename... Ts>
            using type = type_list<SetError<Ts>...>;
        };

        // 1.
        // SetValue shall name an alias template such that for any pack of types As, the
        // type SetValue<As...> is either ill-formed or else
        // valid-completion-signatures<SetValue<As...>> is satisfied.
        //
        // 2.
        //  for any type Err, SetError<Err> is either ill-formed or else
        //  valid-completion-signatures<SetError<Err>> is satisfied.
        //
        // Note:
        // valid-completion-signatures<T> is true when T is completion_signatures<As...>
        //
        template <typename T>
        inline constexpr bool is_cmplsigs_instance = false; // NOLINT

        template <class... As>
        inline constexpr bool // NOLINTNEXTLINE
            is_cmplsigs_instance<cmplsigs::completion_signatures<As...>> = true;

        template <typename AdditionalSignatures, typename SetValue, typename SetError,
                  typename Stop>
        struct __completion_signatures_set;

        template <typename... Ss, typename... As, typename... Vs, typename... Es>
        struct __completion_signatures_set<
            cmplsigs::completion_signatures<As...>,
            type_list<cmplsigs::completion_signatures<Vs>...>,
            type_list<cmplsigs::completion_signatures<Es>...>,
            cmplsigs::completion_signatures<Ss...>>
        {
            // using type = cmplsigs::completion_signatures<As..., Vs..., Es..., Ss>;
            using type = cmplsigs::completion_signatures<As..., Vs..., Es..., Ss...>;
        };

    }; // namespace __detail

    // SetValue shall name an alias template such that for any pack of types As, the type
    // SetValue<As...> is either ill-formed or else
    // valid-completion-signatures<SetValue<As...>> is satisfied.
    template <cmplsigs::valid_completion_signatures _InputSignatures,
              cmplsigs::valid_completion_signatures _AdditionalSignatures =
                  cmplsigs::completion_signatures<>,
              template <class...> class _SetValue = default_set_value,
              template <class> class _SetError = default_set_error,
              cmplsigs::valid_completion_signatures _SetStopped =
                  cmplsigs::completion_signatures<set_stopped_t()>>
        requires(__detail::is_cmplsigs_instance<_SetValue<>> &&
                 __detail::is_cmplsigs_instance<_SetError<int>>)
    using transform_completion_signatures =
        typename unique_variadic_template<typename __detail::__completion_signatures_set<
            _AdditionalSignatures,
            cmplsigs::gather_signatures<set_value_t, _InputSignatures, _SetValue,
                                        type_list>,
            cmplsigs::gather_signatures<set_error_t, _InputSignatures,
                                        std::type_identity_t,
                                        __detail::error_list<_SetError>::template type>,
            std::conditional_t<
                std::is_same_v<cmplsigs::gather_signatures<
                                   set_stopped_t, _InputSignatures, type_list, type_list>,
                               type_list<>>,
                cmplsigs::completion_signatures<>, _SetStopped>>::type>::type;

}; // namespace mcs::execution::tfxcmplsigs