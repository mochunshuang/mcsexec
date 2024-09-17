#pragma once

#include "__connect_all.hpp"

namespace mcs::execution::snd::__detail
{
    template <class Sndr, class Rcvr>
        requires valid_specialization<mate_type::state_type, Sndr, Rcvr> &&
                 valid_specialization<connect_all_result, Sndr, Rcvr>
    struct basic_operation : basic_state<Sndr, Rcvr> // exposition only
    {
        using operation_state_concept = operation_state_t;
        using tag_t = tag_of_t<Sndr>; // exposition only

        connect_all_result<Sndr, Rcvr> inner_ops; // exposition only // NOLINT

        basic_operation(Sndr &&sndr, Rcvr &&rcvr) noexcept(
            std::is_nothrow_constructible_v<basic_state<Sndr, Rcvr>, Sndr, Rcvr> &&
            noexcept(connect_all(this, std::forward<Sndr>(sndr),
                                 mate_type::indices_for<Sndr>()))) // exposition only
            : basic_state<Sndr, Rcvr>(std::forward<Sndr>(sndr), std::move(rcvr)),
              inner_ops{connect_all(this, std::forward<Sndr>(sndr),
                                    mate_type::indices_for<Sndr>())}
        {
        }

        void start() & noexcept
        {
            inner_ops.apply([&]<typename... Op>(Op &...ops) {
                general::impls_for<tag_t>::start(this->state, this->rcvr, ops...);
            });
        }

        basic_operation(const basic_operation &) = delete;
        basic_operation(basic_operation &&) = delete;
        basic_operation &operator=(const basic_operation &) = delete;
        basic_operation &operator=(basic_operation &&) = delete;
        ~basic_operation() = default;
    };
}; // namespace mcs::execution::snd::__detail
