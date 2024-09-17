#pragma once
#include "../../__stoptoken/__inplace_stop_source.hpp"

namespace mcs::execution::snd::general
{

    struct on_stop_request
    {                                             // exposition only
        stoptoken::inplace_stop_source &stop_src; // exposition only // NOLINT
        void operator()() noexcept
        {
            stop_src.request_stop();
        }
    };

}; // namespace mcs::execution::snd::general