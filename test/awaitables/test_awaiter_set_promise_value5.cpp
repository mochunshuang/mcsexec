#include <cassert>
#include <iostream>
#include <coroutine>
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
    void await_resume() {}
};

my_coroutine<int> task(AsyncAwaiter *awaiter)
{
    auto a = AsyncAwaiter{10};
    awaiter = &a;
    co_await a;
    std::cout << "co_return: " << 999 << '\n';
    co_return 999;
}

int main()
{
    AsyncAwaiter *ptr = nullptr;
    auto coro = task(ptr);
    coro.resume();
    assert(coro.handle.promise().value == 10);

    ptr->await_resume(); // NOTE: 不会唤醒协程
    // assert(coro.handle.promise().return_value_ == 999);
    // assert(coro.handle.done());

    assert(not coro.handle.done());

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND