#pragma once

#include "./__detail/__GET_AWAITER.hpp"

#include "./__is_awaiter.hpp"

namespace mcs::execution::awaitables
{

    template <class Expr, class Promise>
    concept is_awaitable = requires(Expr &&expr, Promise &p) {
        {
            __detail::GET_AWAITER(::std::forward<Expr>(expr), p)
        } -> awaitables::is_awaiter<Promise>;
    };

}; // namespace mcs::execution::awaitables