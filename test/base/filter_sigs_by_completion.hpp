#pragma once
#include "../../include/execution.hpp"
#include <exception>
#include <iostream>

#include "./concat_same_list.hpp"

namespace ex = mcs::execution;

template <typename Tag, typename T>
struct same_completion
{
    constexpr static bool value = false; // NOLINT
};

template <typename Tag, typename... T>
struct same_completion<Tag, Tag(T...)>
{
    constexpr static bool value = true; // NOLINT
};

template <typename Tag, typename T>
struct filter_sigs_by_completion;

template <typename Tag, typename Cur, typename... T>
struct filter_sigs_by_completion<Tag, ex::cmplsigs::completion_signatures<Cur, T...>>
{
    using FilteredCur = std::conditional_t<same_completion<Tag, Cur>::value,
                                           ex::cmplsigs::completion_signatures<Cur>,
                                           ex::cmplsigs::completion_signatures<>>;

    using FilteredRest = typename filter_sigs_by_completion<
        Tag, ex::cmplsigs::completion_signatures<T...>>::type;

    // 合并结果
    using type = typename concat_same_list<ex::cmplsigs::completion_signatures,
                                           FilteredCur, FilteredRest>::type;
};

// 递归终止条件
template <typename Tag>
struct filter_sigs_by_completion<Tag, ex::cmplsigs::completion_signatures<>>
{
    using type = ex::cmplsigs::completion_signatures<>;
};

template <typename Tag, typename T>
struct skip_sigs_by_completion;

template <typename Tag, typename Cur, typename... T>
struct skip_sigs_by_completion<Tag, ex::cmplsigs::completion_signatures<Cur, T...>>
{
    using SkippedCur = std::conditional_t<same_completion<Tag, Cur>::value,
                                          ex::cmplsigs::completion_signatures<>,
                                          ex::cmplsigs::completion_signatures<Cur>>;

    using SkippedRest =
        typename skip_sigs_by_completion<Tag,
                                         ex::cmplsigs::completion_signatures<T...>>::type;

    // 合并结果
    using type = typename concat_same_list<ex::cmplsigs::completion_signatures,
                                           SkippedCur, SkippedRest>::type;
};

// 递归终止条件
template <typename Tag>
struct skip_sigs_by_completion<Tag, ex::cmplsigs::completion_signatures<>>
{
    using type = ex::cmplsigs::completion_signatures<>;
};

void test_skip(); // NOLINT

int main() // NOLINT
{
    test_skip();
    using T = ex::cmplsigs::completion_signatures<
        ex::recv::set_value_t(int), ex::recv::set_error_t(std::exception_ptr),
        ex::recv::set_value_t(double), ex::recv::set_error_t(int),
        ex::recv::set_value_t(float), ex::recv::set_stopped_t()>;

    // 过滤出所有 set_value_t 的签名
    using Filtered = filter_sigs_by_completion<ex::recv::set_value_t, T>::type;

    // 验证结果
    static_assert(
        std::is_same_v<
            Filtered, ex::cmplsigs::completion_signatures<ex::recv::set_value_t(int),
                                                          ex::recv::set_value_t(double),
                                                          ex::recv::set_value_t(float)>>);

    static_assert(
        std::is_same_v<
            filter_sigs_by_completion<ex::recv::set_error_t, T>::type,
            ex::cmplsigs::completion_signatures<ex::recv::set_error_t(std::exception_ptr),
                                                ex::recv::set_error_t(int)>>);

    static_assert(
        std::is_same_v<filter_sigs_by_completion<ex::recv::set_stopped_t, T>::type,
                       ex::cmplsigs::completion_signatures<ex::recv::set_stopped_t()>>);

    {
        using T = ex::cmplsigs::completion_signatures<
            ex::recv::set_value_t(int), ex::recv::set_error_t(std::exception_ptr),
            ex::recv::set_value_t(double), ex::recv::set_error_t(int),
            ex::recv::set_value_t(float)>;
        static_assert(
            std::is_same_v<filter_sigs_by_completion<ex::recv::set_stopped_t, T>::type,
                           ex::cmplsigs::completion_signatures<>>);
    }

    return 0;
}
inline void test_skip()
{
    using T = ex::cmplsigs::completion_signatures<
        ex::recv::set_value_t(int), ex::recv::set_error_t(std::exception_ptr),
        ex::recv::set_value_t(double), ex::recv::set_error_t(int),
        ex::recv::set_value_t(float), ex::recv::set_stopped_t()>;

    // 跳过所有 set_value_t 的签名
    using Skipped = skip_sigs_by_completion<ex::recv::set_value_t, T>::type;

    // 验证结果
    static_assert(
        std::is_same_v<Skipped,
                       ex::cmplsigs::completion_signatures<
                           ex::recv::set_error_t(std::exception_ptr),
                           ex::recv::set_error_t(int), ex::recv::set_stopped_t()>>);

    static_assert(
        std::is_same_v<skip_sigs_by_completion<ex::recv::set_error_t, T>::type,
                       ex::cmplsigs::completion_signatures<
                           ex::recv::set_value_t(int), ex::recv::set_value_t(double),
                           ex::recv::set_value_t(float), ex::recv::set_stopped_t()>>);

    static_assert(
        std::is_same_v<
            skip_sigs_by_completion<ex::recv::set_stopped_t, T>::type,
            ex::cmplsigs::completion_signatures<
                ex::recv::set_value_t(int), ex::recv::set_error_t(std::exception_ptr),
                ex::recv::set_value_t(double), ex::recv::set_error_t(int),
                ex::recv::set_value_t(float)>>);

    {
        using T = ex::cmplsigs::completion_signatures<
            ex::recv::set_value_t(int), ex::recv::set_error_t(std::exception_ptr),
            ex::recv::set_value_t(double), ex::recv::set_error_t(int),
            ex::recv::set_value_t(float)>;
        static_assert(
            std::is_same_v<
                skip_sigs_by_completion<ex::recv::set_stopped_t, T>::type,
                ex::cmplsigs::completion_signatures<
                    ex::recv::set_value_t(int), ex::recv::set_error_t(std::exception_ptr),
                    ex::recv::set_value_t(double), ex::recv::set_error_t(int),
                    ex::recv::set_value_t(float)>>);
    }
}