#include <concepts>
#include <iostream>
#include <type_traits>
#include <cassert>
#include <tuple>
#include <utility>

namespace mcs::execution
{
    /**
     * @brief 多继承代替tuple. 要求能解绑. 并且要求能够复制消除
     *
     */
    template <std::size_t, typename T>
    struct product_type_element
    {
        T value; // NOLINT
    };

    template <typename...>
    struct product_type_base;

    template <class... T, std::size_t... I>
    struct product_type_base<std::index_sequence<I...>, T...>
        : product_type_element<I, T>...
    {
        static constexpr ::std::size_t size()
        {
            return sizeof...(T);
        }

        template <::std::size_t J, typename S>
        static auto element_get( // NOLINT
            product_type_element<J, S> &self) noexcept -> S &
        {
            return self.value;
        }
        template <::std::size_t J, typename S>
        static auto element_get( // NOLINT
            product_type_element<J, S> &&self) noexcept -> S
        {
            return ::std::move(self.value);
        }
        template <::std::size_t J, typename S>
        static auto element_get( // NOLINT
            product_type_element<J, S> const &self) noexcept -> S const &
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
    };

    template <class... T>
    struct product_type : product_type_base<std::index_sequence_for<T...>, T...>
    {
        template <class Self, class Fn>
            requires ::std::invocable<Fn, T &...>
        constexpr decltype(auto) apply(this Self &&self, Fn &&fn) noexcept(
            ::std::is_nothrow_invocable_v<Fn, T &...>)
        {
            return
                [&]<::std::size_t... I>(::std::index_sequence<I...>) -> decltype(auto) {
                    // TODO(mcs): clangd 目前不支持forward_like,不能编译
                    return ::std::forward<Fn>(fn)(
                        ::std::forward_like<Self>(self.template get<I>())...);
                    // return ::std::forward<Fn>(fn)(
                    //     ::std::forward<Self>(self).template get<I>()...);
                }(::std::index_sequence_for<T...>{});
        }
    };

    // Deduction guide
    template <typename... T>
    product_type(T &&...) -> product_type<std::decay_t<T>...>;

    template <class Tag, class Data, class... Child>
    struct basic_sender : product_type<Tag, Data, Child...> // exposition only
    {

        using sender_concept = int;
        using indices_for = std::index_sequence_for<Child...>; // exposition only
    };
}; // namespace mcs::execution

namespace std
{
    template <typename... T>
    struct tuple_size<::mcs::execution::product_type<T...>>
    {
        static constexpr size_t value = sizeof...(T);
    };

    template <size_t I, typename... T>
    struct tuple_element<I, ::mcs::execution::product_type<T...>>
    {
        using type = typename std::tuple_element<I, std::tuple<T...>>::type;
    };

}; // namespace std

namespace std
{
    template <typename... T>
    struct tuple_size<::mcs::execution::basic_sender<T...>>
        : tuple_size<::mcs::execution::product_type<T...>>
    {
    };

    template <::std::size_t I, typename... T>
    struct tuple_element<I, ::mcs::execution::basic_sender<T...>>
        : tuple_element<I, ::mcs::execution::product_type<T...>>
    {
    };

}; // namespace std

struct no_move_no_copy
{
    int age; // NOLINT
    explicit no_move_no_copy(int age) : age{age} {}

    no_move_no_copy(const no_move_no_copy &) = delete;
    no_move_no_copy &operator=(const no_move_no_copy &) = delete;
    no_move_no_copy(no_move_no_copy &&) = delete;
    no_move_no_copy &operator=(no_move_no_copy &&) = delete;
    ~no_move_no_copy() = default;
};

decltype(auto) test_immove(int age)
{
    return no_move_no_copy(age);
}
////////////////////
struct empty_data
{
};

template <typename T>
concept movable_value = std::movable<T>;

template <typename>
concept sender = true;

template <std::semiregular Tag, movable_value Data = empty_data, sender... Child>
constexpr auto make_sender(Tag tag, Data &&data = empty_data{},
                           Child &&...child) -> decltype(auto)
{
    return mcs::execution::basic_sender<std::decay_t<Tag>, std::decay_t<Data>,
                                        std::decay_t<Child>...>{
        tag, std::forward<Data>(data), std::forward<Child>(child)...};
}

struct __just_t
{
    template <movable_value... Ts>
    constexpr auto operator()(Ts &&...ts) const noexcept
    {
        return make_sender(*this, mcs::execution::product_type{std::forward<Ts>(ts)...});
    }
};

void test_tuple();

template <typename T>
struct test_bind
{
    T value; // NOLINT

    // 默认构造函数
    test_bind() = default;
    ~test_bind() = default;

    // 拷贝构造函数
    test_bind(const test_bind &other) : value(other.value)
    {
        std::cout << "test_bind(const test_bind &other)\n";
    }

    // 移动构造函数
    test_bind(test_bind &&other) noexcept : value(std::move(other.value))
    {
        std::cout << "test_bind(test_bind &&other)\n";
    }

    // 拷贝赋值运算符
    test_bind &operator=(const test_bind &other)
    {
        if (this != &other)
        {
            value = other.value;
            std::cout << "test_bind &operator=(const test_bind &other)\n";
        }
        return *this;
    }

    // 移动赋值运算符
    test_bind &operator=(test_bind &&other) noexcept
    {
        if (this != &other)
        {
            value = std::move(other.value);
            std::cout << "test_bind &operator=(test_bind &&other)\n";
        }
        return *this;
    }

    // 带参数的构造函数
    explicit test_bind(T val) : value(val)
    {
        std::cout << "explicit test_bind(T val)\n";
    }
};

int main()
{
    test_tuple();

    using namespace mcs::execution;

    auto obj = test_immove(1);
    assert(obj.age == 1);

    {
        // 使用聚合初始化构造 product_type 对象
        auto obj = product_type{test_immove(1), test_immove(2), test_immove(3)};
        // 由于 no_move_no_copy 不能复制或移动，这里无法直接访问 age
        // assert(obj.age == 1);

        // 使用 get 函数访问元素
        auto &a = obj.get<0>();
        {
            auto &[a, b, c] = obj; // 现在应该可以工作
        }

        {
            auto just = product_type{1, 2, 3};
            auto &[a, b, c] = just;
        }
        {
            /**
             * @brief basic_sender 结构化绑定测试
             *
             */
            auto just = __just_t{}(1, 2, 3);
            auto &[tag, data] = just;
        }
        {
            // 不能移动，因此不能满足
            // auto just = __just_t{}(test_immove(1), test_immove(2), test_immove(3));
            // auto &[tag, data] = just;
        }
        assert(a.age == 1);
    }
    {
        // 能否支持 std::apply() ?
        auto args = product_type{1, 2, 3};
        args.apply([](int &a, int &b, int &c) {
            assert(a == 1);
            assert(b == 2);
            assert(c == 3);
            std::cout << "apply done\n";
        });
    }
    {
        std::cout << "\nproduct_type: test_bind\n";
        auto args = mcs::execution::product_type{test_bind<int>{1}, test_bind<int>{2}};

        auto [a, b] = args;   // copy
        auto &[c, d] = args;  // no copy
        auto &&[e, f] = args; // no copy

        static_assert(not std::is_same_v<decltype(e), test_bind<int> &&>);
        std::cout << "\nproduct_type: test_bind end\n";
    }
    {
        std::cout << "\nbasic_sender: test_bind\n";
        auto args = mcs::execution::basic_sender<test_bind<int>, test_bind<int>>{
            test_bind<int>{1}, test_bind<int>{2}};

        auto [a, b] = args;   // copy
        auto &[c, d] = args;  // no copy
        auto &&[e, f] = args; // move

        static_assert(not std::is_same_v<decltype(e), test_bind<int> &&>);
        std::cout << "\nbasic_sender: test_bind end\n";
    }
    {
        std::cout << "\nbasic_sender: test_apply\n";

        auto args = mcs::execution::basic_sender<test_bind<int>, test_bind<int>>{
            test_bind<int>{1}, test_bind<int>{2}};

        std::cout << "\nbasic_sender: test_apply start \n";
        using T = decltype(static_cast<decltype(args)>(args));
        static_assert(
            std::is_same_v<mcs::execution::basic_sender<test_bind<int>, test_bind<int>>,
                           T>);
        using LT = mcs::execution::basic_sender<test_bind<int>, test_bind<int>> &;
        using RT = mcs::execution::basic_sender<test_bind<int>, test_bind<int>> &&;
        // Note: forward_like: copy。 forward: copy and move
        static_cast<T>(args).apply([](auto &&a, auto &&b) {
            assert(a.value == 1);
            assert(b.value == 2);
            std::cout << "end T\n";
        });

        // Note: forward_like: Nothing。
        static_cast<LT>(args).apply([](auto &&a, auto &&b) {
            assert(a.value == 1);
            assert(b.value == 2);
            std::cout << "end LT\n";
        });

        // Note: forward_like: Nothing。
        static_cast<RT>(args).apply([](auto &&a, auto &&b) {
            assert(a.value == 1);
            assert(b.value == 2);
            std::cout << "end RT\n";
        });

        std::cout << "\nbasic_sender: test_apply end\n";
    }

    /**
     * @brief 因此知道，结构化绑定 和 apply 是两种东西。 不是等价的
     *
     */

    std::cout << "\ndone\n";
    return 0;
}

void test_tuple()
{
    std::tuple args{1, 2, 3};
    auto t = std::tuple<no_move_no_copy, no_move_no_copy, no_move_no_copy>{1, 2, 3};
    auto &a = std::get<0>(t);
    assert(a.age == 1);
    static_assert(std::is_same_v<decltype(a), no_move_no_copy &>);

    // tuple 支持std::apply
    std::apply([](int, int, int) { std::cout << "call done\n"; }, args);

    // 测试结构化绑定
    {
        std::cout << "\nstd::tuple: test_bind\n";
        auto args = std::tuple{test_bind<int>{1}, test_bind<int>{2}};
        auto [a, b] = args;   // copy
        auto &[c, d] = args;  // no copy
        auto &&[e, f] = args; // no copy
        std::cout << "\nstd::tuple: test_bind end\n";
    }
    std::cout << "\n";
}