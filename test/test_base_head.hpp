#pragma once

#include "./../include/execution.hpp"
#include "./test_common/test_macro.hpp"
#include "./test_common/receivers.hpp"

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

struct non_default_constructible
{
    int x; // NOLINT

    explicit non_default_constructible(int x) : x(x) {}

    friend bool operator==(non_default_constructible const &lhs,
                           non_default_constructible const &rhs)
    {
        return lhs.x == rhs.x;
    }
};

struct move_only_type
{
    move_only_type() : val(0) {}
    explicit move_only_type(int v) : val(v) {}
    ~move_only_type() = default;

    move_only_type(const move_only_type &) = delete;
    move_only_type &operator=(const move_only_type &) = delete;

    move_only_type &operator=(move_only_type &&) = default;
    move_only_type(move_only_type &&) = default;
    int val; // NOLINT
};

struct copy_and_movable_type
{
    explicit copy_and_movable_type(int v) : val(v) {}

    int val; // NOLINT
};