#pragma once

#include "../conn/__connect.hpp"
#include "../factories/__schedule.hpp"
#include "../queries/__get_env.hpp"

#include "./__scope_state_base.hpp"

namespace mcs::execution::scope
{
    template <class scope_type, class Recv>
    struct scope_state : scope_state_base
    {
        scope_type *scope;                   // exposition_only // NOLINT
        std::remove_cvref_t<Recv> &receiver; // exposition_only // NOLINT
        using op_t = decltype(conn::connect(
            factories::schedule(queries::get_scheduler(queries::get_env(receiver))),
            receiver)); // exposition-only
        op_t op;        // NOLINT

        scope_state(scope_type *scope, Recv &recv)
            : scope_state_base{.execute = &scope_state::invoke}, scope(scope),
              receiver(recv),
              op(conn::connect(
                  factories::schedule(queries::get_scheduler(queries::get_env(recv))),
                  recv))
        {
        }

        void complete() noexcept
        {
            static_assert(noexcept(op.start()), "op.start() need noexcept");
            op.start();
        }

        void complete_inline() noexcept // NOLINT
        {
            recv::set_value(std::move(receiver));
        }

        static void invoke(scope_state_base *self) noexcept
        {
            auto *derived = static_cast<scope_state *>(self);
            derived->complete();
        }
    };

}; // namespace mcs::execution::scope