#include <cassert>
#include <iostream>
#include <coroutine>

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
        // NOTE: suspend_never 避免影响
        std::suspend_never initial_suspend() noexcept
        {
            return {};
        }
        std::suspend_always final_suspend() noexcept
        {
            return {};
        }
        void return_value(T &&v)
        {
            return_value_ += v;
        }
        void unhandled_exception() {}

        T return_value_{0};
    };

    // NOTE: core: promise_type +
    using promise_type = promise;
    my_coroutine() noexcept = default;
    my_coroutine(std::coroutine_handle<promise_type> h) noexcept : handle{h} {}
    ~my_coroutine() noexcept
    {
        assert(handle.done());
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

struct get_handle_awaiter
{
    int value;
    get_handle_awaiter(int v) : value(v) {}
    constexpr bool await_ready() noexcept
    {
        std::cout << "  >>> AsyncAwaiter: await_ready\n";
        return false;
    }
    std::noop_coroutine_handle await_suspend(
        std::coroutine_handle<my_coroutine<int>::promise> h)
    {

        h.promise().return_value_ += value;
        value = h.promise().return_value_;
        std::cout << "  >>> AsyncAwaiter: await_suspend: " << h.promise().return_value_
                  << '\n';
        return std::noop_coroutine();
    }

    auto await_resume()
    {
        std::cout << "  >>> AsyncAwaiter: await_resume\n";
        return value;
    }
};

my_coroutine<int> task()
{
    std::cout << "task: start\n";
    int ret{};
    ret = co_await get_handle_awaiter{1};
    std::cout << "[1] co_await result: " << ret << '\n';
    assert(ret == 1);
    ret = co_await get_handle_awaiter{2};
    std::cout << "[2] co_await result: " << ret << '\n';
    assert(ret == 3);
    ret = co_await get_handle_awaiter{3};
    std::cout << "[3] co_await result: " << ret << '\n';
    assert(ret == 6);

    co_return ret;
}

int main()
{

    auto coro = task();
    // NOTE: await_suspend 已经填值了
    assert(coro.handle.promise().return_value_ == 1);

    std::cout << "coro.resume()\n";
    coro.resume();
    assert(coro.handle.promise().return_value_ == 3);
    assert(not coro.handle.done());

    std::cout << "coro.resume()\n";
    coro.resume();
    assert(coro.handle.promise().return_value_ == 6);
    assert(not coro.handle.done());

    std::cout << "coro.resume()\n";
    coro.resume();
    assert(coro.handle.promise().return_value_ == 12);

    // NOTE: final_suspend 的 await 不参与计数。自定义就自己管理 handler
    assert(coro.handle.done());

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND