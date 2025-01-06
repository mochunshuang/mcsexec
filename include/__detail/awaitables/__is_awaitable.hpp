#pragma once

#include "./__detail/__GET_AWAITER.hpp"

#include "./__is_awaiter.hpp"

namespace mcs::execution::awaitables
{

    template <class C, class Promise>
    concept is_awaitable = requires(C &&c, Promise &p) {
        {
            __detail::GET_AWAITER(::std::forward<C>(c), p)
        } -> awaitables::is_awaiter<Promise>;
    };

}; // namespace mcs::execution::awaitables