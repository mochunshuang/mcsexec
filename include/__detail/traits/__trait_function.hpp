#pragma once

#include <tuple>

namespace mcs::execution::traits
{
    namespace __detail
    {
        template <typename _T>
        struct function_traits;

        // 成员函数模板特化
        template <typename _Ret, typename _Class, typename... _Args>
        struct function_traits<_Ret (_Class::*)(_Args...)>
        {
            using ret_t = _Ret;
            using arg_t = std::tuple<_Args...>;
            static constexpr bool is_noexcept = false; // NOLINT
            static constexpr bool is_const = false;    // NOLINT
            static constexpr bool is_lvalue = false;   // NOLINT
            static constexpr bool is_rvalue = false;   // NOLINT
        };

        template <typename _Ret, typename _Class, typename... _Args>
        struct function_traits<_Ret (_Class::*)(_Args...) noexcept>
        {
            using ret_t = _Ret;
            using arg_t = std::tuple<_Args...>;
            static constexpr bool is_noexcept = true; // NOLINT
            static constexpr bool is_const = false;   // NOLINT
            static constexpr bool is_lvalue = false;  // NOLINT
            static constexpr bool is_rvalue = false;  // NOLINT
        };

        template <typename _Ret, typename _Class, typename... _Args>
        struct function_traits<_Ret (_Class::*)(_Args...) const>
        {
            using ret_t = _Ret;
            using arg_t = std::tuple<_Args...>;
            static constexpr bool is_noexcept = false; // NOLINT
            static constexpr bool is_const = true;     // NOLINT
            static constexpr bool is_lvalue = false;   // NOLINT
            static constexpr bool is_rvalue = false;   // NOLINT
        };

        template <typename _Ret, typename _Class, typename... _Args>
        struct function_traits<_Ret (_Class::*)(_Args...) const noexcept>
        {
            using ret_t = _Ret;
            using arg_t = std::tuple<_Args...>;
            static constexpr bool is_noexcept = true; // NOLINT
            static constexpr bool is_const = true;    // NOLINT
            static constexpr bool is_lvalue = false;  // NOLINT
            static constexpr bool is_rvalue = false;  // NOLINT
        };

        template <typename _Ret, typename _Class, typename... _Args>
        struct function_traits<_Ret (_Class::*)(_Args...) &>
        {
            using ret_t = _Ret;
            using arg_t = std::tuple<_Args...>;
            static constexpr bool is_noexcept = false; // NOLINT
            static constexpr bool is_const = false;    // NOLINT
            static constexpr bool is_lvalue = true;    // NOLINT
            static constexpr bool is_rvalue = false;   // NOLINT
        };

        template <typename _Ret, typename _Class, typename... _Args>
        struct function_traits<_Ret (_Class::*)(_Args...) & noexcept>
        {
            using ret_t = _Ret;
            using arg_t = std::tuple<_Args...>;
            static constexpr bool is_noexcept = true; // NOLINT
            static constexpr bool is_const = false;   // NOLINT
            static constexpr bool is_lvalue = true;   // NOLINT
            static constexpr bool is_rvalue = false;  // NOLINT
        };

        template <typename _Ret, typename _Class, typename... _Args>
        struct function_traits<_Ret (_Class::*)(_Args...) const &>
        {
            using ret_t = _Ret;
            using arg_t = std::tuple<_Args...>;
            static constexpr bool is_noexcept = false; // NOLINT
            static constexpr bool is_const = true;     // NOLINT
            static constexpr bool is_lvalue = true;    // NOLINT
            static constexpr bool is_rvalue = false;   // NOLINT
        };

        template <typename _Ret, typename _Class, typename... _Args>
        struct function_traits<_Ret (_Class::*)(_Args...) const & noexcept>
        {
            using ret_t = _Ret;
            using arg_t = std::tuple<_Args...>;
            static constexpr bool is_noexcept = true; // NOLINT
            static constexpr bool is_const = true;    // NOLINT
            static constexpr bool is_lvalue = true;   // NOLINT
            static constexpr bool is_rvalue = false;  // NOLINT
        };

        template <typename _Ret, typename _Class, typename... _Args>
        struct function_traits<_Ret (_Class::*)(_Args...) &&>
        {
            using ret_t = _Ret;
            using arg_t = std::tuple<_Args...>;
            static constexpr bool is_noexcept = false; // NOLINT
            static constexpr bool is_const = false;    // NOLINT
            static constexpr bool is_lvalue = false;   // NOLINT
            static constexpr bool is_rvalue = true;    // NOLINT
        };

        template <typename _Ret, typename _Class, typename... _Args>
        struct function_traits<_Ret (_Class::*)(_Args...) && noexcept>
        {
            using ret_t = _Ret;
            using arg_t = std::tuple<_Args...>;
            static constexpr bool is_noexcept = true; // NOLINT
            static constexpr bool is_const = false;   // NOLINT
            static constexpr bool is_lvalue = false;  // NOLINT
            static constexpr bool is_rvalue = true;   // NOLINT
        };

        template <typename _Ret, typename _Class, typename... _Args>
        struct function_traits<_Ret (_Class::*)(_Args...) const &&>
        {
            using ret_t = _Ret;
            using arg_t = std::tuple<_Args...>;
            static constexpr bool is_noexcept = false; // NOLINT
            static constexpr bool is_const = true;     // NOLINT
            static constexpr bool is_lvalue = false;   // NOLINT
            static constexpr bool is_rvalue = true;    // NOLINT
        };

        template <typename _Ret, typename _Class, typename... _Args>
        struct function_traits<_Ret (_Class::*)(_Args...) const && noexcept>
        {
            using ret_t = _Ret;
            using arg_t = std::tuple<_Args...>;
            static constexpr bool is_noexcept = true; // NOLINT
            static constexpr bool is_const = true;    // NOLINT
            static constexpr bool is_lvalue = false;  // NOLINT
            static constexpr bool is_rvalue = true;   // NOLINT
        };

        // 全局函数模板特化
        template <typename _Ret, typename... _Args>
        struct function_traits<_Ret (*)(_Args...)>
        {
            using ret_t = _Ret;
            using arg_t = std::tuple<_Args...>;
            static constexpr bool is_noexcept = false; // NOLINT
            static constexpr bool is_const = false;    // NOLINT
            static constexpr bool is_lvalue = false;   // NOLINT
            static constexpr bool is_rvalue = false;   // NOLINT
        };

        template <typename _Ret, typename... _Args>
        struct function_traits<_Ret (*)(_Args...) noexcept>
        {
            using ret_t = _Ret;
            using arg_t = std::tuple<_Args...>;
            static constexpr bool is_noexcept = true; // NOLINT
            static constexpr bool is_const = false;   // NOLINT
            static constexpr bool is_lvalue = false;  // NOLINT
            static constexpr bool is_rvalue = false;  // NOLINT
        };

        template <typename _T>
        struct function_info_impl;

        template <typename _T>
        struct function_info_impl
        {
            using type = function_traits<_T>;
        };

        template <typename _T>
            requires(std::is_class_v<_T>) and requires {
                { &_T::operator() };
            }
        struct function_info_impl<_T>
        {
            using type = function_traits<decltype(&_T::operator())>;
        };

    }; // namespace __detail

    template <typename _F>
    using trait_function = __detail::function_info_impl<_F>::type;

}; // namespace mcs::execution::traits