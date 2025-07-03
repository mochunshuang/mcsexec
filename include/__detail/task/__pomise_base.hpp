#pragma once

#include <system_error>
#include <utility>
#include <variant>

namespace mcs::execution::__task
{

    template <typename R>
    struct pomise_base
    {
        using value_type = std::remove_cvref_t<R>;
        using result_type =
            std::variant<std::monostate, value_type, std::exception_ptr, std::error_code>;
        result_type result;
        template <typename T>
        void return_value(T &&value) noexcept // NOLINT
        {
            this->result.template emplace<value_type>(std::forward<T>(value));
        }
    };
    template <>
    struct pomise_base<void>
    {
        struct void_t
        {
        };
        using value_type = void_t;
        using result_type =
            std::variant<std::monostate, void_t, std::exception_ptr, std::error_code>;
        result_type result;         // NOLINT
        void return_void() noexcept // NOLINT
        {
            this->result.template emplace<value_type>(void_t{});
        }
    };

}; // namespace mcs::execution::__task