#pragma once

#include "../__detail/__default_impls.hpp"

namespace mcs::execution::snd::general
{
    // impls_for:
    // The exposition-only class template impls-for ([exec.snd.general]) is specialized
    // for into_variant as follows
    template <class Tag>
    struct impls_for : snd::__detail::default_impls
    {
    };

    template <>
    struct impls_for<sender_t> : snd::__detail::default_impls
    {
    }; // exposition only

}; // namespace mcs::execution::snd::general
