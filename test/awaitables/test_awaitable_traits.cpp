#include <coroutine>
#include <type_traits>
#include <iostream>

// NOLINTBEGIN

// 基础可等待对象
template <typename T>
struct SimpleAwaiter
{
    T value;
    bool await_ready() const noexcept
    {
        return false;
    }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    T await_resume() const noexcept
    {
        return value;
    }
};

// 可转换为可等待对象的类型（有成员operator co_await）
struct MyInt
{
    int value;
    SimpleAwaiter<int> operator co_await() const
    {
        return {value};
    }
};

// 自身即为可等待对象的类型（无operator co_await，但有await方法）
struct SelfAwaiter
{
    int value;
    bool await_ready() const noexcept
    {
        return false;
    }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    int await_resume() const noexcept
    {
        return value;
    }
};

// 辅助模板：解决重载歧义，明确优先级
namespace detail
{
    // 概念：检查T是否有成员operator co_await
    template <typename T>
    concept HasMemberCoAwait = requires(const T &t) {
        { t.operator co_await() };
    };

    // 概念：检查T是否有非成员operator co_await
    template <typename T>
    concept HasNonMemberCoAwait = requires(const T &t) {
        { operator co_await(t) };
    };

    // 概念：检查T是否自身就是可等待对象（有三个必要方法）
    template <typename T>
    concept IsSelfAwaitable = requires(const T &t, std::coroutine_handle<> h) {
        { t.await_ready() } -> std::same_as<bool>;
        t.await_suspend(h);
        t.await_resume();
    };

    // 1. 优先匹配：有成员operator co_await
    template <HasMemberCoAwait T>
    auto get_awaitable_type(const T &t) -> decltype(t.operator co_await())
    {
        return t.operator co_await();
    }

    // 2. 其次匹配：有非成员operator co_await（无成员版本时）
    template <HasNonMemberCoAwait T>
    auto get_awaitable_type(const T &t) -> decltype(operator co_await(t))
    {
        return operator co_await(t);
    }

    // 3. 最后匹配：自身是可等待对象（前两种都不满足时）
    template <IsSelfAwaitable T>
    auto get_awaitable_type(const T &t) -> const T &
    {
        return t;
    }

    // 推导co_await t的结果类型
    template <typename T>
    using await_result_t = decltype(get_awaitable_type(std::declval<T>()).await_resume());
} // namespace detail

// 概念：co_await t的结果类型为int
template <typename T>
concept returnsInt = std::is_same_v<detail::await_result_t<T>, int>;

// 测试函数（仅接受符合returnsInt约束的类型）
template <returnsInt T>
void test(const T &t)
{
    std::cout << "符合约束：co_await结果为int\n";
}

int main()
{
    MyInt obj1{42};        // 有成员operator co_await
    SelfAwaiter obj2{100}; // 自身是可等待对象

    test(obj1); // 编译通过：匹配成员operator co_await路径
    test(obj2); // 编译通过：匹配自身可等待对象路径

    // 验证类型推导
    static_assert(std::is_same_v<detail::await_result_t<MyInt>, int>);
    static_assert(std::is_same_v<detail::await_result_t<SelfAwaiter>, int>);

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND