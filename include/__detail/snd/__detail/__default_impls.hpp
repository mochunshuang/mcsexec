#pragma once

#include "../../__core_types.hpp"

#include "../../queries/__get_env.hpp"
#include "../../opstate/__start.hpp"
#include "../../__functional/__callable.hpp"

#include "../general/__FWD_ENV.hpp"

namespace mcs::execution::snd::__detail
{
    struct default_impls // exposition only
    {
        static constexpr auto get_attrs = // NOLINT
            [](const auto &, const auto &...child) noexcept -> decltype(auto) {
            if constexpr (sizeof...(child) == 1)
                return (::mcs::execution::snd::general::FWD_ENV(
                            ::mcs::execution::queries::get_env(child)),
                        ...);
            else
                return ::mcs::execution::empty_env();
        };

        static constexpr auto get_env = // NOLINT
            [](auto, auto &, const auto &rcvr) noexcept -> decltype(auto) {
            return ::mcs::execution::snd::general::FWD_ENV(
                ::mcs::execution::queries::get_env(rcvr));
        };

        static constexpr auto get_state = // NOLINT
            []<class Sndr, class Rcvr>(Sndr &&sndr,
                                       Rcvr & /*rcvr*/) noexcept -> decltype(auto) {
            return sndr.apply([](auto &, auto &data, auto &&...) -> decltype(auto) {
                return std::forward_like<Sndr>(data);
            });
        };

        // NOLINTNEXTLINE
        static constexpr auto start = [](auto &, auto &, auto &...ops) noexcept -> void {
            (::mcs::execution::opstate::start(ops), ...);
        };

        static constexpr auto complete = // NOLINT
            []<class Index, class Rcvr, class Tag, class... Args>(
                Index, auto & /*state*/, Rcvr &rcvr, Tag, Args &&...args) noexcept -> void
            requires functional::callable<Tag, Rcvr, Args...>
        {
            // Mandates: Index::value == 0,the index is  tag
            static_assert(Index::value == 0,
                          "I don't know how to complete this operation.");
            Tag()(std::move(rcvr), std::forward<Args>(args)...);
        };
    };

}; // namespace mcs::execution::snd::__detail
