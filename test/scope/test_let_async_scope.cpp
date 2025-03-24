#include "../test_base_head.hpp"

#include <cassert>
#include <iostream>
#include <stdexcept>
#include <type_traits>
#include <utility>

// NOLINTBEGIN
int main()
{
    using namespace mcs::execution;

    TEST("base") = [] {
        auto fun = [&](auto scope_token, auto &scoped_data, std::string &) noexcept {
            // NOLINTEND
            EXPECT(scoped_data == 1);
            static_assert(scope::async_scope_token<decltype(scope_token)>);
            return just(2, 1.0);
        };
        auto scope_sender = just(1, std::string("abc")) | let_async_scope(std::move(fun));
        using Sndr = decltype(scope_sender);
        using CS [[maybe_unused]] = snd::completion_signatures_of_t<Sndr>;
        using C [[maybe_unused]] =
            mcs::execution::consumers::__sync_wait::sync_wait_result_type<Sndr>;
        using Rcvr = consumers::__sync_wait::sync_wait_receiver<Sndr>;

        using child_type [[maybe_unused]] = snd::__detail::mate_type::child_type<Sndr>;
        using data_type = snd::__detail::mate_type::data_type<Sndr>;
        static_assert(std::is_same_v<std::decay_t<data_type>, decltype(fun)>);

        using CS2 [[maybe_unused]] = snd::completion_signatures_of_t<Sndr, Rcvr>;
        using State [[maybe_unused]] = snd::__detail::mate_type::state_type<Sndr, Rcvr>;

        using O [[maybe_unused]] = decltype(scope_sender.connect(Rcvr{}));
        static_assert(noexcept(scope_sender.connect(Rcvr{})));
        using OP [[maybe_unused]] =
            decltype(ex::conn::connect(std::forward<Sndr>(std::declval<Sndr>()),
                                       std::forward<Rcvr>(std::declval<Rcvr>())));
        auto [a, b] = mcs::this_thread::sync_wait(scope_sender).value();
        EXPECT(a == 2);
        EXPECT(b == 1.0);
    };

    TEST("base + then") = [] {
        auto scope_sender =
            just(1, std::string("abc")) |
            let_async_scope(
                [](auto scope_token, auto &scoped_data, std::string &) noexcept {
                    // NOLINTEND
                    EXPECT(scoped_data == 1);
                    static_assert(scope::async_scope_token<decltype(scope_token)>);
                    return just(2, 1.0);
                }) |
            then([](auto a, auto b) {
                EXPECT(a == 2);
                EXPECT(b == 1.0);
            });
        mcs::this_thread::sync_wait(scope_sender);
    };

    TEST("base + then + noexcept") = [] {
        bool then_called = false;
        auto scope_sender =
            just(1, std::string("abc")) | then([](auto, auto) noexcept {

            }) |
            let_async_scope([](auto scope_token) noexcept {
                // NOLINTEND
                static_assert(scope::async_scope_token<decltype(scope_token)>);
                return just(2, 1.0);
            }) |
            then([&](auto a, auto b) {
                EXPECT(a == 2);
                EXPECT(b == 1.0);
                then_called = true;
            });
        mcs::this_thread::sync_wait(scope_sender);
        EXPECT(then_called);
    };

    TEST("base + then + throw") = [] {
        bool upon_error_called = false;
        auto scope_sender =
            just(1, std::string("abc")) |
            then([](auto, auto) { throw std::logic_error("then throw"); }) |
            let_async_scope([](auto scope_token) noexcept {
                // NOLINTEND
                static_assert(scope::async_scope_token<decltype(scope_token)>);
                return just(2, 1.0);
            }) |
            then([](auto a, auto b) {
                EXPECT(a == 2);
                EXPECT(b == 1.0);
            }) |
            ex::upon_error([&](std::exception_ptr e) {
                try
                {
                    auto ret = std::any_cast<std::exception_ptr>(e);
                    std::rethrow_exception(ret);
                }
                catch (const std::logic_error &e)
                {
                    EXPECT(std::string_view(e.what()) == "then throw");
                }
                upon_error_called = true;
            });
        mcs::this_thread::sync_wait(scope_sender);
        EXPECT(upon_error_called);
    };

    TEST("base + then + throw 2 ") = [] {
        bool upon_error_called = false;
        auto scope_sender =
            just(1, std::string("abc")) | then([](auto, const auto &) { return; }) |
            let_async_scope([](auto scope_token) { // NOTE: need noexcept
                // NOLINTEND
                static_assert(scope::async_scope_token<decltype(scope_token)>);
                throw std::logic_error("let_async_scope throw");
                return just(2, 1.0);
            }) |
            then([](auto a, auto b) {
                EXPECT(a == 2);
                EXPECT(b == 1.0);
            }) |
            ex::upon_error([&](std::exception_ptr e) {
                try
                {
                    auto ret = std::any_cast<std::exception_ptr>(e);
                    std::rethrow_exception(ret);
                }
                catch (const std::logic_error &e)
                {
                    EXPECT(std::string_view(e.what()) == "let_async_scope throw");
                }
                upon_error_called = true;
            });
        mcs::this_thread::sync_wait(scope_sender);
        EXPECT(upon_error_called);
    };

    // TODO 有bug要解决
#if 0
    TEST("with spawn") = [] {
        //
        auto some_work = [&](int i) noexcept -> ex::sender auto {
            return ex::just() | ex::then([i]() {
                       if (i % 10 == 0)
                       {
                           std::cout << "handle work_id: " << i << " done\n";
                       }
                   });
        };
        bool done = false;
        auto foo = [&]() noexcept -> ex::sender auto {
            return ex::just() |
                   ex::let_async_scope([&](ex::async_scope_token auto scope) noexcept {
                       return ex::just() | ex::then([&] noexcept {
                                  std::cout << "Before tasks launch\n";
                              }) |
                              ex::then([&] noexcept {
                                  EXPECT(not done);
                                  for (int i = 0; i < 100; ++i) // NOLINT
                                  {
                                      ex::spawn(some_work(i), scope);
                                      EXPECT(not done);
                                  };
                                  EXPECT(not done);
                              });
                   }) |
                   ex::then([&] {
                       std::cout << "After tasks complete successfully\n";
                       done = true;
                   });
        };
        ex::sender auto work = foo();
        using CS [[maybe_unused]] = snd::completion_signatures_of_t<decltype(work)>;

        EXPECT(not done);
        mcs::this_thread::sync_wait(std::move(work));
        EXPECT(done);
    };
#endif
    std::cout << " main done\n";
    return 0;
}
// NOLINTEND