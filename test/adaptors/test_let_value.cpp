
#include "../test_base_head.hpp"
#include <cassert>

int main()
{

    auto sndr = ex::let_value(ex::just(), [] { return ex::just(); });
    "let_value returns a sender"_test = [&] {
        static_assert(ex::sender<decltype(sndr)>);
    };
    "let_value with environment returns a sender"_test = [&] {
        // 检查 Sndr 是否可以在 Rcvr 的环境中作为 sender 使用
        // sender_in 检查 sndr + env 能不能 生成完成签名
        static_assert(ex::sender_in<decltype(sndr), ex::empty_env>);
    };

    TEST("let_value simple example") = [] {
        bool called{false};
        auto snd = ex::let_value(ex::just(), [&] {
            called = true;
            return ex::just();
        });
        bool called2{false};
        test::channel chanel{test::channel::NO_CALL};
        auto op = ex::conn::connect(
            std::move(snd), test::void_receiver{.called = &called2, .chanel = &chanel});
        EXPECT(not called && not called2 && chanel == test::channel::NO_CALL);
        start(op);
        EXPECT(called && called2 && chanel == test::channel::VALUE_CHANNEL);
    };

    TEST("let_value can be piped") = [] {
        [[maybe_unused]] ex::sender auto snd =
            ex::just() | ex::let_value([] { return ex::just(); });
    };

    TEST("let_value returning void, can we waited on?") = [] {
        ex::sender auto snd = ex::just() | ex::let_value([] { return ex::just(); });
        mcs::this_thread::sync_wait(std::move(snd));
    };

    TEST("let_value can be used to produce values") = [] {
        ex::sender auto snd =
            ex::just() | ex::let_value([] { return ex::just(1, 2, 3); });
        auto ret = mcs::this_thread::sync_wait(std::move(snd));
        static_assert(
            std::is_same_v<decltype(ret), std::optional<std::tuple<int, int, int>>>);
        if (ret.has_value())
        {
            auto [a, b, c] = ret.value();
            EXPECT(a == 1 and b == 2 and c == 3);
        }
    };

    TEST("let_value can be used to transform values") = [] {
        ex::sender auto snd =
            ex::just(13) | ex::let_value([](int x) { return ex::just(x + 4); });
        auto [ret] = mcs::this_thread::sync_wait(std::move(snd)).value();
        EXPECT(ret == 17);
    };
    TEST("let_value returning void can be waited on") = [] {
        ex::sender auto snd = ex::let_value(ex::just(), [] { return ex::just(); });
        mcs::this_thread::sync_wait(std::move(snd));
        {
            ex::sender auto snd =
                ex::let_value(ex::just(1), [](int) { return ex::just(); });
            mcs::this_thread::sync_wait(std::move(snd));
        }
    };

    TEST("cpo for let_value") = [] {
        ex::sender auto snd =
            ex::just() | ex::let_value([] { return ex::just(1, 2, 3); });
        using T = decltype(snd);
        using CO = ex::cmplsigs::get_completion_signatures<T>;
        static_assert(
            std::is_same_v<ex::cmplsigs::completion_signatures<
                               mcs::execution::recv::set_value_t(int, int, int)>,
                           CO>);
    };

    TEST("let_value can be used with multiple parameters") = [] {
        auto snd = ex::just(3, 0.1415) |
                   ex::let_value([](int x, double y) { return ex::just(x + y); });
        auto [ret] = mcs::this_thread::sync_wait(std::move(snd)).value();
        EXPECT(ret == 3.1415);
    };

    TEST("let_value can be used to change the sender") = [] {
        bool called{false};
        int err_code = 17;
        ex::sender auto snd =
            ex::just(13) | ex::let_value([](int x) { return ex::just_error(x + 4); });
        auto op = connect(std::move(snd),
                          test::error_receiver{.called = &called, .error = err_code});
        EXPECT(not called);
        start(op);
        EXPECT(called);
    };

    TEST("let_value can be used for composition") = [] {
        auto is_prime = [](int x) {
            if (x > 2 && (x % 2 == 0))
                return false;
            int d = 3;
            while (d * d < x)
            {
                if (x % d == 0)
                    return false;
                d += 2;
            }
            return true;
        };
        bool called1{false};
        bool called2{false};
        bool called3{false};
        auto f1 = [&](int x) {
            called1 = true;
            return ex::just(2 * x);
        };
        auto f2 = [&](int x) {
            called2 = true;
            return ex::just(x + 3);
        };
        auto f3 = [&](int x) {
            called3 = true;
            if (!is_prime(x))
                throw std::logic_error("not prime");
            return ex::just(x);
        };
        ex::sender auto snd = ex::just(13)        //
                              | ex::let_value(f1) //
                              | ex::let_value(f2) //
                              | ex::let_value(f3) //
            ;
        auto [ret] = mcs::this_thread::sync_wait(std::move(snd)).value();
        EXPECT(ret == ((13 * 2) + 3));
        EXPECT(called1);
        EXPECT(called2);
        EXPECT(called3);
    };

    TEST("let_value can throw, and set_error will be called") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};
        auto snd = ex::just(13) //
                   | ex::let_value([](int &) -> decltype(ex::just(0)) {
                         throw std::logic_error{"err"};
                     });

        auto op = ex::connect(
            std::move(snd),
            test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        EXPECT(not called);
        EXPECT(c == test::channel::NO_CALL);
        start(op);
        EXPECT(called);
        EXPECT(c == test::channel::ERROR_CHANNEL);

        /**
         * @brief std::exception_ptr
         * 是一个指向异常的智能指针，它可以捕获并保存任何类型的异常。
         * 要访问异常的具体信息，必须通过 std::rethrow_exception 重新抛出异常，然后在
         * catch 块中处理。
         */
        try
        {
            // 从 std::any 中提取 std::exception_ptr
            auto e = std::any_cast<std::exception_ptr>(any);
            std::rethrow_exception(e);
        }
        catch (const std::logic_error &ex)
        {
            std::string whatStr = ex.what();
            // Note: 字符串处理变成string 才行，离谱
            EXPECT(ex.what() != "err");
            EXPECT(whatStr == "err");
        }
    };

    TEST("let_value can be used with just_error") = [] {
        bool called{false};
        bool called_fun{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        ex::sender auto snd = ex::just_error(std::string{"err"}) //
                              | ex::let_value([&]() {
                                    called_fun = true;
                                    return ex::just(17);
                                });
        auto op =
            connect(std::move(snd),
                    test::any_receiver{.called = &called, .data = &any, .chanel = &c});

        EXPECT(not called);
        EXPECT(not called_fun);
        EXPECT(c == test::channel::NO_CALL);
        start(op);
        EXPECT(called);
        // Note: Requirement 3:  using let_value_t = __let_t<set_value_t>;
        // Note: just_error => let_value， let_value 的完成行为是转发
        EXPECT(not called_fun);
        EXPECT(c == test::channel::ERROR_CHANNEL);

        auto str = std::any_cast<std::string>(any);
        EXPECT(str == std::string{"err"});
    };

    TEST("let_value can be used with just_stopped") = [] {
        bool called{false};
        bool called_fun{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        ex::sender auto snd =
            ex::just_stopped() | ex::let_value([]() { return ex::just(1); });
        auto op =
            connect(std::move(snd),
                    test::any_receiver{.called = &called, .data = &any, .chanel = &c});

        EXPECT(not called);
        EXPECT(not called_fun);
        EXPECT(c == test::channel::NO_CALL);

        start(op);

        EXPECT(called);
        EXPECT(not called_fun);
        EXPECT(c == test::channel::STOPDE_CHANNEL);
    };

    struct need_add_exception_ptr_receiver
    {
        using receiver_concept = ex::receiver_t;

        void set_value() noexcept
        {
            completed = true;
        }

        void set_error(std::exception_ptr) noexcept
        {
            completed = true;
        }

        constexpr auto get_env() const noexcept // NOLINT
        {
            return mcs::execution::empty_env{};
        }

        bool &completed;
    };

    // 额外的异常签名
    TEST("let_value does add std::exception_ptr for if except") = [] {
        auto snd = ex::let_value(ex::just(), []() noexcept { return ex::just(); });
        bool completed{false};
        auto op = ex::connect(std::move(snd),
                              need_add_exception_ptr_receiver{.completed = completed});

        EXPECT(not completed);
        start(op);
        EXPECT(completed);

        // receiver 需要额外的 std::exception_ptr 通道，因为还处理可能的异常
        using T = decltype(snd);
        using CO = ex::cmplsigs::get_completion_signatures<T>;
        static_assert(
            std::is_same_v<
                ex::cmplsigs::completion_signatures<mcs::execution::recv::set_value_t()>,
                CO>);
    };
    return 0;
}