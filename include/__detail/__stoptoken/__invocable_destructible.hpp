#pragma once

#include <concepts>
namespace mcs::execution::stoptoken
{
    namespace __detail
    {
        template <typename T>
        concept invocable_destructible = std::invocable<T> && std::destructible<T>;
    };
}; // namespace mcs::execution::stoptoken