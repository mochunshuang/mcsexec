#pragma once

namespace mcs::execution::cmplsigs
{

    template <class Sndr, class... Env>
    struct completion_signatures_for_impl;

    // NOTE: 越简单越简洁越好，越不容易出bug。无招胜有招
    template <class Sndr, class... Env>
    using completion_signatures_for = // exposition only
        typename completion_signatures_for_impl<Sndr, Env...>::type;

}; // namespace mcs::execution::cmplsigs