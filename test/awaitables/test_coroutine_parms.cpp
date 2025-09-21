#include <coroutine>
#include <iostream>
#include <string>

// NOLINTBEGIN

// 协程返回的Task类型
template <typename T>
struct Task
{
    // promise_type是协程与Task的桥梁
    struct promise_type
    {
        // 存储协程函数的参数（示例：保存x和y）
        int x_;
        std::string y_;

        // 构造promise时接收协程函数的参数
        promise_type(int x, const std::string &y) : x_(x), y_(y)
        {
            std::cout << "promise接收参数: x=" << x << ", y=" << y << "\n";
        }

        // 创建Task对象（持有协程句柄）
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

        // 存储协程的返回结果
        T result_;
    };

    // Task持有协程句柄（间接关联到promise和参数）
    std::coroutine_handle<promise_type> handle;
};

// 协程函数：接收参数x和y，返回Task<int>
Task<int> coro_func(int x, const std::string &y)
{
    // 协程体内可直接使用参数x和y（来自协程帧）
    std::cout << "协程体内使用参数: x=" << x << ", y=" << y << "\n";
    co_return x + y.size(); // 示例：返回x与y长度的和
}

int main()
{
    // 调用协程函数，传入参数
    Task<int> task = coro_func(10, "hello");

    // 通过Task的句柄访问promise中的参数和结果
    auto &promise = task.handle.promise();
    std::cout << "从promise获取参数: x=" << promise.x_ << ", y=" << promise.y_ << "\n";
    std::cout << "协程返回结果: " << promise.result_ << "\n";

    task.handle.destroy();
    return 0;
}
// NOLINTEND