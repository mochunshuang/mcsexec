#include "../test_base_head.hpp"

// NOLINTBEGIN

#include <chrono>
#include <iostream>
#include <thread>

void base_test() noexcept
{
    bool then_called = false;
    auto scope_sender =
        ex::just(1, std::string("abc")) | ex::then([](auto, auto) noexcept {

        }) |
        ex::let_async_scope([](auto scope_token) noexcept {
            // NOLINTEND
            static_assert(ex::scope::async_scope_token<decltype(scope_token)>);
            return ex::just(2, 1.0);
        }) |
        ex::then([&](auto a, auto b) {
            EXPECT(a == 2);
            EXPECT(b == 1.0);
            then_called = true;
        });
    mcs::this_thread::sync_wait(scope_sender);
    EXPECT(then_called);
}

void base_test2() noexcept
{
    bool then_called = false;
    auto scope_sender =
        ex::just(1, std::string("abc")) | ex::then([](auto, auto) noexcept {

        }) |
        ex::let_async_scope([](auto scope_token) noexcept {
            // NOLINTEND
            static_assert(ex::scope::async_scope_token<decltype(scope_token)>);
            return ex::just(2);
        }) |
        ex::then([&](auto a) {
            EXPECT(a == 2);
            then_called = true;
        });
    mcs::this_thread::sync_wait(scope_sender);
    EXPECT(then_called);
}

void base_test3()
{
    auto snd = ex::just() | ex::then([]() {}) |
               ex::let_async_scope([](auto scope_token) noexcept {
                   // NOLINTEND
                   static_assert(ex::scope::async_scope_token<decltype(scope_token)>);
                   return ex::just(2, 1.0);
               }) |
               ex::then([](auto a, auto b) {
                   EXPECT(a == 2);
                   EXPECT(b == 1.0);
                   std::cout << "let_async_scope done" << '\n';
               });
    mcs::this_thread::sync_wait(snd);
}

void base_test4()
{
    ex::static_thread_pool<1> pool;
    auto sch = pool.get_scheduler();
    bool then_called = false;
    auto snd = ex::just() | ex::then([]() {}) |
               ex::let_async_scope([&](auto scope_token) noexcept {
                   // NOLINTEND
                   static_assert(ex::scope::async_scope_token<decltype(scope_token)>);

                   ex::spawn(ex::starts_on(sch, ex::just() | ex::then([&] {
                                                    then_called = true;

                                                    std::cout
                                                        << "spawn  work on pool done"
                                                        << '\n';
                                                })),
                             scope_token);
                   return ex::just();
               }) |
               ex::then([&]() {
                   EXPECT(then_called);
                   std::cout << "let_async_scopedone" << '\n';
               });
    mcs::this_thread::sync_wait(std::move(snd));
}

void base_test5()
{
    ex::static_thread_pool<1> pool;
    auto sch = pool.get_scheduler();
    bool then_called = false;
    auto snd = ex::just() | ex::then([]() {}) |
               ex::let_async_scope([&](auto scope_token) noexcept {
                   // NOLINTEND
                   static_assert(ex::scope::async_scope_token<decltype(scope_token)>);

                   ex::spawn(ex::starts_on(sch,
                                           [](auto &then_called) noexcept -> ex::task<> {
                                               then_called = true;
                                               co_return;
                                           }(then_called)),
                             scope_token);
                   return ex::just();
               }) |
               ex::then([&]() {
                   EXPECT(then_called);
                   std::cout << "let_async_scopedone" << '\n';
               });
    mcs::this_thread::sync_wait(std::move(snd));
}

void base_test6()
{
    bool then_called = false;
    auto ret = [&]() noexcept -> ex::task<bool> {
        co_return true;
    }() | ex::then([&](auto ret) noexcept { then_called = true; });
    static_assert(ex::sender<decltype(ret)>);

    ex::counting_scope scope;
    ex::static_thread_pool<1> pool;

    ex::spawn(ex::starts_on(pool.get_scheduler(), std::move(ret)), scope.get_token());

    mcs::this_thread::sync_wait(scope.join());

    EXPECT(then_called);
}

int main()
{
    base_test();
    base_test2();
    base_test3();
    base_test4();
    base_test5();
    base_test6();

    TEST("BASE") = [] {
        auto rc = mcs::this_thread::sync_wait([]() -> ex::task<bool> {
            co_return true;
        }());
        assert(rc);
    };

    TEST("scope") = [&] {
        ex::static_thread_pool<1> pool;
        auto sch = pool.get_scheduler();
        bool then_called = false;
        auto snd =
            ex::just() | ex::then([]() {}) |
            ex::let_async_scope([&](auto scope_token) noexcept {
                // NOLINTEND
                static_assert(ex::scope::async_scope_token<decltype(scope_token)>);

                ex::spawn(ex::starts_on(sch,
                                        [](auto &then_called) noexcept -> ex::task<> {
                                            std::this_thread::sleep_for(
                                                std::chrono::milliseconds(10));
                                            then_called = true;
                                            co_return;
                                        }(then_called)),
                          scope_token);
                return ex::just();
            }) |
            ex::then([&]() {
                EXPECT(then_called);
                std::cout << "let_async_scope done\n";
            });
        mcs::this_thread::sync_wait(std::move(snd));
    };
    std::cout << "main done\n";
    return 0;
}
// NOLINTEND