#include <algorithm>
#include <cassert>
#include <coroutine>
#include <exception>
#include <iostream>
#include <type_traits>
#include <utility>
#include <variant>

template <typename T>
struct Promise;
template <typename T>
struct Task;

template <template <typename> typename Task, typename T>
struct Promise<Task<T>>
{
    using value_t = T;

    Task<T> get_return_object() noexcept // NOLINT
    {
        return Task<T>{this};
    }
    // Note: std::suspend_never,task not lazy
    constexpr std::suspend_never initial_suspend() noexcept // NOLINT
    {
        return {};
    }
    constexpr auto final_suspend() noexcept // NOLINT
    {
        struct final_awaitable
        {
            [[nodiscard]] constexpr bool await_ready() const noexcept // NOLINT
            {
                return false;
            }
            [[nodiscard]] auto await_suspend( // NOLINT
                std::coroutine_handle<Promise<Task<T>>> this_coro) noexcept
            {
                auto &promise = this_coro.promise();
                if (promise.continuation)
                    promise.continuation.resume();
            }
            void constexpr await_resume() noexcept {} // NOLINT
        };
        return final_awaitable{};
    }
    constexpr void unhandled_exception() noexcept( // NOLINT
        std::is_nothrow_constructible_v<decltype(result), std::exception_ptr>)
    {
        result.template emplace<2>(std::current_exception());
    }
    template <typename V>
    constexpr void return_value(V &&v) noexcept( // NOLINT
        std::is_nothrow_constructible_v<T, decltype(std::forward<V>(v))>)
    {
        result.template emplace<1>(std::forward<V>(v));
    }

    [[nodiscard]] bool isReady() const noexcept
    {
        return result.index() != 0;
    }

    T &&getResult()
    {
        if (result.index() == 2)
            std::rethrow_exception(std::get<2>(result));
        return std::move(std::get<1>(result));
    }

    std::variant<std::monostate, T, std::exception_ptr> result; // NOLINT
    std::coroutine_handle<> continuation;                       // NOLINT
};

template <typename T>
struct Awaitable
{
    using promise_t = Promise<T>;

    [[nodiscard]] bool await_ready() const noexcept // NOLINT
    {
        return promise.isReady();
    };

    [[nodiscard]] auto await_suspend( // NOLINT
        std::coroutine_handle<> continuation) noexcept
    {
        // parent coroutine be continuation
        promise.continuation = continuation;
        // suspended coroutine resume()
        return std::coroutine_handle<promise_t>::from_promise(promise);
    }

    promise_t::value_t &&await_resume() noexcept // NOLINT
    {
        return promise.getResult();
    }

    promise_t &promise; // NOLINT
};

template <typename T>
struct [[nodiscard]] Task
{
    using promise_type = Promise<Task>;
    auto operator co_await() const noexcept
    {
        return Awaitable{*promise};
    }

    Task(const Task &) = delete;
    Task(Task &&) = delete;
    Task &operator=(const Task &) = delete;
    Task &operator=(Task &&) = delete;
    ~Task() noexcept
    {
        std::coroutine_handle<promise_type>::from_promise(*promise).destroy();
    }

    promise_type *promise; // NOLINT
  private:
    explicit Task(promise_type *promise) : promise{promise} {}
    template <typename>
    friend struct Promise;
};

Task<int> foo() noexcept // NOLINT
{
    std::cout << "foo() called \n";
    co_return 2;
}

Task<double> bar() noexcept // NOLINT
{
    auto c = foo();
    std::cout << "co_await foo\n";
    auto ret = co_await c;
    std::cout << "co_return foo\n";
    co_return ret + 1.0;
}

int main()
{
    auto task [[maybe_unused]] = bar();
    assert(task.promise->getResult() == 3);
    return 0;
}