#pragma once

#include <optional>

namespace mcs::execution::task_v2::__detail
{
    template <typename T>
    struct pomise_return
    {
        template <std::convertible_to<T> V>
        constexpr void return_value(V &&value) noexcept // NOLINT
        {
            result_.emplace(std::forward<V>(value));
        }
        // exposition only; present only if is_void_v<T> is false;
        std::optional<T> result_; // NOLINT
    };
    template <>
    struct pomise_return<void>
    {
        static constexpr void return_void() noexcept // NOLINT
        {
        }
    };

}; // namespace mcs::execution::task_v2::__detail