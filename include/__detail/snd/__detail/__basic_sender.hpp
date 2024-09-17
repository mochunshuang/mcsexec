#pragma once

#include "__basic_operation.hpp"
#include "../../cmplsigs/__completion_signatures_for.hpp"
#include <type_traits>

namespace mcs::execution::snd::__detail
{

    template <class Tag, class Data, class... Child>
    struct basic_sender : product_type<Tag, Data, Child...> // exposition only
    {

        using sender_concept = sender_t;
        using indices_for = std::index_sequence_for<Child...>; // exposition only

        decltype(auto) get_env() const noexcept // NOLINT
        {
            return this->apply(
                [](auto & /*tag*/, auto &data, auto &...child) -> decltype(auto) {
                    return general::impls_for<Tag>::get_attrs(data, child...);
                });
        }

        template <decays_to<basic_sender> Self, receiver Rcvr>
        auto connect(this Self &&self, Rcvr rcvr) noexcept(noexcept(
            std::is_nothrow_constructible_v<basic_operation<Self, Rcvr>, Self, Rcvr>))
            -> basic_operation<Self, Rcvr>
        {
            return {std::forward<Self>(self), std::move(rcvr)};
        }

        template <decays_to<basic_sender> Self, class Env>
        auto get_completion_signatures(this Self && /*self*/, // NOLINT
                                       Env && /*env*/) noexcept
            -> cmplsigs::completion_signatures_for<std::remove_cvref_t<Self>, Env>
        {
            return {};
        }
    };
}; // namespace mcs::execution::snd::__detail