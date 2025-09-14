#include <iostream>
#include <coroutine>
// NOLINTBEGIN

struct Tracker
{
    static void *operator new(size_t size)
    {
        std::cout << "Allocating " << size << " bytes on the heap" << std::endl;
        return ::operator new(size);
    }

    static void operator delete(void *ptr, size_t size)
    {
        std::cout << "Deallocating " << size << " bytes from the heap" << std::endl;
        ::operator delete(ptr);
    }

    void print() noexcept
    {
        std::cout << "Tracker: value: " << value << '\n';
    }
    int value = 0;
};

// 协程返回类型
struct Task
{
    struct promise_type
    {
        Task get_return_object()
        {
            return {std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend()
        {
            return {};
        }
        std::suspend_never final_suspend() noexcept
        {
            return {};
        }
        void return_void() {}
        void unhandled_exception() {}
    };
    Task(std::coroutine_handle<promise_type> h) noexcept : handle{h} {}

    std::coroutine_handle<promise_type> handle{};
};

Task coroutine()
{
    // NOTE: gcc 显示使用的就是栈内存。不触发 new / delete
    Tracker t; // 协程帧中的对象
    t.print();
    co_return;
}

int main()
{
    { // TEST
        auto *p = new Tracker;
        delete p;
    }
    std::cout << "coroutine test: \n";
    auto h = coroutine();
    h.handle.resume();
    // h.handle.destroy(); //NOTE: 会崩溃，因为：std::suspend_never final_suspend() 不允许

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND