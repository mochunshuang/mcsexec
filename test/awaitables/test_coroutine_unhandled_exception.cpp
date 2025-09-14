#include <cassert>
#include <exception>
#include <iostream>
#include <coroutine>
#include <string_view>
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
            return_value_ = std::move(v);
        }

        // NOTE: 关键函数：处理协程体内部未捕获的异常
        void unhandled_exception()
        {
            exception_ = std::current_exception();
        }

        // 支持 co_yield
        template <typename U>
        std::suspend_always yield_value(U &&v)
        {
            value = std::forward<U>(v);
            return {};
        }

        T value{0};
        T return_value_{0};
        std::exception_ptr exception_{}; // 存储异常指针
    };

    using promise_type = promise;
    my_coroutine() noexcept = default;
    my_coroutine(std::coroutine_handle<promise_type> h) noexcept : handle{h} {}
    ~my_coroutine() noexcept
    {
        if (handle)
        {
            handle.destroy();
        }
    }

    void resume()
    {
        if (handle && !handle.done())
        {
            handle.resume();
        }
    }

    // 添加：检查并重新抛出异常
    void rethrow_if_exception()
    {
        if (has_exception())
        {
            std::rethrow_exception(handle.promise().exception_);
        }
    }

    // 添加：检查是否有异常
    bool has_exception() const
    {
        return handle && handle.promise().exception_ != nullptr;
    }

    std::coroutine_handle<promise_type> handle{};
};

inline constexpr auto exception_string_value = "Test exception from coroutine";

// 测试协程：会抛出异常
my_coroutine<int> task_that_throws(int count)
{
    std::cout << "Coroutine started\n";

    for (int i = 0; i < count; ++i)
    {
        std::cout << "co_yield: " << i << '\n';
        co_yield i;

        if (i == 2)
        {
            std::cout << ">>> 1.Throwing exception from coroutine\n";
            throw std::runtime_error(exception_string_value);
        }
    }

    co_return 999;
}

int main()
{

    try
    {
        auto coro = task_that_throws(5);
        // 执行协程直到完成或抛出异常
        while (!coro.handle.done())
        {
            coro.resume();

            // 每次恢复后检查是否有异常
            if (coro.has_exception())
            {
                std::cout << ">>> 2.Exception detected in coroutine\n";
                coro.rethrow_if_exception();
            }
        }

        std::cout << "+++ Coroutine completed normally\n";
    }
    catch (const std::exception &e)
    {
        std::cout << ">>> 3.Caught exception in main(): " << e.what() << '\n';
        assert(e.what() == std::string_view{exception_string_value});
    }

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND