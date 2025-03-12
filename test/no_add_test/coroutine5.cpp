#include <cassert>
#include <chrono>
#include <iostream>
#include <coroutine>
#include <exception>
#include <thread>

// NOLINTBEGIN

namespace test_awaitable
{
    struct my_suspend_always
    {
        constexpr bool await_ready() const noexcept
        {
            std::cout << "my_suspend_always: await_ready() 线程 ID: "
                      << std::this_thread::get_id() << '\n';
            return false;
        }
        constexpr void await_suspend(std::coroutine_handle<>) const noexcept
        {
            std::cout << "my_suspend_always: await_suspend() 线程 ID: "
                      << std::this_thread::get_id() << '\n';
        }
        constexpr void await_resume() const noexcept
        {
            std::cout << "my_suspend_always: await_resume() 线程 ID: "
                      << std::this_thread::get_id() << '\n';
        }
        constexpr my_suspend_always() noexcept
        {
            std::cout << "my_suspend_always() 线程 ID: " << std::this_thread::get_id()
                      << '\n';
        }
        constexpr ~my_suspend_always() noexcept
        {
            std::cout << "~my_suspend_always() 线程 ID: " << std::this_thread::get_id()
                      << '\n';
        }
    };

    struct ReturnType
    {
        struct promise_type
        {
            // Note: promise_type => coroutine
            ReturnType get_return_object()
            {
                return ReturnType{
                    std::coroutine_handle<promise_type>::from_promise(*this)};
            }
            my_suspend_always initial_suspend() noexcept
            {
                return {};
            }
            my_suspend_always final_suspend() noexcept
            {
                std::cout << "final_suspend() 线程 ID: " << std::this_thread::get_id()
                          << '\n';
                return {};
            }
            my_suspend_always yield_value(int i) noexcept
            {
                this->value = i;
                return {};
            }
            void return_void()
            {
                std::cout << "return_void: now_value: " << value
                          << ",线程 ID: " << std::this_thread::get_id() << '\n';
            }
            void unhandled_exception() noexcept
            {
                exception = std::current_exception();
            }

            int value{};
            std::exception_ptr exception{};
        };
        using handle_type = std::coroutine_handle<promise_type>;
        ReturnType(handle_type h) : handle(h)
        {
            std::cout << "ReturnType(handle_type h) 线程 ID: "
                      << std::this_thread::get_id() << '\n';
        }
        ~ReturnType() noexcept
        {
            std::cout << "~ReturnType() 线程 ID: " << std::this_thread::get_id() << '\n';
            if (handle)
            {
                handle.destroy();
                handle = nullptr;
            }
        }

        // Note: coroutine record coroutine_handle
        std::coroutine_handle<promise_type> handle{};
    };

    struct ReturnTypeValue
    {
        struct promise_type
        {
            // Note: promise_type => coroutine
            ReturnTypeValue get_return_object()
            {
                return ReturnTypeValue{
                    std::coroutine_handle<promise_type>::from_promise(*this)};
            }
            my_suspend_always initial_suspend() noexcept
            {
                return {};
            }
            my_suspend_always final_suspend() noexcept
            {
                std::cout << "final_suspend() 线程 ID: " << std::this_thread::get_id()
                          << '\n';
                return {};
            }
            my_suspend_always yield_value(int i) noexcept
            {
                this->value = i;
                return {};
            }
            void return_value(int value)
            {
                std::cout << "return_value: now_value: " << this->value
                          << ", new_value: " << value
                          << ",线程 ID: " << std::this_thread::get_id() << '\n';
                this->value = value;
            }
            void unhandled_exception() noexcept
            {
                exception = std::current_exception();
            }

            int value{};
            std::exception_ptr exception{};
        };
        using handle_type = std::coroutine_handle<promise_type>;
        ReturnTypeValue(handle_type h) : handle(h)
        {
            std::cout << "ReturnTypeValue(handle_type h) 线程 ID: "
                      << std::this_thread::get_id() << '\n';
        }
        ~ReturnTypeValue() noexcept
        {
            std::cout << "~ReturnTypeValue() 线程 ID: " << std::this_thread::get_id()
                      << '\n';
            if (handle)
            {
                handle.destroy();
                handle = nullptr;
            }
        }

        // Note: coroutine record coroutine_handle
        std::coroutine_handle<promise_type> handle{};
    };

    auto switch_to_new_thread()
    {
        struct awaitable
        {
            bool await_ready() noexcept
            {
                std::cout << "await_ready 线程 ID: " << std::this_thread::get_id()
                          << '\n';
                return false;
            }
            void await_suspend(std::coroutine_handle<> h) noexcept
            {
                std::cout << "await_suspend 线程 ID: " << std::this_thread::get_id()
                          << '\n';
                // Note: 调度在 await_suspend 确定和定义
                std::jthread([h] {
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                    h.resume();
                }).detach();
            }
            void await_resume() noexcept
            {
                std::cout << "await_resume 线程 ID: " << std::this_thread::get_id()
                          << '\n';
            }
            awaitable() noexcept
            {
                std::cout << "awaitable() 线程 ID: " << std::this_thread::get_id()
                          << '\n';
            }
            ~awaitable() noexcept
            {
                std::cout << "~awaitable() 线程 ID: " << std::this_thread::get_id()
                          << '\n';
            }
        };
        return awaitable{};
    }

    struct my_awaitable
    {
        bool await_ready() noexcept
        {
            std::cout << "my_awaitable: await_ready 线程 ID: "
                      << std::this_thread::get_id() << '\n';
            return false;
        }
        auto await_suspend(std::coroutine_handle<> h) noexcept
        {
            std::cout << "my_awaitable: await_suspend 线程 ID: "
                      << std::this_thread::get_id() << '\n';
            return h;
        }
        void await_resume() noexcept
        {
            std::cout << "my_awaitable: await_resume 线程 ID: "
                      << std::this_thread::get_id() << '\n';
        }
        my_awaitable() noexcept
        {
            std::cout << "my_awaitable() 线程 ID: " << std::this_thread::get_id() << '\n';
        }
        ~my_awaitable() noexcept
        {
            std::cout << "~my_awaitable() 线程 ID: " << std::this_thread::get_id()
                      << '\n';
        }
    };

    struct my_awaitable2
    {
        std::chrono::high_resolution_clock::time_point start_time;
        bool await_ready() noexcept
        {
            std::cout << "my_awaitable: await_ready 线程 ID: "
                      << std::this_thread::get_id() << '\n';
            return false;
        }
        void await_suspend(std::coroutine_handle<> h) noexcept
        {
            std::cout << "my_awaitable: await_suspend 线程 ID: "
                      << std::this_thread::get_id() << '\n';

            start_time = std::chrono::high_resolution_clock::now();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            h.resume();
        }
        void await_resume() noexcept
        {
            std::cout << "my_awaitable: await_resume 线程 ID: "
                      << std::this_thread::get_id() << '\n';
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::high_resolution_clock::now() - start_time);
            std::cout << "睡眠模拟暂停耗时: " << duration.count() << " 微秒\n";

            auto milliseconds_value = std::chrono::milliseconds(1);
            auto microseconds_value =
                std::chrono::duration_cast<std::chrono::microseconds>(milliseconds_value);
            std::cout << "1 毫秒 = " << microseconds_value.count() << " 微秒\n";
        }
        my_awaitable2() noexcept
        {
            std::cout << "my_awaitable2() 线程 ID: " << std::this_thread::get_id()
                      << '\n';
        }
        ~my_awaitable2() noexcept
        {
            std::cout << "~my_awaitable2() 线程 ID: " << std::this_thread::get_id()
                      << '\n';
        }
    };

    // Note: co_yield + co_await + co_return is ok
    ReturnType test_await() // NOLINT
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        std::cout << "切换开始，线程 ID: " << std::this_thread::get_id() << '\n';
        co_await switch_to_new_thread();
        // NOTE: 等待器在此销毁: ~awaitable() 被调用
        std::cout << "切换恢复，线程 ID: " << std::this_thread::get_id() << '\n';
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - start_time);
        std::cout << "切换耗时: " << duration.count() << " 微秒\n";

        std::cout << "co_yield前, 线程 ID: " << std::this_thread::get_id() << '\n';
        co_yield 4;
        std::cout << "co_yield后, 线程 ID: " << std::this_thread::get_id() << '\n';

        std::cout << "co_await前, 线程 ID: " << std::this_thread::get_id() << '\n';
        co_await my_awaitable{};
        std::cout << "co_await后, 线程 ID: " << std::this_thread::get_id() << '\n';

        std::cout << "co_await my_awaitable2前, 线程 ID: " << std::this_thread::get_id()
                  << '\n';
        co_await my_awaitable2{};
        std::cout << "co_await my_awaitable2后, 线程 ID: " << std::this_thread::get_id()
                  << '\n';

        std::cout << "co_return 前, 线程 ID: " << std::this_thread::get_id() << '\n';
        co_return;
        std::cout << "co_return 后, 线程 ID: " << std::this_thread::get_id() << '\n';
    }

    ReturnTypeValue test_await2() // NOLINT
    {

        auto start_time = std::chrono::high_resolution_clock::now();
        std::cout << "切换开始，线程 ID: " << std::this_thread::get_id() << '\n';
        co_await switch_to_new_thread();
        // NOTE: 等待器在此销毁: ~awaitable() 被调用
        std::cout << "切换恢复，线程 ID: " << std::this_thread::get_id() << '\n';
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - start_time);
        std::cout << "切换耗时: " << duration.count() << " 微秒\n";

        std::cout << "co_yield前, 线程 ID: " << std::this_thread::get_id() << '\n';
        co_yield 4;
        std::cout << "co_yield后, 线程 ID: " << std::this_thread::get_id() << '\n';

        std::cout << "co_await前, 线程 ID: " << std::this_thread::get_id() << '\n';
        co_await my_awaitable{};
        // NOTE: ~my_awaitable() 销毁
        std::cout << "co_await后, 线程 ID: " << std::this_thread::get_id() << '\n';

        std::cout << "co_await my_awaitable2前, 线程 ID: " << std::this_thread::get_id()
                  << '\n';
        co_await my_awaitable2{};
        std::cout << "co_await my_awaitable2后, 线程 ID: " << std::this_thread::get_id()
                  << '\n';

        std::cout << "co_return value 前, 线程 ID: " << std::this_thread::get_id()
                  << '\n';
        co_return 2;
        std::cout << "co_return value 后, 线程 ID: " << std::this_thread::get_id()
                  << '\n';
    }

}; // namespace test_awaitable

int main()
{

    std::cout << "\n=============test_await=============\n";
    {
        auto ret = test_awaitable::test_await();

        // NOTE: resume() 前后的线程不会变，当作函数处理
        std::cout << "\n第一次 resume前,线程 ID: " << std::this_thread::get_id() << '\n';
        ret.handle.resume();
        std::cout << "第一次 resume后,线程 ID: " << std::this_thread::get_id() << '\n';

        // NOTE: 异步的证明： 遇到 co_yield 和 co_return 不会停下
        assert(ret.handle.promise().value == 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        assert(ret.handle.promise().value == 4);

        std::cout << "\n第二次 resume前\n";
        ret.handle.resume();
        std::cout << "第二次 resume后\n";

        assert(ret.handle.promise().value == 4);
    }
    std::cout << "\n=============test_await2=============\n";
    {
        auto ret = test_awaitable::test_await2();

        // NOTE: resume() 前后的线程不会变，当作函数处理
        std::cout << "\n第一次 resume前,线程 ID: " << std::this_thread::get_id() << '\n';
        ret.handle.resume();
        std::cout << "第一次 resume后,线程 ID: " << std::this_thread::get_id() << '\n';

        // NOTE: 异步的证明： 遇到 co_yield 和 co_return 不会停下
        assert(ret.handle.promise().value == 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        assert(ret.handle.promise().value == 4);

        std::cout << "\n第二次 resume前\n";
        ret.handle.resume();
        std::cout << "第二次 resume后\n";

        assert(ret.handle.promise().value == 2);

        // NOTE: 不会有第三次 resume(), co_await 不增加 resume 次数
        // std::cout << "第三次 resume前\n";
        // ret.handle.resume();
        // std::cout << "第三次 resume后\n";
    }

    // NOTE: 协程函数体构造的 awaitable,在 awaitable.resume() 调用后立即销毁
    // NOTE: promise_type 相关的 awaitable 和 协程一起被销毁

    // NOTE: 异步的证明： 遇到 co_await 会暂停协程。恢复很快则看起来没有暂停而已
    // NOTE: 异步的证明： 不遇到 co_yield 和 co_return 不会离开协程
    // NOTE: 异步的证明： co_await 不增加 resume() 的次数
    // NOTE: co_yield 和 co_return 才离开协程，增加 resume() 的次数

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND