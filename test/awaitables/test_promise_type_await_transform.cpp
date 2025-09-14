#include <iostream>
#include <coroutine>
#include <thread>
#include <chrono>

// NOLINTBEGIN
using namespace std::chrono_literals;

// 自定义等待器
struct TimerAwaiter
{
    std::chrono::milliseconds duration;

    bool await_ready() const noexcept
    {
        return duration.count() <= 0;
    }

    void await_suspend(std::coroutine_handle<> h) const
    {
        std::thread([this, h] {
            std::this_thread::sleep_for(duration);
            h.resume();
        }).detach();
    }

    void await_resume() const noexcept {}
};

// 协程返回类型
struct AsyncTask
{
    struct promise_type
    {
        // 必须实现的promise接口
        AsyncTask get_return_object()
        {
            return {};
        }
        std::suspend_never initial_suspend() noexcept
        {
            return {};
        }
        std::suspend_never final_suspend() noexcept
        {
            return {};
        }
        void return_void() {}
        void unhandled_exception()
        {
            std::terminate();
        }

        // 自定义await_transform
        template <typename T>
        auto await_transform(T &&value)
        {
            std::cout << "Transforming value of type: " << typeid(T).name() << "\n";

            if constexpr (std::is_arithmetic_v<std::decay_t<T>>)
            {
                // 将数值转换为定时器等待
                return TimerAwaiter{std::chrono::milliseconds(value)};
            }
            else if constexpr (std::is_same_v<std::decay_t<T>, std::string>)
            {
                // 为字符串添加特殊处理
                std::cout << ">>> Waiting for string: " << value << "\n";
                return TimerAwaiter{100ms};
            }
            else
            {
                // 直接返回可等待对象
                return std::forward<T>(value);
            }
        }
    };
};

/*
NOTE: 作用如下
类型转换：将任意类型转换为可等待(awaitable)类型

访问控制：限制协程中可等待的类型

逻辑注入：在等待前/后添加自定义逻辑

错误处理：统一处理等待时的异常
*/
AsyncTask demo_coroutine()
{
    std::cout << "      协程开始\n";

    co_await 200; // 等待200ms（通过await_transform转换）

    std::cout << "200ms后...\n";

    co_await TimerAwaiter{300ms}; // 直接使用等待器

    std::cout << "300ms后...\n";

    co_await std::string("Hello"); // 等待字符串（特殊处理）

    std::cout << "      协程结束\n";
}

int main()
{
    std::cout << "=== await_transform 示例 ===\n";

    demo_coroutine();

    // 主线程等待足够时间让协程完成
    std::this_thread::sleep_for(700ms);

    std::cout << "\n=== 示例结束 ===\n";
    return 0;
}
// NOLINTEND