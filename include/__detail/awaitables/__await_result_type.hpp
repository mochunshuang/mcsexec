#pragma once

#include "./__detail/__GET_AWAITER.hpp"

namespace mcs::execution::awaitables
{
    // await-result-type
    template <class C, class Promise>
    using await_result_type = decltype(awaitables::__detail::GET_AWAITER(
                                           std::declval<C>(), std::declval<Promise>())
                                           .await_resume());

}; // namespace mcs::execution::awaitables
