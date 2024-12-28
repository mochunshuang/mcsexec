
#include <functional>
#include <memory>
#include <cstddef>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>

namespace mcs::execution::snd::__detail
{
    template <::std::size_t, typename T>
    struct product_type_element
    {
        T value; // NOLINT
        auto operator==(const product_type_element &) const -> bool = default;
    };

    template <typename, typename...>
    struct product_type_base;

    template <::std::size_t... I, typename... T>
    struct product_type_base<::std::index_sequence<I...>, T...>
        : ::mcs::execution::snd::__detail::product_type_element<I, T>...
    {
        static constexpr ::std::size_t size()
        {
            return sizeof...(T);
        }

        template <::std::size_t J, typename S>
        static auto element_get( // NOLINT
            ::mcs::execution::snd::__detail::product_type_element<J, S> &self) noexcept
            -> S &
        {
            return self.value;
        }
        template <::std::size_t J, typename S>
        static auto element_get( // NOLINT
            ::mcs::execution::snd::__detail::product_type_element<J, S> &&self) noexcept
            -> S &&
        {
            return ::std::move(self.value);
        }
        template <::std::size_t J, typename S>
        static auto element_get( // NOLINT
            const ::mcs::execution::snd::__detail::product_type_element<J, S>
                &self) noexcept -> const S &
        {
            return self.value;
        }

        template <::std::size_t J>
        auto get() & -> decltype(auto)
        {
            return this->element_get<J>(*this);
        }
        template <::std::size_t J>
        auto get() && -> decltype(auto)
        {
            return this->element_get<J>(::std::move(*this));
        }
        template <::std::size_t J>
        [[nodiscard]] auto get() const & -> decltype(auto)
        {
            return this->element_get<J>(*this);
        }

        template <::std::size_t J, typename Allocator, typename Self> // NOLINTNEXTLINE
        static auto make_element(Allocator &&alloc, Self &&self) -> decltype(auto)
        {
            using type = ::std::remove_cvref_t<decltype(product_type_base::element_get<J>(
                std::forward<Self>(self)))>;
            if constexpr (::std::uses_allocator_v<type, Allocator>)
                return ::std::make_obj_using_allocator<type>(
                    alloc, product_type_base::element_get<J>(std::forward<Self>(self)));
            else
                return product_type_base::element_get<J>(std::forward<Self>(self));
        }

        auto operator==(const product_type_base &) const -> bool = default;
    };

    template <typename... T>
    struct product_type : ::mcs::execution::snd::__detail::product_type_base<
                              ::std::index_sequence_for<T...>, T...>
    {

        template <typename Allocator, typename Product, std::size_t... I>
        static auto make_from(Allocator &&allocator, Product &&product, // NOLINT
                              std::index_sequence<I...> /*unused*/) -> product_type
        {
            return {product_type::template make_element<I>(
                allocator, ::std::forward<Product>(product))...};
        }

        template <typename Allocator, typename Product> // NOLINTNEXTLINE
        static auto make_from(Allocator &&allocator, Product &&product) -> product_type
        {
            return product_type::make_from(::std::forward<Allocator>(allocator),
                                           ::std::forward<Product>(product),
                                           ::std::index_sequence_for<T...>{});
        }

        template <typename Fun, ::std::size_t... I> // NOLINTNEXTLINE
        constexpr auto apply_elements(::std::index_sequence<I...>,
                                      Fun &&fun) const -> decltype(auto)
        {
            return ::std::forward<Fun>(fun)(this->template get<I>()...);
        }
        template <typename Fun>
        constexpr auto apply(Fun &&fun) const -> decltype(auto)
        {
            return apply_elements(::std::index_sequence_for<T...>{},
                                  ::std::forward<Fun>(fun));
        }
        template <typename Fun, ::std::size_t... I> // NOLINTNEXTLINE
        constexpr auto apply_elements(::std::index_sequence<I...>,
                                      Fun &&fun) -> decltype(auto)
        {
            return ::std::forward<Fun>(fun)(this->template get<I>()...);
        }
        template <typename Fun>
        constexpr auto apply(Fun &&fun) -> decltype(auto)
        {
            return apply_elements(::std::index_sequence_for<T...>{},
                                  ::std::forward<Fun>(fun));
        }
    };
    template <typename... T>
    product_type(T &&...) -> product_type<::std::decay_t<T>...>;

    // helper
    template <typename T>
    constexpr bool is_product_type_v = false; // NOLINT

    template <typename... U>
    constexpr bool // NOLINTNEXTLINE
        is_product_type_v<::mcs::execution::snd::__detail::product_type<U...>> = true;

    template <typename T>
    concept is_product_type = is_product_type_v<T>;

}; // namespace mcs::execution::snd::__detail

namespace std
{
    template <typename... T>
    struct tuple_size<::mcs::execution::snd::__detail::product_type<T...>> // NOLINT
        : ::std::integral_constant<std::size_t, sizeof...(T)>
    {
    };

    template <::std::size_t I, typename... T>
    struct tuple_element<I, ::mcs::execution::snd::__detail::product_type<T...>> // NOLINT
    {
        using type = ::std::decay_t<
            decltype(::std::declval<::mcs::execution::snd::__detail::product_type<T...>>()
                         .template get<I>())>;
    };
} // namespace std

namespace std
{
    // 如何整合以下4个目标函数 为一个模板
    // 仅仅针对 is_product_type_v的概念特化
    template <std::size_t I, typename T>
        requires ::mcs::execution::snd::__detail::is_product_type<
                     ::std::remove_cvref_t<T>>
    constexpr auto get(T &&t) noexcept -> decltype(auto) // NOLINT
    {
        return std::forward<T>(t).template get<I>();
    }

    // TODO(mcs): 为了apply 的概念能通过，源码是未完成的
    template <typename... _Elements>
    inline constexpr bool // NOLINTNEXTLINE
        __is_tuple_like_v<::mcs::execution::snd::__detail::product_type<_Elements...>> =
            true;
}; // namespace std

namespace std
{
#if 0
    // 为 product_type 提供 std::get<I> 的特化
    template <std::size_t I, typename... Ts>
    constexpr auto get(const mcs::execution::snd::__detail::product_type<Ts...>
                           &pt) noexcept -> decltype(auto)
    {
        return std::as_const(pt).template get<I>();
    }

    template <std::size_t I, typename... T>
    constexpr auto get(mcs::execution::snd::__detail::product_type<T...> &pt) noexcept
        -> decltype(auto)
    {
        return pt.template get<I>();
    }

    template <std::size_t I, typename... T>
    constexpr auto get(mcs::execution::snd::__detail::product_type<T...> &&pt) noexcept
        -> decltype(auto)
    {
        return std::move(pt).template get<I>();
    }

    template <std::size_t I, typename... T>
    constexpr auto get(const mcs::execution::snd::__detail::product_type<T...>
                           &&pt) noexcept -> decltype(auto)
    {
        return std::move(pt).template get<I>();
    }
#endif
}; // namespace std

#include <iostream>

void test5_base();
void test5_invoke_and_apply();
void test5_get();
int main()
{
    test5_base(); // 正常编译
    test5_invoke_and_apply();
    test5_get();
    std::cout << "hello world\n";
    return 0;
}

void test5_base()
{
    // 使用 std::tuple 进行测试
    auto t = std::tuple{42, 3.14, "Hello, World!"};

    static_assert(std::is_same_v<decltype(t), std::tuple<int, double, const char *>>);

    // 定义一个函数，接受三个参数
    auto print_values = [](int i, double d, const std::string &s) {
        std::cout << "int: " << i << ", double: " << d << ", string: " << s << std::endl;
    };

    // 使用 std::apply 调用函数，传递 std::tuple 的元素
    std::apply(print_values, t);
}

void foo(std::size_t size)
{
    // 处理 size
    (void)size;
}

void test5_invoke_and_apply()
{
    using namespace mcs::execution::snd::__detail;
    // 创建一个 product_type 实例
    auto pt = product_type{42, 3.14, "Hello, World!"};
    [[maybe_unused]] auto [a, b, c] = pt;
    static_assert(std::is_same_v<decltype(pt),
                                 product_type<::std::decay_t<int>, ::std::decay_t<double>,
                                              ::std::decay_t<const char(&)[14]>>>);

    // 定义一个函数，接受三个参数
    auto print_values = [](int i, double d, const std::string &s) {
        std::cout << "int: " << i << ", double: " << d << ", string: " << s << std::endl;
    };

    // 使用 std::invoke 调用函数，传递 product_type 的元素
    std::invoke(print_values, pt.get<0>(), pt.get<1>(), pt.get<2>());
    std::invoke(print_values, std::get<0>(pt), std::get<1>(pt), std::get<2>(pt));

    static_assert(
        std::is_same_v<decltype(pt), mcs::execution::snd::__detail::product_type<
                                         int, double, const char *>>);
    int v [[maybe_unused]] = std::get<0>(pt);

    // 以上都满足，就下面不行
    using _Tuple = decltype(pt);
    using _Indices [[maybe_unused]] =
        std::make_index_sequence<std::tuple_size_v<std::remove_reference_t<_Tuple>>>;
    static_assert(std::is_same_v<_Indices, std::integer_sequence<std::size_t, 0, 1, 2>>);

    // TODO(mcs): 这是bug 吧
    static_assert(not std::is_same_v<unsigned int, std::size_t>);
    unsigned int value = 10;
    foo(value); // 如果 std::size_t 和 unsigned int 相同，这里可能会导致问题

    // TODO(mcs): 源码看到概念tuple-like(c++23)的没写完。
    // std::apply(print_values, pt); // 编译错误
}

void test5_get()
{
    // 测试 get 的完美转发
    {
        // NOLINTNEXTLINE
        auto t = mcs::execution::snd::__detail::product_type{42, 3.14, "Hello, World!"};

        // 测试左值引用
        [[maybe_unused]] auto a = std::get<0>(t);

        // 测试右值引用
        [[maybe_unused]] auto b = std::get<1>(std::move(t));
    }

    // noexcept 特性
    {
        // NOLINTNEXTLINE
        auto t = mcs::execution::snd::__detail::product_type{42, 3.14, "Hello, World!"};

        static_assert(noexcept(std::get<0>(t)), "get<0> should be noexcept");
        static_assert(noexcept(std::get<1>(std::move(t))), "get<1> should be noexcept");
    }
    // get 的类型推导
    {
        auto t = mcs::execution::snd::__detail::product_type{42, 3.14, "Hello, World!"};
        static_assert(std::is_same_v<decltype(std::get<0>(t)), int &>,
                      "get<0> should return int&");
        // 平凡类型
        static_assert(std::is_trivially_copyable_v<decltype(t)>,
                      "t1 should be trivially copyable");

        // TODO(mcs): 和 tuple 不一致。 原因是 std::move(t) 无效。 t 是平凡类型
        // Note: 原因是 返回值应该是 S&& 而不是 S,不使用返回值优化
        static_assert(std::is_same_v<decltype(std::get<1>(std::move(t))), double &&>,
                      "get<1> should return double&&");

        {
            std::tuple<int, double, std::string> t = {42, 3.14, "Hello"};

            // 测试类型推导
            static_assert(std::is_same_v<decltype(std::get<0>(t)), int &>,
                          "get<0> should return int&");
            static_assert(std::is_same_v<decltype(std::get<1>(std::move(t))), double &&>,
                          "get<1> should return double&&");
        }
    }
    {
        auto t = mcs::execution::snd::__detail::product_type{
            42, 3.14, std::string("Hello, World!")};
        static_assert(std::is_same_v<decltype(std::get<0>(t)), int &>,
                      "get<0> should return int&");
        static_assert(not std::is_trivially_copyable_v<decltype(t)>,
                      "t1 should be trivially copyable");

        static_assert(std::is_same_v<decltype(std::get<1>(std::move(t))), double &&>,
                      "get<1> should return double&&");
        static_assert(std::is_same_v<decltype(std::move(t).get<1>()), double &&>,
                      "get<1> should return double&&");

        {
            std::tuple<int, double, std::string> t = {42, 3.14,
                                                      std::string("Hello, World!")};

            // 测试类型推导
            static_assert(std::is_same_v<decltype(std::get<0>(t)), int &>,
                          "get<0> should return int&");
            static_assert(std::is_same_v<decltype(std::get<1>(std::move(t))), double &&>,
                          "get<1> should return double&&");
        }
    }
}