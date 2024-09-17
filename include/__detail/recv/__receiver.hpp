#pragma once

#include "../__core_types.hpp"
#include "../queries/__get_env.hpp"

namespace mcs::execution::recv
{

    //////////////////////////////////////////////////////////////////////////////
    // [exec.recv.concepts]
    template <class Rcvr>
    concept receiver =
        std::derived_from<typename std::remove_cvref_t<Rcvr>::receiver_concept,
                          receiver_t> &&
        requires(const std::remove_cvref_t<Rcvr> &rcvr) {
            { queries::get_env(rcvr) } -> queryable;
        } && std::move_constructible<std::remove_cvref_t<Rcvr>> && // rvalues are
                                                                   // movable, and
        std::constructible_from<std::remove_cvref_t<Rcvr>,
                                Rcvr>; // lvalues are copyable

}; // namespace mcs::execution::recv
