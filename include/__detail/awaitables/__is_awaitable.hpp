#pragma once

#include "./__detail/__GET_AWAITER.hpp"

#include "./__is_awaiter.hpp"

namespace mcs::execution::awaitables
{

    template <class C, class Promise>
    concept is_awaitable = requires(C (*fc)() noexcept, Promise &p) {
        // { GET_AWAITER(fc(), p) } -> is_awaiter<Promise>;
        { __detail::GET_AWAITER(fc(), p) } -> is_awaiter<Promise>;
    };

}; // namespace mcs::execution::awaitables