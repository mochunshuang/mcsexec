#include <concepts>
#include <exception>
#include <functional>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

#include "./completion_signatures.h"
#include "./transform_completion_signatures.h"

// NOLINTBEGIN

auto fun = [](auto &&p) {
    if constexpr (std::is_same_v<decltype(auto(p)), int>)
        return 1;
    else if constexpr (std::is_same_v<decltype(auto(p)), double>)
        return std::string("hello");
};
static_assert(std::is_same_v<decltype(fun(1)), int>);
static_assert(std::is_same_v<decltype(fun(1.0)), std::string>);

template <typename Fun, typename T, typename Sigs>
consteval auto transform_sigs_with_fun() // NOLINT
{
    auto cs = Sigs{};
    auto transform1 = []<class Tag, class... As>(Tag (*)(As...)) {
        if constexpr (std::is_same_v<Tag, T>)
            return completion_signatures<Tag(
                decltype(std::invoke(std::declval<Fun>(), std::declval<As>()...)))>{};
        else
            return completion_signatures<Tag(As...)>{};
    };
    auto transform_all = [=](auto *...sigs) {
        return (completion_signatures<>{} + ... + transform1(sigs));
    };
    return __apply(transform_all, cs);
};

int main()
{
    completion_signatures<set_value_t(int), set_value_t(double),
                          set_error_t(std::exception_ptr)>
        cs{};

    using Target = completion_signatures<set_value_t(int), set_value_t(std::string),
                                         set_error_t(std::exception_ptr)>;

    using Fun_t = decltype(fun);

    using T = decltype(transform_sigs_with_fun<Fun_t, set_value_t, decltype(cs)>());

    static_assert(std::is_same_v<Target, T>);
    {
        static constexpr auto V_transform = []<class... As>() { // NOLINT
            return completion_signatures<set_value_t(
                decltype(std::invoke(std::declval<Fun_t>(), std::declval<As>()...)))>{};
        };
        static_assert(std::is_same_v<T, decltype(transform_completion_signatures(
                                            cs, V_transform))>);
    }
    {
        using AS [[maybe_unused]] = int &;
        using T0 = int;
        using T1 = int &&;
        auto fun = [](int v) {
        };
        using Fun_t = decltype(fun);
        // NOTE: 调用失败，严格性OK
        // using ret = decltype(std::invoke(std::declval<Fun_t>(), std::declval<As>()));

        using ret = decltype(std::invoke(std::declval<Fun_t>(), std::declval<T0>()));
        using ret [[maybe_unused]] =
            decltype(std::invoke(std::declval<Fun_t>(), std::declval<T1>()));
        static_assert(std::is_same_v<void, ret>);
    }
    // NOTE: decltype 不会丢失 引用信息
    {
        auto fun = [](int &v) -> int & {
            return v;
        };
        using Fun_t = decltype(fun);
        using R = decltype(std::declval<Fun_t>()(std::declval<int &>()));

        // NOTE: invocable 也需要精确到 引用类型
        {
            static_assert(std::invocable<Fun_t, int &>);
            static_assert(not std::invocable<Fun_t, int>);
        }
        // NOTE: invoke_result_t 也需要精确到 引用类型,不会丢失返回值 cf
        {
            static_assert(std::is_same_v<std::invoke_result_t<Fun_t, int &>, R>);
        }

        static_assert(std::is_same_v<R, int &>);
        {
            auto fun = [](int &&v) -> int && {
                return v;
            };
            using Fun_t = decltype(fun);
            using R = decltype(std::declval<Fun_t>()(std::declval<int &&>()));
            static_assert(std::is_same_v<R, int &&>);
            static_assert(std::is_same_v<std::invoke_result_t<Fun_t, int &&>, R>);
        }
        {
            auto fun = [](const int &v) -> const int & {
                return v;
            };
            using Fun_t = decltype(fun);
            using R = decltype(std::declval<Fun_t>()(std::declval<int>()));
            static_assert(std::is_same_v<R, const int &>);
            static_assert(std::is_same_v<std::invoke_result_t<Fun_t, int &>, R>);
        }
    }

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND