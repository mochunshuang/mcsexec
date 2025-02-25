#include <chrono>
#include <coroutine>
#include <iostream>
#include <thread>
#include <variant>

// 事件类型定义
struct StepCompleted
{
    int step;
    void *ctx;
};

// 协程任务框架
struct Task
{
    struct promise_type
    {
        std::variant<std::monostate, StepCompleted> last_event;
        std::coroutine_handle<> continuation;

        Task get_return_object()
        {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend()
        {
            return {};
        }
        std::suspend_always final_suspend() noexcept
        {
            return {};
        }
        void unhandled_exception() {}
        void return_void()
        {
            if (continuation)
            {
                continuation.resume();
            }
        }
        std::suspend_always yield_value(StepCompleted e)
        {
            last_event = e;
            return {};
        }
    };

    std::coroutine_handle<promise_type> handle;

    void resume()
    {
        if (!handle.done())
            handle.resume();
    }
};

// 异步操作awaiter
struct AsyncAwaiter
{
    Task task;

    bool await_ready()
    {
        return false;
    }
    void await_suspend(std::coroutine_handle<> h)
    {
        task.handle.promise().continuation = h;
        task.resume();
    }
    void await_resume() {}
};

// 实现协程awaitable接口
auto operator co_await(Task task)
{
    return AsyncAwaiter{task};
}

// 实际业务协程
Task async_op_1(void *ctx)
{
    std::cout << "async_op_1\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    co_return;
}

Task async_op_2(void *ctx)
{
    std::cout << "async_op_2\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    co_return;
}

Task async_op_3(void *ctx)
{
    std::cout << "async_op_3\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    co_return;
}

// 主协程逻辑
Task workflow()
{
    void *ctx = nullptr;

    while (true)
    {
        std::cout << "进入协程\n";

        co_await async_op_1(ctx);
        co_yield StepCompleted{1, ctx};

        co_await async_op_2(ctx);
        co_yield StepCompleted{2, ctx};

        co_await async_op_3(ctx);
        co_yield StepCompleted{3, ctx};
        std::cout << "完成周期\n";
    }
}

// 使用示例
int main()
{
    auto main_task = workflow();

    while (true)
    {
        main_task.resume(); // 手动处理

        auto &event = main_task.handle.promise().last_event;
        if (auto *sc = std::get_if<StepCompleted>(&event))
        {
            std::cout << "步骤" << sc->step << "完成\n";
        }
    }
}