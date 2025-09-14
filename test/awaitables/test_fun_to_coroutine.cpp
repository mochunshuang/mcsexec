#include <cassert>
#include <exception>
#include <iostream>
#include <coroutine>
#include <thread>

// NOLINTBEGIN

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
        void return_void() {}
        void unhandled_exception() {}

        // 支持 co_yield
        template <typename U>
        std::suspend_always yield_value(U && /**/)
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
            handle.destroy();
    }

    // 用于 co_await Task 的 awaitable 类型
    bool await_ready()
    {
        return false;
    }
    void await_suspend(std::coroutine_handle<>) {}
    void await_resume()
    {
        return;
    }
    std::coroutine_handle<promise_type> handle{};
};

my_coroutine not_make(auto &called)
{
    called = true;
    return {};
}

my_coroutine do_make(auto &called)
{
    std::cout << "do_make: ...\n";
    std::cout << "thead_id: " << std::this_thread::get_id() << '\n';
    called = true;
    co_return;
}

my_coroutine do_make1(auto &called)
{
    std::cout << "do_make1: ...\n";
    called = true;
    co_yield 0;
}
my_coroutine do_make2(auto &called)
{
    std::cout << "do_make2: ...\n";
    called = true;
    co_await do_make(called);
}

int main()
{

    std::cout << "main_thead_id: " << std::this_thread::get_id() << '\n';
    bool called = false;
    auto a = not_make(called);
    assert(called);

    {
        called = false;
        assert(not called);
        auto b = do_make(called);
        assert(not called);
        b.handle.resume();
        assert(called);

        try
        {
            // NOTE: 再次 resume(); 未定义行为
            /*
协程的生命周期：一旦协程执行完毕（也就是碰到了co_return语句），
    它就会进入完成状态。这时再调用 resume() 是未定义行为，而非抛出异常。

异常处理机制：协程框架里的 unhandled_exception()
方法，主要是用来处理协程内部抛出但未被捕获的异常， 并非处理对已完成协程调用 resume()
这种错误操作
            */
            // b.handle.resume();
            assert(called);
        }
        catch (const std::exception &ptr)
        {
            std::cout << "resume() exception: " << ptr.what() << '\n';
        }
        catch (...)
        {
            std::cout << "resume() exception unknown: " << '\n';
        }
    }
    {
        // NOTE: 拿到协程返回对象，可以人为规定哪个 线程来执行函数体
        called = false;
        auto b = do_make(called);
        assert(not called);
        std::jthread thread{[&]() {
            b.handle.resume();
        }};
        thread.join();
        assert(called);
    }
    {
        called = false;
        auto b = do_make1(called);
        assert(not called);
        b.handle.resume();
        assert(called);
    }
    {
        called = false;
        auto b = do_make2(called);
        assert(not called);
        b.handle(); // NOTE: 另一种写法
        assert(called);
    }

    // NOTE: co_return，co_yield，co_await make fun to coroutine
    std::cout << "main done\n";
    return 0;
}
// NOLINTEND