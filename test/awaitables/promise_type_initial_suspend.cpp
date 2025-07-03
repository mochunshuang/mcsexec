#include <cassert>
#include <iostream>
#include <coroutine>

// NOLINTBEGIN

template <typename T>
struct my_coroutine
{
    struct promise
    {
        my_coroutine get_return_object() noexcept
        {
            return my_coroutine{std::coroutine_handle<promise>::from_promise(*this)};
        }
        T initial_suspend() noexcept
        {
            return {};
        }
        std::suspend_always final_suspend() noexcept
        {
            return {};
        }
        void return_void() {}
        void unhandled_exception() {}

        // 支持 co_yield
        template <typename U>
        std::suspend_always yield_value(U &&value)
        {
            return {};
        }
    };

    // NOTE: core: promise_type +
    using promise_type = promise;
    my_coroutine() noexcept = default;
    my_coroutine(std::coroutine_handle<promise_type> h) noexcept : handle{h} {}
    ~my_coroutine() noexcept
    {
        if (handle != nullptr)
            handle.destroy();
    }

    // 用于 co_await Task 的 awaitable 类型
    bool await_ready()
    {
        return false;
    }
    void await_suspend(std::coroutine_handle<>) {}
    void await_resume()
    {
        return;
    }
    std::coroutine_handle<promise_type> handle{};
};

my_coroutine<std::suspend_always> suspend_always(auto &called)
{
    called = true;
    co_return;
}
my_coroutine<std::suspend_never> suspend_never(auto &called)
{
    called = true;
    co_return;
}

int main()
{
    bool called = false;
    {
        assert(not called);
        auto b = suspend_always(called);
        assert(not called);
        b.handle.resume();
        assert(called);
    }
    {
        called = false;
        assert(not called);
        auto b = suspend_never(called);
        assert(called);

        // NOTE: 未定义行为
        //  b.handle.resume();
    }
    std::cout << "main done\n";
    return 0;
}
// NOLINTEND