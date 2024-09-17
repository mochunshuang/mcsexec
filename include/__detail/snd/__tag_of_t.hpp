#pragma once

#include "./__sender.hpp"

namespace mcs::execution::snd
{
    template <class Sndr>
    concept has_tag_t = requires { typename Sndr::__tag_t; }; // exposition only

    namespace __detail
    {
        template <typename Sndr>
        struct tag_of_t_impl;

        template <has_tag_t Sndr>
        struct tag_of_t_impl<Sndr>
        {
            using type = typename Sndr::__tag_t;
        };

        template <typename Sndr>
            requires(not has_tag_t<Sndr> &&
                     requires { std::declval<Sndr>().template get<0>(); })
        struct tag_of_t_impl<Sndr>
        {
            using type = decltype(auto(std::declval<Sndr>().template get<0>()));
        };
    }; // namespace __detail

    // If the declaration auto&& [tag, data, ...children] = sndr; would be well-formed,
    // tag_of_t<Sndr> is an alias for decltype(auto(tag)).
    // Otherwise, tag_of_t<Sndr> is ill-formed.
    // template <sender Sndr>
    template <sender Sndr>
    using tag_of_t = typename __detail::tag_of_t_impl<Sndr>::type;
}; // namespace mcs::execution::snd
