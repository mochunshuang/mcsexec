#pragma once

#include "./__basic_state.hpp"
#include "./__valid_specialization.hpp"

#include "../../__recv.hpp"

#include "./mate_type/__env_type.hpp"
#include "./mate_type/__state_type.hpp"

namespace mcs::execution::snd::__detail
{

    template <class Sndr, class Rcvr, class Index>
        requires valid_specialization<mate_type::env_type, Index, Sndr, Rcvr>
    struct basic_receiver // exposition only
    {
        using receiver_concept = receiver_t;

        using tag_t = tag_of_t<Sndr>;                      // exposition only
        using state_t = mate_type::state_type<Sndr, Rcvr>; // exposition only

        // NOLINTNEXTLINE
        static constexpr const auto &complete = general::impls_for<tag_t>::complete;
        basic_state<Sndr, Rcvr> *parent_op; // exposition only // NOLINT

        template <class... Args>
            requires functional::callable<decltype(complete), Index, state_t &, Rcvr &,
                                          set_value_t,
                                          Args...>
        void set_value(Args &&...args) && noexcept // NOLINT
        {
            complete(Index(), parent_op->state, parent_op->rcvr, set_value_t(),
                     std::forward<Args>(args)...);
        }

        template <class Error>
            requires functional::callable<decltype(complete), Index, state_t &, Rcvr &,
                                          set_error_t,
                                          Error>
        void set_error(Error &&err) && noexcept // NOLINT
        {
            complete(Index(), parent_op->state, parent_op->rcvr, set_error_t(),
                     std::forward<Error>(err));
        }

        void set_stopped() && noexcept // NOLINT
            requires functional::callable<decltype(complete), Index, state_t &, Rcvr &,
                                          set_stopped_t>
        {
            complete(Index(), parent_op->state, parent_op->rcvr, set_stopped_t());
        }

        auto get_env() const noexcept -> mate_type::env_type<Index, Sndr, Rcvr> // NOLINT
        {
            return general::impls_for<tag_t>::get_env(Index(), parent_op->state,
                                                      parent_op->rcvr);
        }
    };
}; // namespace mcs::execution::snd::__detail