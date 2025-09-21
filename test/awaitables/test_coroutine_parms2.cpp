
// NOLINTBEGIN
#include <coroutine>
#include <iostream>
#include <string>
#include <tuple>
#include <utility>

// Task type returned by the coroutine
template <typename T, typename... Args>
struct Task
{
    // promise_type is the bridge between the coroutine and Task
    struct promise_type
    {
        // Use tuple to store any number and type of parameters
        std::tuple<Args...> args_;

        // Receive the parameter pack of the coroutine function when constructing the
        // promise
        template <typename... CtorArgs>
        promise_type(CtorArgs &&...args) : args_(std::forward<CtorArgs>(args)...)
        {
            std::cout << "promise received parameters: ";
            print_args(std::make_index_sequence<sizeof...(Args)>{});
            std::cout << "\n";
        }

        // Helper function: print parameters
        template <std::size_t... I>
        void print_args(std::index_sequence<I...>)
        {
            ((std::cout << (I == 0 ? "" : ", ") << "arg" << I << "="
                        << std::get<I>(args_)),
             ...);
        }

        // Create Task object (holds the coroutine handle)
        Task get_return_object()
        {
            return Task{std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_never initial_suspend() noexcept
        {
            return {};
        }
        std::suspend_always final_suspend() noexcept
        {
            return {};
        }
        void return_value(T value)
        {
            result_ = value;
        }
        void unhandled_exception()
        {
            std::terminate();
        }

        // Store the return result of the coroutine
        T result_;
    };

    // Task holds the coroutine handle (indirectly associated with promise and parameters)
    std::coroutine_handle<promise_type> handle;

    // Get the stored parameters
    auto &get_args() const
    {
        return handle.promise().args_;
    }
};

// Coroutine function example 1: receives two parameters
Task<int, int, std::string> coro_func(int x, const std::string &y)
{
    std::cout << "Using parameters in the coroutine body: x=" << x << ", y=" << y << "\n";
    co_return x + static_cast<int>(y.size());
}

// Coroutine function example 2: receives three parameters
Task<double, int, double, std::string> coro_func2(int x, double y, const std::string &z)
{
    std::cout << "Using parameters in the coroutine body: x=" << x << ", y=" << y
              << ", z=" << z << "\n";
    co_return x + y + z.size();
}

int main()
{
    // Test the first coroutine function
    auto task1 = coro_func(10, "hello");
    auto args1 = task1.get_args();
    std::cout << "Get parameters from promise: ";
    std::cout << "arg0=" << std::get<0>(args1) << ", arg1=" << std::get<1>(args1) << "\n";
    std::cout << "Coroutine return result: " << task1.handle.promise().result_ << "\n\n";

    task1.handle.destroy();

    // Test the second coroutine function
    auto task2 = coro_func2(5, 3.14, "world");
    auto args2 = task2.get_args();
    std::cout << "Get parameters from promise: ";
    std::cout << "arg0=" << std::get<0>(args2) << ", arg1=" << std::get<1>(args2)
              << ", arg2=" << std::get<2>(args2) << "\n";
    std::cout << "Coroutine return result: " << task2.handle.promise().result_ << "\n";

    task2.handle.destroy();

    return 0;
}
// NOLINTEND