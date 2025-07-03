#include <cassert>
#include <iostream>
#include <coroutine>
#include <string>
#include <utility>

// NOLINTBEGIN

template <typename T>
struct my_coroutine
{
    struct promise
    {
        my_coroutine get_return_object()
        {
            return my_coroutine{std::coroutine_handle<promise>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept
        {
            return {};
        }
        std::suspend_always final_suspend() noexcept
        {
            return {};
        }
        void return_value(T &&v)
        {
            return_value_ = v;
        }
        void unhandled_exception() {}

        // 支持 co_yield
        template <typename U>
        std::suspend_always yield_value(U &&v)
        {
            value = std::forward<U>(v);
            return {};
        }

        T value;
        T return_value_;
    };

    // NOTE: core: promise_type +
    using promise_type = promise;
    my_coroutine() noexcept = default;
    my_coroutine(std::coroutine_handle<promise_type> h) noexcept : handle{h} {}
    ~my_coroutine() noexcept
    {
        if (handle)
        {
            handle.destroy();
            std::cout << "std::suspend_always : 需要手动执行destroy()\n";
        }
    }

    void resume()
    {
        handle.resume();
    }
    std::coroutine_handle<promise_type> handle{};
};

struct AsyncAwaiter
{
    int value;
    AsyncAwaiter(int v) : value(v) {}
    bool await_ready()
    {
        return false;
    }
    std::noop_coroutine_handle await_suspend(
        std::coroutine_handle<my_coroutine<int>::promise> h)
    {
        h.promise().value = value;
        return std::noop_coroutine(); // NOTE: 挂起 h
    }

    // NOTE: 第二次调度的时候 await_resume 会被调用
    auto await_resume()
    {
        std::cout << "await_resume\n";
        return std::to_string(value + 1);
    }
};

my_coroutine<int> task()
{
    std::cout << "task: start\n";
    auto v = co_await AsyncAwaiter{10};

    static_assert(std::is_same_v<decltype(v), std::string>);

    std::cout << "co_await result: " << v << '\n';
    co_return 999;
}

int main()
{

    auto coro = task();
    std::cout << "coro.resume()\n";
    coro.resume();
    assert(coro.handle.promise().value == 10);

    std::cout << "coro.resume()\n";
    coro.resume();
    assert(coro.handle.promise().return_value_ == 999);
    assert(coro.handle.done());

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND