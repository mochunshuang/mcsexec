#pragma once

#include <boost/ut.hpp>
#include "./../include/execution.hpp"
#include "./test_common/test_macro.hpp"
#include "./test_common/receivers.hpp"

using namespace boost::ut;     // NOLINT
namespace ex = mcs::execution; // NOLINT
using namespace ex::conn;      // NOLINT
using namespace ex::opstate;   // NOLINT

template <typename... T>
struct TEST_TYPE;

template <ex::sender S, class... Ts>
inline void wait_for_value(S &&snd, Ts &&...val)
{
    std::optional<std::tuple<Ts...>> res =
        mcs::this_thread::sync_wait(std::forward<S>(snd));
    EXPECT(res.has_value());
    std::tuple<Ts...> expected(std::forward<Ts>(val)...);
    if constexpr (std::tuple_size_v<std::tuple<Ts...>> == 1)
        EXPECT(std::get<0>(res.value()) == std::get<0>(expected));
    else
        EXPECT(res.value() == expected);
}