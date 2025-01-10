#include <algorithm>
#include <cassert>
#include <coroutine>
#include <exception>
#include <type_traits>
#include <variant>
#include <iostream>

template <typename TaskType>
struct promise_base;
template <template <typename> typename Task, typename T>
struct promise_base<Task<T>>
{
    using handle_type = std::coroutine_handle<promise_base>;

    Task<T> get_return_object() noexcept // NOLINT
    {
        return Task<T>{handle_type::from_promise(*this)};
    }

    constexpr std::suspend_always initial_suspend() noexcept // NOLINT
    {
        return {};
    }
    constexpr std::suspend_always final_suspend() noexcept // NOLINT
    {
        return {};
    }
    constexpr void unhandled_exception() noexcept // NOLINT
    {
        result_or_exception = std::current_exception();
    }
    constexpr void return_value(T &&t) noexcept // NOLINT
    {
        std::cout << "return_value called \n";
        result_or_exception = std::move(t);
    }

    template <std::convertible_to<T> From>
    std::suspend_always yield_value(From &&from) noexcept // NOLINT
    {
        std::cout << "yield_value called \n";
        result_or_exception = std::forward<From>(from); // 在承诺中缓存结果
        return {};
    }

    template <typename Other_Task>
    auto await_transform(Other_Task &&task) noexcept // NOLINT
    {
        struct Awaiter
        {
            Other_Task task; // NOLINT

            bool await_ready() const noexcept // NOLINT
            {
                return false; // 总是挂起
            }

            void await_suspend(std::coroutine_handle<> handle) noexcept // NOLINT
            {
                // handle.resume();      // 恢复当前协程
                task.handle.resume(); // 恢复 task
            }

            decltype(auto) await_resume() noexcept // NOLINT
            {
                return std::get<T>(task.handle.promise().result_or_exception); // 返回结果
            }
        };

        return Awaiter{std::move(task)}; // NOLINT
    }

    std::variant<T, std::exception_ptr> result_or_exception; // NOLINT
};
template <typename T>
struct task_base;

template <typename T>
struct task_base
{
    using promise_type = promise_base<task_base>;
    using handle_type = promise_type::handle_type;
    handle_type handle;
};

task_base<int> fun0() noexcept // NOLINT
{
    std::cout << "fun0\n";
    co_yield 1;
    co_yield 2;

    co_return 3;
};

task_base<int> fun1() noexcept // NOLINT
{
    std::cout << "fun1\n";
    auto r = co_await fun0();
    co_return r;
};

void test_base(); // NOLINT
void test_fun1(); // NOLINT
int main()
{
    // test_base();
    test_fun1();
    return 0;
}
void test_base()
{
    auto yield = [] {
        auto c = fun0();
        c.handle.resume();
        int ret = 0;
        auto test = [&] {
            std::visit(
                [&](auto &v) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(v)>, int>)
                    {
                        ret = v;
                    }
                },
                c.handle.promise().result_or_exception);
        };
        test();
        assert(ret == 1);
        c.handle.resume();
        test();
        assert(ret == 2);

        c.handle.resume();
        test();
        assert(ret == 3);
    };
    yield();
}

void test_fun1()
{
#if 0 
    std::cout << "\ntest_fun1:\n";
    auto c = fun1();   // 创建 fun1 的协程
    c.handle.resume(); // 启动协程

    int ret = 0;
    auto test = [&] {
        std::visit(
            [&](auto &v) {
                if constexpr (std::is_same_v<std::decay_t<decltype(v)>, int>)
                {
                    ret = v;
                }
            },
            c.handle.promise().result_or_exception);
    };

    // 第一次 resume 会触发 fun1 内部的 co_await fun0()
    // fun0 会 yield 1
    test();
    assert(ret == 1);

    // 第二次 resume 会继续 fun0，yield 2
    c.handle.resume();
    test();
    assert(ret == 2);

    // 第三次 resume 会继续 fun0，co_return 3
    c.handle.resume();
    test();
    assert(ret == 3);

    // 第四次 resume 会继续 fun1，co_return 3
    c.handle.resume();
    test();
    assert(ret == 3);
#endif
}