#include <cassert>
#include <cstdlib>
#include <iostream>
#include <coroutine>
#include <exception>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <utility>
// NOLINTBEGIN

namespace test_yield_value
{
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
            std::suspend_always initial_suspend() noexcept
            {
                return {};
            }
            std::suspend_always final_suspend() noexcept
            {
                std::cout << "final_suspend() 线程 ID: " << std::this_thread::get_id()
                          << '\n';
                return {};
            }
            std::suspend_always yield_value(int i) noexcept
            {
                this->value = i;
                return {};
            }
            void return_void()
            {
                std::cout << "return_void() 线程 ID: " << std::this_thread::get_id()
                          << '\n';
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
    // Note: co_yield + co_return is ok
    ReturnType test_yield_value() noexcept // NOLINT
    {
        int i = 0;
        while (true)
        {
            co_yield ++i;
        }
        co_return;
    }
    ReturnType test_unhandled_exception() // NOLINT
    {
        int i = 0;
        co_yield ++i;
        throw std::runtime_error("error msg");
        co_yield ++i;
        co_return;
    }
}; // namespace test_yield_value

namespace test_await_value
{

    auto test_awaitable()
    {
        struct my_awaitable
        {
            using return_type = int;
            bool await_ready() const noexcept
            {
                return false;
            }
            auto await_suspend(std::coroutine_handle<> continuation) const noexcept
            {
                return continuation;
            }
            return_type await_resume() const noexcept
            {
                return -1;
            }
        };
        return my_awaitable{};
    }

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
                std::jthread([h] { h.resume(); });
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

    // Note: co_yield + co_await + co_return is ok
    test_yield_value::ReturnType test_await() // NOLINT
    {
        auto ret = co_await test_awaitable();
        static_assert(std::is_same_v<decltype(ret), int>);
        co_yield ret;

        std::cout << "协程开始，线程 ID: " << std::this_thread::get_id() << '\n';
        co_await switch_to_new_thread();
        // 等待器在此销毁
        std::cout << "协程恢复，线程 ID: " << std::this_thread::get_id() << '\n';

        co_yield 4;
        std::cout << "协程再次恢复，线程 ID: " << std::this_thread::get_id() << '\n';
        co_return;
    }
    test_yield_value::ReturnType test_co_return() // NOLINT
    {
        std::cout << "协程开始，线程 ID: " << std::this_thread::get_id() << '\n';
        co_await switch_to_new_thread();
        std::cout << "协程恢复，线程 ID: " << std::this_thread::get_id() << '\n';
        co_return;

        std::cout << "永远不会执行 " << std::this_thread::get_id() << '\n';
        std::abort();
    }

}; // namespace test_await_value

namespace test_return_value
{
    struct ReturnType
    {
        struct promise_type
        {
            // Note: promise_type => coroutine
            ReturnType get_return_object()
            {
                std::cout << "get_return_object() 线程 ID: " << std::this_thread::get_id()
                          << '\n';
                return ReturnType{
                    std::coroutine_handle<promise_type>::from_promise(*this)};
            }
            std::suspend_always initial_suspend() noexcept
            {
                std::cout << "initial_suspend() 线程 ID: " << std::this_thread::get_id()
                          << '\n';
                return {};
            }
            std::suspend_always final_suspend() noexcept
            {
                std::cout << "final_suspend() 线程 ID: " << std::this_thread::get_id()
                          << '\n';
                return {};
            }
            std::suspend_always yield_value(int i) noexcept
            {
                std::cout << "yield_value() 线程 ID: " << std::this_thread::get_id()
                          << '\n';
                this->value = i;
                return {};
            }
            void return_value(std::string msg)
            {
                std::cout << "return_value() 线程 ID: " << std::this_thread::get_id()
                          << '\n';
                ret_value = std::move(msg);
            }

            void unhandled_exception() noexcept
            {
                exception = std::current_exception();
            }

            int value{};
            std::string ret_value{};
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
    ReturnType test_co_return()
    {
        co_yield 1;
        co_return "msg";
    }
}; // namespace test_return_value

int main()
{
    {
        // Note: coroutine return type
        auto ret = test_yield_value::test_yield_value();
        static_assert(std::is_same_v<decltype(ret), test_yield_value::ReturnType>);
        ret.handle.resume();
        // Note: coroutine => promise_type
        assert(ret.handle.promise().value == 1);

        ret.handle.resume();
        assert(ret.handle.promise().value == 2);
    }
    {
        auto ret = test_yield_value::test_unhandled_exception();
        ret.handle.resume();
        assert(ret.handle.promise().value == 1);
        ret.handle.resume();
        try
        {
            if (ret.handle.promise().exception)
                std::rethrow_exception(ret.handle.promise().exception);
        }
        catch (const std::runtime_error &e)
        {
            assert(std::string_view(e.what()) == std::string_view("error msg"));
            std::cout << "Caught exception: '" << e.what() << "'\n";
        }

        // NOTE: 协程抛异常后，不能再访问哦
        //  ret.handle.resume();
        //  assert(ret.handle.promise().value == 2);
    }
    {
        auto ret = test_await_value::test_await();
        ret.handle.resume();
        assert(ret.handle.promise().value == -1);

        // NOTE: resume() 前后的线程不会变，当作函数处理
        std::cout << "resume前,线程 ID: " << std::this_thread::get_id() << '\n';
        ret.handle.resume();
        std::cout << "resume后,线程 ID: " << std::this_thread::get_id() << '\n';

        ret.handle.resume();
        assert(ret.handle.promise().value == 4);
    }
    {
        // NOTE: 见到 co_return void 不会停止，立即执行
        std::cout << "\n ==test_await_value== \n";
        auto ret = test_await_value::test_co_return();
        std::cout << "resume前,线程 ID: " << std::this_thread::get_id() << '\n';
        ret.handle.resume();
        std::cout << "resume后,线程 ID: " << std::this_thread::get_id() << '\n';
    }

    {
        // NOTE:  co_return value 会暂停,需要第二次 resume()
        std::cout << "\n ==test_return_value== \n";
        auto ret = test_return_value::test_co_return();
        std::cout << "resume前,线程 ID: " << std::this_thread::get_id() << '\n';
        assert(ret.handle.promise().value == 0);
        ret.handle.resume();
        assert(ret.handle.promise().value == 1);
        std::cout << "resume后,线程 ID: " << std::this_thread::get_id() << '\n';

        std::cout << "继续reusme,用其他线程: " << '\n';
        std::jthread([&ret] { ret.handle.resume(); });
    }
    std::cout << "main done\n";
    return 0;
}
// NOLINTEND