#pragma once

#include <concepts>

namespace mcs::execution::opstate
{
    ////////////////////////////////////////////
    // [exec.opstate.start]
    namespace __start
    {
        // If op.start() does not start ([async.ops]) the asynchronous operation
        // associated with the operation state op, the behavior of calling start(op) is
        // undefine
        struct start_t
        {
            template <typename O>
            constexpr void operator()(O &&o) const noexcept
                requires(not std::is_rvalue_reference_v<decltype(o)>) && requires(O &o) {
                    { o.start() } noexcept -> std::same_as<void>;
                }
            {
                o.start();
            }
        };

    }; // namespace __start

    inline constexpr __start::start_t start{}; // NOLINT
}; // namespace mcs::execution::opstate
