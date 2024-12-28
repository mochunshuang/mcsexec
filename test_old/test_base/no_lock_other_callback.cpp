#include <array>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>
#include <thread>
#include <mutex>
#include <functional>
#include <atomic>
#include <memory>

struct inplace_callback_base
{
    virtual ~inplace_callback_base() = default;
    inplace_callback_base *next{};              // NOLINT
    virtual auto invoke_callback() -> void = 0; // NOLINT

    inplace_callback_base() = default;
    inplace_callback_base(inplace_callback_base &&) = default;
    inplace_callback_base(const inplace_callback_base &) = default;
    inplace_callback_base &operator=(inplace_callback_base &&) = default;
    inplace_callback_base &operator=(const inplace_callback_base &) = default;
};

struct stop_state
{
    std::thread::id id;                             // NOLINT
    std::mutex mtx;                                 // NOLINT
    inplace_callback_base *register_list{};         // NOLINT
    std::atomic<bool> stopped{};                    // NOLINT
    std::atomic<inplace_callback_base *> running{}; // NOLINT
};
stop_state stop_state; // NOLINT

bool request_stop() noexcept
{
    // 1. Effects: Executes a stop request operation ([stoptoken.concepts]).
    using relock = std::unique_ptr<std::unique_lock<std::mutex>,
                                   decltype([](auto p) { p->lock(); })>;

    // when stop_state == not stoped. set stop_state=true and invoke_callback
    if (not stop_state.stopped.exchange(true))
    {
        std::unique_lock guard(stop_state.mtx);
        for (auto *it = stop_state.register_list; it != nullptr;
             it = stop_state.register_list)
        {
            stop_state.running = it;
            stop_state.id = ::std::this_thread::get_id();
            stop_state.register_list = it->next;
            {
                relock r(&guard);
                guard.unlock();

                // No constraint is placed on the order in which the callback
                // invocations are executed
                it->invoke_callback();
            }
            stop_state.running = nullptr;
        }
        return true;
    }

    // 2. Postconditions: stop_requested() is true.
    // assert(stop_requested()); //Note: no necessary. allway is true

    return false;
}

struct callback_test : inplace_callback_base
{

    int id = 0; // NOLINT

    auto invoke_callback() -> void override
    {
        // 随机sleep
        constexpr int k_num = 100;
        std::random_device rd{};                                       // NOLINT
        std::uniform_int_distribution<std::size_t> dist{0, k_num - 1}; // NOLINT

        std::this_thread::sleep_for(std::chrono::milliseconds(dist(rd)));

        std::cout << "tid: " << std::this_thread::get_id() << ", work_id: " << id
                  << " invoke_callback done\n ";
    }
};

int main()
{
    // 向 stop_state 注册 10 个回调，然后并发测试
    std::array<callback_test, 10> callbacks; // NOLINT
    for (int i = 0; i < 10; ++i)
    {
        callbacks[i].id = i;
        callbacks[i].next = stop_state.register_list;
        stop_state.register_list = &callbacks[i];
    }

    // 创建多个线程并发调用 request_stop
    std::vector<std::thread> threads;
    for (int i = 0; i < 5; ++i)
    {
        threads.emplace_back([]() { request_stop(); });
    }

    // 等待所有线程完成
    for (auto &t : threads)
    {
        t.join();
    }

    std::cout << "main done\n";
    return 0;
}
