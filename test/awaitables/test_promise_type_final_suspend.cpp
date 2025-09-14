#include <cassert>
#include <exception>
#include <iostream>
#include <coroutine>

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
        std::suspend_never initial_suspend() noexcept
        {
            return {};
        }
        T final_suspend() noexcept
        {
            std::cout << ">>>>> final_suspend: called\n";
            return {};
        }
        void return_void() {}
        void unhandled_exception() {}
    };

    using promise_type = promise;
    my_coroutine() noexcept = default;
    my_coroutine(std::coroutine_handle<promise_type> h) noexcept : handle{h} {}
    ~my_coroutine() noexcept
    {
        // NOTE: 资源必须唯一所有权。让别人处理，你不知道，变成两次释放，崩溃理所当然
        if (handle)
        {
            handle.destroy();
            if constexpr (std::is_same_v<T, std::suspend_always>)
            {
                std::cout << "std::suspend_always : 手动执行destroy()\n";
            }
            else
            {
                std::cout << "std::suspend_never : 执行destroy()\n";
            }
        }
    }

    my_coroutine(const my_coroutine &) = delete;
    my_coroutine &operator=(const my_coroutine &) = delete;

    my_coroutine(my_coroutine &&other) noexcept : handle(other.handle)
    {
        other.handle = {};
    }

    my_coroutine &operator=(my_coroutine &&other) noexcept
    {
        if (this != &other)
        {
            if (handle)
                handle.destroy();
            handle = other.handle;
            other.handle = {};
        }
        return *this;
    }

    bool done() const
    {
        return handle.done();
    }
    void set_handle_void()
    {
        handle = nullptr;
    }

    std::coroutine_handle<promise_type> handle{};
};

my_coroutine<std::suspend_always> suspend_always()
{
    std::cout << "suspend_always: starting\n";
    co_return;
    std::cout << "suspend_always: after co_return (this will never be printed)\n";
    std::terminate();
}

my_coroutine<std::suspend_never> suspend_never()
{
    std::cout << "suspend_never: starting\n";
    co_return;
    std::cout << "suspend_never: after co_return (this will never be printed)\n";
    std::terminate();
}

int main()
{
    {
        std::cout << "=== Testing suspend_always ===\n";
        auto coro1 = suspend_always();
        std::cout << "After creation: coro1.done() = " << coro1.done() << "\n";

        assert(coro1.done());
        // 必须手动销毁，否则会内存泄漏
        if (!coro1.done())
        {
            std::cout << "suspend_always 手动销毁: \n";
            coro1.handle.destroy();
        }
        // NOTE: 内存泄漏：若使用suspend_always但忘记调用.destroy()，协程帧将永远泄漏
        // coro1.handle.resume(); //NOTE: 一样未定义
        constexpr auto test_handle_destroy = false;
        if constexpr (test_handle_destroy)
        {
            coro1.handle.destroy(); // NOTE: 不会崩溃，不会两次释放
            assert(coro1.handle.address() != nullptr);
            // NOTE: 核心是就算手动 destroy()。handle 也不会自动为 nullptr
            // NOTE: 没有这两行，程序 两次释放同一块内存，行为未定义。崩溃
            coro1.set_handle_void();
            assert(coro1.handle.address() == nullptr);
        }

        // NOTE: suspend_always 要你必须 handle.destroy()
    }

    {
        std::cout << "\n=== Testing suspend_never ===\n";
        auto coro2 = suspend_never();
        std::cout << "After creation: coro2.done() = " << coro2.done() << "\n";
        assert(not coro2.done());
        // 协程已自动销毁，无需手动调用destroy

        assert(coro2.handle.address() != nullptr);

        constexpr auto test_crush = false;
        if constexpr (not test_crush)
        {
            // NOTE: suspend_never 就算自动销毁了，也不会 帮你 置为 nullptr
            coro2.set_handle_void();
            assert(coro2.handle.address() == nullptr);
        }

        // coro2.handle.destroy(); //NOTE: 两次释放，崩溃

        // NOTE: 既然如此，既然不自动帮你置为nullptr ，仅仅帮一半
        // NOTE: 实践： final_suspend 永远不是 suspend_never

        // NOTE: 如果使用 final_suspend 就不要让 协程获得 handle 句柄

        // NOTE: suspend_always 要你必须不能 再 handle.destroy()
    }

    std::cout << "\nmain done\n";
    return 0;
}
// NOLINTEND