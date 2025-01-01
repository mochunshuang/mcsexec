#include "../test_base_head.hpp"
#include <string>

int main()
{
    using namespace mcs::execution; // NOLINT
    TEST("let_stopped returns a sender") = [] {
        auto snd = ex::let_stopped(ex::just(), [] { return ex::just(); });
        static_assert(ex::sender<decltype(snd)>);
    };

    TEST("let_stopped with environment returns a sender") = [] {
        auto snd = ex::let_stopped(ex::just(), [] { return ex::just(); });
        static_assert(snd::sender_in<decltype(snd), empty_env>);
    };

    TEST("let_stopped simple example") = [] {
        bool called{false};
        bool fun_called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        auto snd = ex::let_stopped(ex::just_stopped(), [&] {
            fun_called = true;
            return ex::just();
        });

        auto op = ex::connect(
            snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        EXPECT(not called);
        EXPECT(not fun_called);
        EXPECT(c == test::channel::NO_CALL);
        start(op);
        EXPECT(called);
        EXPECT(fun_called);
        EXPECT(c == test::channel::VALUE_CHANNEL);
    };

    TEST("let_stopped can be piped") = [] {
        ex::sender auto snd [[maybe_unused]] =
            ex::just() | ex::let_stopped([] { return ex::just(); });
    };

    TEST("let_stopped returning void can we waited on (cancel annihilation)") = [] {
        ex::sender auto snd =
            ex::just_stopped() | ex::let_stopped([] { return ex::just(); });
        mcs::this_thread::sync_wait(snd);
    };

    TEST("let_stopped can throw, calling set_error") = [] {
        auto snd = ex::just_stopped() //
                   | ex::let_stopped([]() -> decltype(ex::just(0)) {
                         throw std::logic_error{"err"};
                     });
        try
        {
            mcs::this_thread::sync_wait(snd);
            UNEXPECT(" UNEXPECT ....");
        }
        catch (const std::logic_error &e)
        {
            EXPECT(std::string{e.what()} == "err");
        }
        catch (...)
        {
            UNEXPECT(" UNEXPECT ....");
        }
    };

    TEST("let_stopped can be used with just_error") = [] {
        bool called{false};
        // "let_stopped function is not called on error flow"
        bool fun_called{false};
        auto error_code = 200;                           // NOLINT
        ex::sender auto snd = ex::just_error(error_code) //
                              | ex::let_stopped([&] {
                                    fun_called = true;
                                    return ex::just(1);
                                });
        auto op = ex::connect(
            snd, test::error_receiver<int>{.called = &called, .error = error_code});
        EXPECT(not called);
        EXPECT(not fun_called);
        start(op);
        EXPECT(called);
        EXPECT(not fun_called);
    };

    TEST("let_stopped function is not called on regular flow") = [] {
        bool called{false};
        ex::sender auto snd = ex::just(1) //
                              | ex::let_stopped([&] {
                                    called = true;
                                    return ex::just(0);
                                });
        auto [ret] = mcs::this_thread::sync_wait(snd).value();
        EXPECT(not called);
        EXPECT(ret = 1);
    };

    return 0;
}