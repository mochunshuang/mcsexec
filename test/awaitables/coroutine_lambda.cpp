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

void bad()
{
    // NOTE: lambda_coroutine don`t capture any
    auto h = [i = 1]() -> my_coroutine<int> // a lambda that's also a coroutine
    {
        std::cout << "bad: i: " << i << '\n';
        co_return 0;
    }(); // immediately invoked
    // NOTE: lambda destroyed is reason
    h.resume(); // uses (anonymous lambda type)::i after free
}

void good()
{
    auto h = [](int i) -> my_coroutine<int> // make i a coroutine parameter
    {
        std::cout << "good: i: " << i << '\n';
        co_return 2;
    }(10);
    // lambda destroyed

    h.resume(); // no problem, i has been copied to the coroutine
                // frame as a by-value parameter

    assert(h.handle.promise().return_value_ == 2);
}

int main()
{
    bad();
    std::cout << "\n===========\n";
    good();
    std::cout << "main done\n";
    return 0;
}
// NOLINTEND