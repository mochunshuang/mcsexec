#pragma once

#include "./__detail/__stoptoken/__declarations.hpp"

#include "./__detail/__stoptoken/__stoppable_callback_for.hpp"
#include "./__detail/__stoptoken/__stoppable_token.hpp"
#include "./__detail/__stoptoken/__unstoppable_token.hpp"
#include "./__detail/__stoptoken/__stoppable_source.hpp"

#include "./__detail/__stoptoken/__nostopstate_t.hpp"

#include "./__detail/__stoptoken/__stop_token.hpp"
#include "./__detail/__stoptoken/__stop_source.hpp"
#include "./__detail/__stoptoken/__stop_callback.hpp"
#include "./__detail/__stoptoken/__never_stop_token.hpp"
#include "./__detail/__stoptoken/__inplace_stop_token.hpp"
#include "./__detail/__stoptoken/__inplace_stop_source.hpp"
#include "./__detail/__stoptoken/__inplace_stop_callback.hpp"

namespace mcs::execution
{
    using stoptoken::stoppable_callback_for;
    using stoptoken::stoppable_token;
    using stoptoken::unstoppable_token;
    using stoptoken::stoppable_source;

    // 33.3.3, class stop_token
    using stoptoken::stop_token;

    // 33.3.4, class stop_source
    using stoptoken::stop_source;

    using stoptoken::nostopstate_t;
    inline constexpr nostopstate_t nostopstate{}; // NOLINT

    // 33.3.5, class template stop_callback
    using stoptoken::stop_callback;

    // [stoptoken.never], class never_stop_token
    using stoptoken::never_stop_token;

    // [stoptoken.inplace], class inplace_stop_token
    using stoptoken::inplace_stop_token;

    // [stopsource.inplace], class inplace_stop_source
    using stoptoken::inplace_stop_source;

    // [stopcallback.inplace], class template inplace_stop_callback
    using stoptoken::inplace_stop_callback;

    using stoptoken::stop_callback_for_t;

}; // namespace mcs::execution