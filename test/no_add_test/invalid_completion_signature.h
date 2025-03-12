#pragma once

#include "completion_signatures.h"
#include <utility>

struct WITH_FUNCTION;
struct WITH_SENDER;
struct WITH_ARGUMENTS;
struct WITH_QUERY;

struct WITH_INFO;

template <const auto &>
struct IN_ALGORITHM;

struct dependent_sender_error
{
};

// NOLINTBEGIN
template <class... What>
struct sender_type_check_failure
{
    template <class... Info>
    consteval explicit sender_type_check_failure(Info &&.../*unused*/)
    {
    }
};
// NO defined replace throw
template <class... What, class... Info>
[[noreturn, nodiscard]] consteval completion_signatures<> invalid_completion_signature(
    Info &&...info);

// template <class... What, class... Info>
// [[noreturn, nodiscard]] consteval completion_signatures<> invalid_completion_signature(
//     Info &&...info)
// {
//     // TODO(mcs): c++26 才能捕获 consteval 的异常
//     throw sender_type_check_failure<What...>{std::forward<Info>(info)...};
// }