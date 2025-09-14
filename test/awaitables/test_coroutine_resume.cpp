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

my_coroutine<std::suspend_always> initial_suspend_always(int &count)
{
    ++count;
    std::cout << "initial_suspend_always called ... \n";
    co_return;
}

my_coroutine<std::suspend_always> initial_suspend_always2(int &count)
{
    std::cout << "initial_suspend_always2 called ... \n";
    while (true) // NOTE: coroutine will never done()
    {
        co_yield count;
        std::cout << "co_yield: " << count << '\n';
    }
}

int main()
{
    int call_count = 0;
    auto coro = initial_suspend_always(call_count);
    assert(call_count == 0);
    coro.resume();
    assert(call_count == 1);
    // coro.resume(); //NOTE: will crush

    {
        // NOTE: can resume many times
        call_count = 0;
        constexpr auto test_time = 8;
        auto coro = initial_suspend_always2(call_count);
        for (int i = 0; i < test_time; ++i)
        {
            ++call_count; // NOTE: 影响 协程体 内部
            coro.resume();
        }
        assert(call_count == test_time);
    }

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND