#pragma once

namespace mcs::execution::snd::__detail::mate_type
{
    struct __get_state
    {
        template <class _State>
        inline _State &operator()(auto & /*unused*/, _State &__state,
                                  auto &.../*unused*/) const noexcept
        {
            return __state;
        }
    };

}; // namespace mcs::execution::snd::__detail::mate_type