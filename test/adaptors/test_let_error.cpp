
#include "../test_base_head.hpp"
#include <algorithm>
#include <cassert>
#include <tuple>

int main()
{
    {
        auto sndr =
            ex::let_error(ex::just(), [](std::exception_ptr) { return ex::just(); });
        "let_error returns a sender"_test = [&] {
            static_assert(ex::sender<decltype(sndr)>);
        };
        "let_error with environment returns a sender"_test = [&] {
            static_assert(ex::sender_in<decltype(sndr), ex::empty_env>);
        };
    }
    TEST("let_error simple example") = [] {
        bool called{false};
        bool fun_called{false};
        auto snd = ex::let_error(ex::just_error(std::exception_ptr{}),
                                 [&](const std::exception_ptr &) {
                                     fun_called = true;
                                     return ex::just();
                                 });
        test::channel chanel{test::channel::NO_CALL};
        auto op = connect(std::move(snd),
                          test::void_receiver{.called = &called, .chanel = &chanel});
        EXPECT(not fun_called && not called && chanel == test::channel::NO_CALL);
        start(op);
        EXPECT(fun_called && called && chanel == test::channel::VALUE_CHANNEL);
    };

    TEST("let_error simple example reference") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};
        auto snd = ex::let_error(ex::split(ex::just_error(std::exception_ptr{})),
                                 [&](const std::exception_ptr &) {
                                     return ex::just(404); // NOLINT
                                 });

        auto op = ex::connect(
            std::move(snd),
            test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        EXPECT(not called);
        EXPECT(c == test::channel::NO_CALL);
        start(op);
        EXPECT(called);
        EXPECT(c == test::channel::VALUE_CHANNEL);

        auto [ret] = std::any_cast<std::tuple<int>>(any);
        EXPECT(ret == 404);
    };

    TEST("let_error can be piped") = [] {
        ex::sender auto snd [[maybe_unused]] =
            ex::just() | ex::let_error([](std::exception_ptr &&) { return ex::just(); });
    };

    TEST("let_error CS") = [] {
        ex::sender auto snd [[maybe_unused]] =
            ex::just() | ex::let_error([](std::exception_ptr &&) { return ex::just(); });
        using T = decltype(snd);
        using CS = ex::cmplsigs::get_completion_signatures<T>;
        static_assert(std::is_same_v<CS, mcs::execution::cmplsigs::completion_signatures<
                                             mcs::execution::recv::set_value_t()>>);
        {
            ex::sender auto snd [[maybe_unused]] =
                ex::just() | ex::let_error([](std::exception_ptr &&) {
                    return ex::just(1, 1.0, 1.0F);
                });
            using T = decltype(snd);
            using CS = ex::cmplsigs::get_completion_signatures<T>;
            static_assert(
                std::is_same_v<
                    CS, mcs::execution::cmplsigs::completion_signatures<
                            mcs::execution::recv::set_value_t(int, double, float)>>);
        }
    };
#if 1
    TEST("let_error simple example reference") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        ex::sender auto snd = ex::just() //
                              | ex::then([]() -> std::string {
                                    throw std::logic_error{"error description"};
                                    return {};
                                }) //
                              | ex::let_error([](std::exception_ptr eptr) {
                                    try
                                    {
                                        std::rethrow_exception(std::move(eptr));
                                    }
                                    catch (const std::exception &e)
                                    {
                                        return ex::just(std::string{e.what()});
                                    }
                                });

        auto op = ex::connect(
            std::move(snd),
            test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        EXPECT(not called);
        EXPECT(c == test::channel::NO_CALL);
        start(op);
        EXPECT(called);
        EXPECT(c == test::channel::VALUE_CHANNEL);

        auto [ret] = std::any_cast<std::tuple<std::string>>(any);
        EXPECT(ret == "error description");
    };
#endif
    TEST("let_error returning void can be waited on") = [] {
        ex::sender auto snd =
            ex::just_error(std::exception_ptr{}) |
            ex::let_error([](const std::exception_ptr &) { return ex::just(); });
        mcs::this_thread::sync_wait(std::move(snd));
    };

    return 0;
}