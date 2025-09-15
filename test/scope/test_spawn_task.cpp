#include <chrono>
#include <iostream>

#include "../test_base_head.hpp"

int main()
{

    ex::static_thread_pool<1> pool;
    std::thread::id main_id = std::this_thread::get_id();
    std::thread::id thread_id = pool[0].thread_id();
    EXPECT(main_id != thread_id);
    ex::counting_scope scope;

    TEST("with sndr") = [&] {
        bool done{};
        auto task = ex::just() | ex::then([&] noexcept { done = true; });
        ex::spawn(std::move(task), scope.get_token());
        while (not done)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    };

    TEST("operation ex::task<>") = [&] {
        bool called{false};
        bool called_fun{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};
        auto task = [](auto &called) -> ex::task<> {
            std::cout << "with ex::task<> internal";
            called = true;
            co_return;
        }(called);
        auto op =
            connect(std::move(task),
                    test::any_receiver{.called = &called, .data = &any, .chanel = &c});

        EXPECT(not called);
        EXPECT(not called_fun);
        EXPECT(c == test::channel::NO_CALL);

        start(op);

        EXPECT(called);
        EXPECT(not called_fun);
        EXPECT(c == test::channel::VALUE_CHANNEL);
        std::cout << "operation ex::task<> done\n";
    };
    TEST("with ex::task<>") = [&] {
        bool done{};
        auto task = [](auto &done) -> ex::task<> {
            std::cout << "with ex::task<> internal";
            done = true;
            co_return;
        }(done);

        static_assert(ex::snd::sender<decltype(task)>);
        ex::spawn(std::move(task), scope.get_token());

        while (not done)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::cout << "with ex::task<> done";
    };

    TEST("with ex::task<int>") = [&] {
        bool done{};
        auto ret{0};
        std::thread::id task_id;
        std::thread::id then_id;

        auto task = [&](auto &task_id) noexcept -> ex::task<int> {
            task_id = std::this_thread::get_id();
            co_return 1;
        }(task_id) | ex::then([&](int v) {
                                                       ret = v;
                                                       done = true;
                                                       then_id =
                                                           std::this_thread::get_id();
                                                   });
        static_assert(ex::snd::sender<decltype(task)>);
        ex::spawn(std::move(task), scope.get_token());

        while (not done)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        EXPECT(ret == 1);

        EXPECT(task_id == main_id);
        EXPECT(task_id == then_id);
    };

    TEST("with ex::starts_on()") = [&] {
        bool done{};
        auto ret{0};
        std::thread::id task_id;
        std::thread::id then_id;

        auto task = [](auto &task_id) noexcept -> ex::task<int> {
            task_id = std::this_thread::get_id();
            co_return 1;
        }(task_id) | ex::then([&](int v) {
                                                      done = true;
                                                      ret = v;
                                                      then_id =
                                                          std::this_thread::get_id();
                                                  });

        ex::spawn(ex::starts_on(pool.get_scheduler(), std::move(task)),
                  scope.get_token());

        while (not done)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // NOTE: co_await sndr。 一定是 suspend 的时候切换出去了吗？
        EXPECT(ret == 1);
        EXPECT(task_id == main_id); // TODO(mcs): BUG
        EXPECT(task_id == then_id);
    };

    // NOTE: 协程生成 lambda 永远永远 不要 [&] ，引用一定要通过(auto& ref...)传递给协程体
    mcs::this_thread::sync_wait(scope.join());
    std::cout << "main done\n";
    return 0;
}