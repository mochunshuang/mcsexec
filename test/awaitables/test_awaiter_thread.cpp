#include <cassert>
#include <chrono>
#include <iostream>
#include <coroutine>
#include <string>
#include <thread>
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

        T value{0};
        T return_value_{0};
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

    // NOTE: noop_coroutine_handle 可以隐式转化成 std::coroutine_handle<>
    std::coroutine_handle<> await_suspend(
        std::coroutine_handle<my_coroutine<int>::promise> h)
    {

        {
            // NOTE: std::coroutine_handle<> 类似基类吧。抽象
            [[maybe_unused]] std::coroutine_handle<> p = h;
        }
        std::cout << ">>>>> await_suspend......\n";
        m_id = std::this_thread::get_id();
        std::thread t{[this, h]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            h.promise().value = value;
            std::cout << ">>>>> resume handle......\n";
            h.resume();
        }};
        t.detach();
        return std::noop_coroutine(); // NOTE: 挂起 h
    }

    // NOTE: 第二次调度的时候 await_resume 会被调用
    auto await_resume()
    {
        std::cout << ">>>>> await_resume()......\n";
        assert(m_id != std::this_thread::get_id());
        return std::to_string(value + 1);
    }
    std::thread::id m_id;
};

my_coroutine<int> task()
{
    std::cout << "task: start\n";
    std::cout << ">>>>> [co_await in ] thread_id: " << std::this_thread::get_id() << "\n";
    auto v = co_await AsyncAwaiter{10};
    std::cout << ">>>>> [co_await out] thread_id: " << std::this_thread::get_id() << "\n";
    std::cout << "co_await result: " << v << '\n';
    co_return 999;
}

int main()
{
    std::cout << ">>>>> [main() ] thread_id: " << std::this_thread::get_id() << "\n";

    auto coro = task();
    std::cout << "coro.resume()\n";
    coro.resume(); // NOTE: only onece
    assert(coro.handle.promise().value != 10);

    while (not coro.handle.done())
    {
        std::cout << "          main thead wait coro.handle.done()\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    assert(coro.handle.promise().value == 10);
    assert(coro.handle.promise().return_value_ == 999);
    assert(coro.handle.done());

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND