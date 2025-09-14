#include <iostream>
#include <coroutine>

// NOLINTBEGIN

// 协程返回类型 - 简化的Generator
template <typename T>
struct Generator
{
    struct promise_type
    {
        T current_value{};

        Generator get_return_object()
        {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::suspend_always initial_suspend()
        {
            return {};
        }
        std::suspend_always final_suspend() noexcept
        {
            // NOTE: co_return 之后调用
            std::cout << ">>> final_suspend()\n\n";
            return {};
        }

        void return_void() {}

        std::suspend_always yield_value(T value)
        {
            current_value = std::move(value);
            return {};
        }

        void unhandled_exception() {}
    };

    std::coroutine_handle<promise_type> handle;

    Generator(std::coroutine_handle<promise_type> h) : handle(h) {}
    ~Generator()
    {
        if (handle)
            handle.destroy();
    }

    // 禁用拷贝，只允许移动
    Generator(const Generator &) = delete;
    Generator(Generator &&other) noexcept : handle(other.handle)
    {
        other.handle = nullptr;
    }

    bool move_next()
    {
        if (!handle || handle.done())
            return false;
        handle.resume();
        return !handle.done();
    }

    T current_value() const
    {
        return handle.promise().current_value;
    }
};

// 测试协程：交替使用co_yield和co_return
Generator<int> test_coroutine()
{
    std::cout << ">>> 协程开始执行 <<<\n";

    // NOTE: yield_value 返回值是 std::suspend_always。意味着离开+挂起(未完成)
    std::cout << "1. [协程体]: 第一次co_yield\n";
    co_yield 1;

    std::cout << "2. [协程体]: 第二次co_yield\n";
    co_yield 2;

    std::cout << "3. [协程体]: 第三次co_yield\n";
    co_yield 3;

    std::cout << "4. [协程体]: 执行co_return,结束协程\n";
    co_return; // NOTE: 离开+协程状态 改成 ”完成“
}

int main()
{
    std::cout << " main(): 创建协程\n";
    auto gen = test_coroutine();

    std::cout << "主函数: 开始迭代协程\n\n";

    int iteration = 1;
    // NOTE: 取值
    while (gen.move_next())
    {
        std::cout << "---- 迭代 #" << iteration++ << " ----\n";
        std::cout << "主函数: 当前值 = " << gen.current_value() << "\n";
        std::cout << "主函数: 协程状态 = " << (gen.handle.done() ? "完成" : "挂起")
                  << "\n\n";
    }

    std::cout << "---- main(): 协程完成后 ----\n";
    std::cout << "主函数: 最终协程状态 = " << (gen.handle.done() ? "完成" : "挂起")
              << "\n";

    return 0;
}
// NOLINTEND