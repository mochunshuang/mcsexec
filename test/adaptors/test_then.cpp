#include "../test_base_head.hpp"

int main()
{
    using namespace mcs::execution; // NOLINT

    TEST("then returns a sender") = [] {
        auto snd = ex::then(ex::just(), [] {});
        static_assert(ex::sender<decltype(snd)>);
    };
    TEST("then with environment returns a sender") = [] {
        auto snd = ex::then(ex::just(), [] {});
        static_assert(ex::sender_in<decltype(snd), empty_env>);
    };
    TEST("then simple example") = [] {
        bool called{false};
        bool fun_called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        auto snd = ex::then(ex::just(), [&] { fun_called = true; });
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
    TEST("then can be piped") = [] {
        ex::sender auto snd [[maybe_unused]] = ex::just() | ex::then([] {});
    };
    TEST("then returning void can we waited on") = [] {
        ex::sender auto snd = ex::just() | ex::then([] {});
        mcs::this_thread::sync_wait(snd);
    };
    TEST("then can be used to transform the value") = [] {
        auto snd =
            ex::just(3) | ex::then([](int x) -> double { return x + 0.1415; }); // NOLINT
        auto [ret] = mcs::this_thread::sync_wait(snd).value();
        EXPECT(ret == 3.1415); // NOLINT
    };
    TEST("then can be used to change the value type") = [] {
        auto snd = ex::just(3, 0.1415) | // NOLINT
                   ex::then([](int x, double y) -> double { return x + y; });
        wait_for_value(std::move(snd), 3.1415); // NOLINT
    };
    TEST("then can throw, and set_error will be called") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};
        auto snd = ex::just(1) //
                   | ex::then([](int) -> int { throw std::logic_error{"err"}; });
        auto op = ex::connect(
            snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});

        EXPECT(not called);
        EXPECT(c == test::channel::NO_CALL);
        start(op);
        EXPECT(called);
        EXPECT(c == test::channel::ERROR_CHANNEL);
    };
    TEST("then can be used with just_stopped") = [] {
        // because just_stopped -> set_stoped(recv,arg...) -> basic_receiver->
        // set_stopped() -> general::impls_for<then_t>::complete -> then_fun not called

        bool called{false};
        // "then function is not called when cancelled"
        bool fun_called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        ex::sender auto snd = ex::just_stopped() | ex::then([&]() -> int {
                                  fun_called = true;
                                  return 1;
                              });
        auto op = ex::connect(
            snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});

        EXPECT(not called);
        EXPECT(not fun_called);
        EXPECT(c == test::channel::NO_CALL);
        start(op);
        EXPECT(called);
        EXPECT(not fun_called);
        EXPECT(c == test::channel::STOPDE_CHANNEL);
    };
    TEST("then can be used with just_error") = [] {
        bool called{false};
        // "then function is not called on error"
        bool fun_called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        ex::sender auto snd = ex::just_error(std::string{"err"}) //
                              | ex::then([&]() -> int {
                                    fun_called = true;
                                    return 1;
                                });
        auto op = ex::connect(
            snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});

        EXPECT(not called);
        EXPECT(not fun_called);
        EXPECT(c == test::channel::NO_CALL);
        start(op);
        EXPECT(called);
        EXPECT(not fun_called);
        EXPECT(c == test::channel::ERROR_CHANNEL);

        auto str = std::any_cast<std::string>(any);
        EXPECT(str == "err");
    };

    TEST("sync_wait : then can be used with just_stopped") = [] {
        ex::sender auto snd = ex::just_stopped() | ex::then([&]() -> int { return 1; });
        using T = decltype(snd);
        using CS [[maybe_unused]] = ex::cmplsigs::get_completion_signatures<T>;

        try
        {
            auto ret = mcs::this_thread::sync_wait(snd);
            EXPECT(not ret.has_value());
        }
        catch (const std::exception &)
        {
            UNEXPECT(" UNEXPECT....");
        }
    };

    TEST("sync_wait : then can be used with just_error") = [] {
        ex::sender auto snd =
            ex::just_error(std::string{"err"}) | ex::then([&]() -> int { return 1; });
        using T = decltype(snd);
        using CS [[maybe_unused]] = ex::cmplsigs::get_completion_signatures<T>;

        try
        {
            auto ret [[maybe_unused]] = mcs::this_thread::sync_wait(snd);
            UNEXPECT(" UNEXPECT....");
        }
        catch (const std::string &err) // 直接捕获 std::string
        {
            EXPECT(err == "err");
        }
        catch (...)
        {
            UNEXPECT(" UNEXPECT....");
        }
    };

    return 0;
}