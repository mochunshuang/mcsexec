#include <coroutine>
#include <iostream>
#include <thread>
#include <variant>

struct StepCompleted
{
    int step;
    void *ctx;
};

struct Task
{
    struct promise_type
    {
        std::variant<std::monostate, StepCompleted> last_event;
        std::coroutine_handle<> continuation;
        std::coroutine_handle<> event_handler;

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
                continuation.resume();
        }
        std::suspend_always yield_value(StepCompleted e)
        {
            last_event = e;
            // 仅当事件处理协程处于挂起状态时才恢复它
            if (event_handler && event_handler.address() && !event_handler.done())
            {
                event_handler.resume();
            }
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

struct AsyncAwaiter
{
    Task task;
    AsyncAwaiter(Task &&t) : task(std::move(t)) {}
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

auto operator co_await(Task task)
{
    return AsyncAwaiter{std::move(task)};
}

Task async_op_1(void *ctx)
{
    std::cout << "async_op_1\n";
    struct Awaitable
    {
        bool await_ready()
        {
            return false;
        }
        void await_suspend(std::coroutine_handle<> h)
        {
            std::thread([h] {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                h.resume();
            }).detach();
        }
        void await_resume() {}
    };
    co_await Awaitable{};
}

// async_op_2 和 async_op_3 的实现类似，省略重复代码...
Task async_op_2(void *ctx)
{
    std::cout << "async_op_2\n";
    struct Awaitable
    {
        bool await_ready()
        {
            return false;
        }
        void await_suspend(std::coroutine_handle<> h)
        {
            std::thread([h] {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                h.resume();
            }).detach();
        }
        void await_resume() {}
    };
    co_await Awaitable{};
}
Task async_op_3(void *ctx)
{
    std::cout << "async_op_3\n";
    struct Awaitable
    {
        bool await_ready()
        {
            return false;
        }
        void await_suspend(std::coroutine_handle<> h)
        {
            std::thread([h] {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                h.resume();
            }).detach();
        }
        void await_resume() {}
    };
    co_await Awaitable{};
}
Task workflow()
{
    void *ctx = nullptr;
    while (true)
    {
        std::cout << "进入协程\n";
        co_await async_op_1(ctx);
        co_yield StepCompleted{1, ctx}; // (1)

        co_await async_op_2(ctx);       // (4)
        co_yield StepCompleted{2, ctx}; // (5)

        co_await async_op_3(ctx);
        co_yield StepCompleted{3, ctx};
        std::cout << "完成周期\n";
    }
}

Task event_handler(std::coroutine_handle<Task::promise_type> main_handle)
{
    while (true)
    {
        co_await std::suspend_always{}; // (0) 初始挂起
        auto &event = main_handle.promise().last_event;
        if (auto *sc = std::get_if<StepCompleted>(&event))
        {
            std::cout << "步骤" << sc->step << "完成\n";
            main_handle.promise().last_event = std::monostate{};
        }
        main_handle.resume(); // (2) 恢复主协程
    }
}

int main()
{
    auto main_task = workflow();
    auto event_task = event_handler(main_task.handle);
    main_task.handle.promise().event_handler = event_task.handle;

    event_task.resume(); // 启动事件处理协程 (0)
    main_task.resume();  // 启动主协程

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}