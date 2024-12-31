#pragma once

namespace mcs::execution::consumers
{
    namespace __sync_wait
    {
        struct sync_wait_with_variant
        {
            template <typename Sndr>
            auto operator()(Sndr &&sndr) const
            {
            }
        };

    }; // namespace __sync_wait

}; // namespace mcs::execution::consumers