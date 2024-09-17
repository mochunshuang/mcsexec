#pragma once

#include "./__value_types_of_t.hpp"
#include <concepts>

namespace mcs::execution::cmplsigs
{
    namespace __detail
    {
        template <typename Sndr, typename Env>
        concept satisfy_value_types_of = requires {
            typename value_types_of_t<Sndr, Env, std::decay_t, std::type_identity_t>;
        };

        template <typename Sndr, typename Env>
        concept satisfy_empty_variant =
            requires {
                typename value_types_of_t<Sndr, Env, std::tuple, std::variant>;
            } && (std::same_as<value_types_of_t<Sndr, Env, std::tuple, std::variant>,
                               std::variant<std::tuple<>>> ||
                  std::same_as<value_types_of_t<Sndr, Env, std::tuple, std::variant>,
                               std::variant<>>);

        template <typename Sndr, typename Env>
        concept satisfy_decayed_tuple = requires {
            typename value_types_of_t<Sndr, Env, decayed_tuple, std::type_identity_t>;
        };

        template <class Sndr, class Env>
        struct __single_sender_value_type;

        template <class Sndr, class Env>
            requires __detail::satisfy_value_types_of<Sndr, Env>
        struct __single_sender_value_type<Sndr, Env>
        {
            using type = value_types_of_t<Sndr, Env, std::decay_t, std::type_identity_t>;
        };

        template <class Sndr, class Env>
            requires(not __detail::satisfy_value_types_of<Sndr, Env> //
                     && __detail::satisfy_empty_variant<Sndr, Env>)
        struct __single_sender_value_type<Sndr, Env>
        {
            using type = void;
        };

        template <class Sndr, class Env>
            requires(not __detail::satisfy_value_types_of<Sndr, Env>   //
                     && not __detail::satisfy_empty_variant<Sndr, Env> //
                     && __detail::satisfy_decayed_tuple<Sndr, Env>)
        struct __single_sender_value_type<Sndr, Env>
        {
            using type = value_types_of_t<Sndr, Env, decayed_tuple, std::type_identity_t>;
        };
    }; // namespace __detail

    template <class Sndr, class Env>
    using single_sender_value_type =
        typename __detail::__single_sender_value_type<Sndr, Env>::type;

}; // namespace mcs::execution::cmplsigs