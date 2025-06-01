#include "../test_base_head.hpp"
#include <chrono>
#include <iostream>
#include <thread>
#include <utility>

int main()
{
    static_assert(ex::snd::sender<mcs::execution::task::lazy<int>>);

    TEST(" lazy<int> is sender") = [] {
        static_assert(ex::snd::sender<mcs::execution::task::lazy<int>>);
    };

    TEST("lazy<int> ") = [] {
        auto rc = mcs::this_thread::sync_wait([] -> ex::lazy<int> { // NOLINT
            co_return 17;                                           // NOLINT
        }());
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 17);
    };

    TEST("lazy<int> ") = [] {
        auto rc = mcs::this_thread::sync_wait([] -> ex::lazy<int> { // NOLINT
            co_return 17;                                           // NOLINT
        }() | ex::then([](int i) { return 1 + i; }));
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 18);
    };

    TEST("co_await ") = [] {
        [[maybe_unused]] auto o = mcs::this_thread::sync_wait([]() -> ex::lazy<> {
            co_await ex::just(); // void // NOLINT
            std::cout << "after co_await ex::just()\n";
            [[maybe_unused]] auto v = co_await ex::just(42); // int // NOLINT
            assert(v == 42);
            [[maybe_unused]] auto [i, b, c] =
                co_await ex::just(17, true, 'c'); // tuple<int, bool, char> // NOLINT
            assert(i == 17 && b == true && c == 'c');
            try
            {
                co_await ex::just_error(-1); // exception
                assert(nullptr == "never reached");
            }
            catch (int e)
            {
                assert(e == -1);
            }
            std::cout << "about to cancel\n";
            try
            {
                co_await ex::just_stopped(); // NOLINT
            }
            catch (...) // NOLINT
            {
            } // cancel: never resumed
            assert(nullptr == "never reached");
        }());
        assert(not o);
    };

    TEST("co_await 2 ") = [] {
        auto fun = [] -> ex::lazy<int> {
            int i = 0;
            co_await ex::just(i);
            co_return -1;
        };
        {
            auto [ret] =
                mcs::this_thread::sync_wait(fun() | ex::then([](int i) {
                                                std::cout << "[co_await 2]: then call\n";
                                                std::cout << "i: " << i << '\n';
                                                return i;
                                            }))
                    .value();
            EXPECT(ret == -1);
        }
    };

    TEST("co_await 3 ") = [] {
        auto fun = [] -> ex::lazy<int> {
            auto [a, b] = co_await ex::just(std::make_pair(-1, "name"));
            co_return a;
        };
        {
            auto [ret] =
                mcs::this_thread::sync_wait(fun() | ex::then([](int i) {
                                                std::cout << "[co_await 2]: then call\n";
                                                std::cout << "i: " << i << '\n';
                                                return i;
                                            }))
                    .value();
            EXPECT(ret == -1);
        }
    };

    TEST("co_await 4 ") = [] {
        auto fun = [] -> ex::lazy<int> {
            auto [a, b] = co_await (ex::just(1) | ex::then([](auto p) noexcept {
                                        if (p > 0)
                                            return std::make_pair(-1, "name");
                                        return std::make_pair(p, "name");
                                    }));
            co_return a;
        };
        {
            auto [ret] =
                mcs::this_thread::sync_wait(fun() | ex::then([](int i) {
                                                std::cout << "[co_await 2]: then call\n";
                                                std::cout << "i: " << i << '\n';
                                                return i;
                                            }))
                    .value();
            EXPECT(ret == -1);
        }
    };

    TEST("co_yield only for error ") = [] {
        auto fun = [] -> ex::lazy<int> {
            co_yield mcs::execution::task::with_error{-99}; // NOLINT
            UNEXPECT("never reached");
            co_return -1;
        };
        {
            auto [ret] = mcs::this_thread::sync_wait(fun() | ex::then([](int i) {
                                                         std::cout
                                                             << "[co_yield]: then call\n";
                                                         std::cout << "i: " << i << '\n';
                                                         return i;
                                                     }))
                             .value();
            EXPECT(ret == -99);
        }
    };

    // Note: 目前都不支持 co_yield + while 做正常的数据处理
    TEST("with while(true)") = [] {
        auto rc = mcs::this_thread::sync_wait([] -> ex::lazy<int> { // NOLINT
            int i = 100000;                                         // NOLINT
            while (true)
            {
                if (i-- == 0)
                    co_return 1;
            }
        }());
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 1);
    };

    TEST("with while(i-->0)") = [] {
        auto rc = mcs::this_thread::sync_wait([] -> ex::lazy<int> { // NOLINT
            int i = 3;                                              // NOLINT
            while (i-- > 0)
            {
                auto ret = co_await (ex::just(i) | ex::then([](int i) noexcept {
                                         std::this_thread::sleep_for(
                                             std::chrono::milliseconds(i));
                                         return i;
                                     }));
                std::cout << "while + co_await: " << ret << '\n';
            }
            co_return 1;
        }());
        assert(rc);
        auto [value] = rc.value_or(std::tuple{0});
        EXPECT(value == 1);
    };

    return 0;
}