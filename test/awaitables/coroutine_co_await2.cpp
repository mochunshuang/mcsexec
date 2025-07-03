#include <iostream>
#include <coroutine>
#include <thread>
#include <chrono>

// NOLINTBEGIN

// 基础Awaiter
template <typename T = void>
struct SimpleAwaiter
{
    T value{};

    bool await_ready() const noexcept
    {
        std::cout << "  await_ready()\n";
        return false;
    }

    void await_suspend(std::coroutine_handle<> h) const noexcept
    {
        std::cout << "  await_suspend() - resuming immediately\n";
        h.resume();
    }

    T await_resume() const noexcept
    {
        std::cout << "  await_resume()\n";
        return value;
    }
};

// 1. 直接作为awaiter的类型
struct DirectAwaiter
{
    int value;

    DirectAwaiter(int v) : value(v) {}

    bool await_ready() const noexcept
    {
        std::cout << "  [DirectAwaiter] await_ready()\n";
        return false;
    }

    void await_suspend(std::coroutine_handle<> h) const noexcept
    {
        std::cout << "  [DirectAwaiter] await_suspend()\n";
        h.resume();
    }

    int await_resume() const noexcept
    {
        std::cout << "  [DirectAwaiter] await_resume()\n";
        return value;
    }
};

// 2. 通过成员operator co_await转换的类型
struct MyInt
{
    int value;

    MyInt(int v) : value(v) {}

    SimpleAwaiter<int> operator co_await() const
    {
        std::cout << ">>> [1] 成员operator co_await()\n";
        return {value};
    }
};

// 3. 通过非成员operator co_await转换的类型
struct MyFloat
{
    float value;

    MyFloat(float v) : value(v) {}
};

SimpleAwaiter<float> operator co_await(MyFloat f)
{
    std::cout << ">>> [2] 非成员operator co_await()\n";
    return {f.value};
}

// 4. 仅通过promise的await_transform转换的类型
struct MyDouble
{
    double value;

    MyDouble(double v) : value(v) {}
};

// 协程返回类型
template <typename T>
struct Task
{
    struct promise_type
    {
        Task get_return_object()
        {
            return {};
        }
        std::suspend_never initial_suspend()
        {
            return {};
        }
        std::suspend_never final_suspend() noexcept
        {
            return {};
        }
        void return_void() {}
        void unhandled_exception() {}

        // 仅处理MyDouble类型
        auto await_transform(MyDouble d)
        {
            std::cout << ">>> [3] promise_type::await_transform()\n";
            return SimpleAwaiter{d.value};
        }

        // 转发其他类型，不做处理
        template <typename U>
        decltype(auto) await_transform(U &&u)
        {
            return std::forward<U>(u);
        }
    };
};

// 测试协程
Task<void> test_coroutine()
{
    std::cout << "Coroutine started\n\n";

    // 1. 直接使用awaiter
    std::cout << "测试1: 直接作为awaiter的类型\n";
    auto a = co_await DirectAwaiter{100};
    std::cout << "  DirectAwaiter value: " << a << "\n\n";

    // 2. 使用成员operator co_await
    std::cout << "测试2: 通过成员operator co_await转换的类型\n";
    auto x = co_await MyInt{42};
    std::cout << "  MyInt value: " << x << "\n\n";

    // 3. 使用非成员operator co_await
    std::cout << "测试3: 通过非成员operator co_await转换的类型\n";
    auto y = co_await MyFloat{3.14f};
    std::cout << "  MyFloat value: " << y << "\n\n";

    // 4. 使用promise的await_transform
    std::cout << "测试4: 通过promise的await_transform转换的类型\n";
    auto z = co_await MyDouble{2.718};
    std::cout << "  MyDouble value: " << z << "\n\n";

    std::cout << "Coroutine finished\n";
    co_return;
}

int main()
{
    test_coroutine();

    // 确保协程有机会完成
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::cout << "main done\n";

    // NOTE: await_transform 和  co_await 关键字重写都是为了 生成 awaiter
    // NOTE: co_await 可以向 ”==“ 关键字 被类成员实现和添加功能。一样返回值类型有约束
    return 0;
}
// NOLINTEND