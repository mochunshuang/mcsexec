#include <exception>
#include <tuple>
#include <iostream>
#include <type_traits>
#include <utility>
#include <variant>

#include "../test_base_head.hpp"

struct set_error_t
{
};

int main()
{
    using T0 = std::tuple<set_error_t, std::exception_ptr>;
    using T1 = std::tuple<set_error_t, std::exception_ptr>;
    static_assert(
        std::is_nothrow_constructible_v<std::tuple<set_error_t, std::exception_ptr>,
                                        set_error_t, std::exception_ptr>);

    using T = decltype(std::tuple_cat(std::declval<T0>(), std::declval<T0>()));
    static_assert(std::is_same_v<T, std::tuple<set_error_t, std::exception_ptr,
                                               set_error_t, std::exception_ptr>>);

    using V = std::variant<std::monostate, T0, T1>;
    static_assert(
        std::is_same_v<
            V, std::variant<std::monostate, std::tuple<set_error_t, std::exception_ptr>,
                            std::tuple<set_error_t, std::exception_ptr>>>);

    using V0 = ex::tfxcmplsigs::unique_variadic_template<V>::type;
    static_assert(
        std::is_same_v<V0, std::variant<std::monostate,
                                        std::tuple<set_error_t, std::exception_ptr>>>);

    // NOTE: tuple 可以存放指针
    std::tuple<V0 *> t{};
    using TT = decltype(std::get<0>(t));
    static_assert(std::is_same_v<std::remove_cvref_t<TT>, V0 *>);

    auto fun = [] consteval {
        struct A
        {
            using type = int;
            bool no_throw{};
        };
        return A{true};
    };
    constexpr auto r = fun();
    static_assert(std::is_same_v<decltype(r)::type, int>);
    if constexpr (r.no_throw)
    {
        std::cout << "consteval is good : " << r.no_throw << '\n';
    }
    { // std::tuple<int &> t; // 失败的
        constexpr auto b = std::is_nothrow_constructible_v<std::tuple<int &>, int &>;
        static_assert(b);
        // 其中 int & 是不会出现的，因为
        /*
            template <class... Ts>
            using decayed_tuple = std::tuple<std::decay_t<Ts>...>;
        */
        // NOTE: 对于 decayed_tuple 开起来不用添加参数了
        //  static_assert(std::is_nothrow_constructible_v<std::tuple<int &>>);
        static_assert(std::is_nothrow_constructible_v<std::tuple<int>>); // 因此不用判断
        static_assert(std::is_nothrow_constructible_v<std::tuple<int>, int>);
    }

    std::cout << "main done\n";
    return 0;
}