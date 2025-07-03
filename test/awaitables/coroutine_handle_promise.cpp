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
            value = v;
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

my_coroutine<int> task(int &count)
{
    std::cout << "Coroutine started\n";
    while (count > 0) // NOTE: coroutine will never done()
    {
        std::cout << "co_yield: " << count << '\n';
        co_yield count--;
    }
    std::cout << "co_return: " << 999 << '\n';
    co_return 999;
}

int main()
{
    int count = 3;
    auto coro = task(count);
    {
        // NOTE: 必须不能 copy ，必须引用
        auto &p = coro.handle.promise();
        static_assert(std::is_same_v<decltype(p), my_coroutine<int>::promise &>);
        auto h = std::coroutine_handle<decltype(p)>::from_promise(p);
        assert(h.address() == coro.handle.address());
    }
    //
    coro.handle.resume();
    assert(coro.handle.promise().value == 3);

    coro.handle.resume();
    assert(coro.handle.promise().value == 2);

    coro.handle.resume();
    assert(coro.handle.promise().value == 1);

    coro.handle.resume();
    assert(coro.handle.promise().value == 999);

    assert(count == 0);

    // NOTE: 但是还是需要手动 destroy()
    assert(coro.handle.done());

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND