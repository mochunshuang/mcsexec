#pragma once
#include "./__forwarding_query.hpp"

#include "../sched/__scheduler.hpp"
#include "../__core_types.hpp"

namespace mcs::execution::queries
{

    // associated execution resource
    // NOLINTNEXTLINE
    inline constexpr struct get_forward_progress_guarantee_t : forwarding_query_t
    {
        using __self_t = get_forward_progress_guarantee_t;

        template <typename T>
        constexpr auto operator()(T &&sch) const noexcept -> forward_progress_guarantee
            requires(sched::scheduler<decltype((sch))>)
        {
            if constexpr (requires {
                              {
                                  std::as_const(sch).query(std::declval<__self_t>())
                              } noexcept;
                          })
            {
                // Note: scheduler should provide the concurrent / parallel guarantee
                return std::as_const(sch).query(*this);
            }
            else
            {
                return forward_progress_guarantee::weakly_parallel;
            }
        }

    } get_forward_progress_guarantee{}; // NOLINT

}; // namespace mcs::execution::queries