#include <iostream>
#include <coroutine>
#include <thread>
#include <chrono>

// NOLINTBEGIN

template <typename T = void>
struct SimpleAwaiter
{
    T value{};

    bool await_ready() const noexcept
    {
        std::cout << "await_ready()\n";
        return false; // 总是挂起
    }

    void await_suspend(std::coroutine_handle<> h) const noexcept
    {
        std::cout << "await_suspend() - resuming immediately\n";
        h.resume(); // 立即恢复
    }

    T await_resume() const noexcept
    {
        std::cout << "await_resume()\n";
        return value;
    }
};

// 2. 普通类型 - 通过成员operator co_await转换
struct MyInt
{
    int value;

    MyInt(int v) : value(v) {}

    SimpleAwaiter<int> operator co_await() const
    {
        std::cout << ">>> [co_await] Using member operator co_await()\n";
        return {value};
    }
};

// 3. 普通类型 - 通过非成员operator co_await转换
struct MyFloat
{
    float value;

    MyFloat(float v) : value(v) {}
};

SimpleAwaiter<float> operator co_await(MyFloat f)
{
    std::cout
        << ">>> [co_await non-member] Using non-member operator co_await() with value: "
        << f.value << "\n";
    return {f.value};
}

// 4. 普通类型 - 通过promise的await_transform转换
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

        // 关键改进：使用模板处理所有类型
        template <typename U>
        auto await_transform(U &&obj)
        {
            std::cout << ">>> [promise_type::await_transform()]  with value: "
                      << obj.value << "\n";
            return SimpleAwaiter{obj.value};
        }
    };
};

// 测试协程
Task<void> test_coroutine()
{
    std::cout << "Coroutine started\n";

    // 1. 直接使用awaiter
    co_await SimpleAwaiter<int>{42};

    std::cout << "\n";

    // 2. 使用成员operator co_await
    auto x = co_await MyInt{42};
    std::cout << "MyInt value: " << x << "\n";

    std::cout << "\n";
    // 3. 使用非成员operator co_await
    auto y = co_await MyFloat{3.14f};
    std::cout << "MyFloat value: " << y << "\n";

    std::cout << "\n";
    // 4. 使用promise的await_transform
    auto z = co_await MyDouble{2.718};
    std::cout << "MyDouble value: " << z << "\n";

    std::cout << "\n";
    std::cout << "Coroutine finished\n";
}

int main()
{
    test_coroutine();
    std::cout << "Main continues\n";

    // 确保协程有机会完成
    std::this_thread::sleep_for(std::chrono::milliseconds(3));
    std::cout << "main done\n";
    return 0;
}
// NOLINTEND