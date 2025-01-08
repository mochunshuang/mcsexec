#include "../test_base_head.hpp"
#include <string>
#include <tuple>
#include <variant>

#include "../sched/MyScheduler.hpp"

int main()
{

    TEST("when_all returns a sender") = [] {
        auto snd = ex::when_all(ex::just(3), ex::just(0.1415)); // NOLINT
        static_assert(ex::sender<decltype(snd)>);
    };
    TEST("when_all with environment returns a sender") = [] {
        auto snd = ex::when_all(ex::just(3), ex::just(0.1415)); // NOLINT
        static_assert(ex::sender_in<decltype(snd), ex::empty_env>);
    };

    TEST("when_all simple example") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        auto snd = ex::when_all(ex::just(3), ex::just(0.1415)); // NOLINT
        auto snd1 = snd | ex::then([](int x, double y) { return x + y; });
        auto op = connect(
            snd1, test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        start(op);
        EXPECT(c == test::channel::VALUE_CHANNEL);

        auto [ret] = std::any_cast<std::tuple<double>>(any);
        EXPECT(ret == 3.1415); // NOLINT
    };

    TEST("when_all returning two values can we waited on") = [] {
        ex::sender auto snd = ex::when_all( //
            ex::just(2),                    //
            ex::just(3)                     //
        );
        auto [a, b] = mcs::this_thread::sync_wait(snd).value();
        EXPECT(a == 2);
        EXPECT(b == 3);
    };

    TEST("when_all with 5 senders") = [] {
        ex::sender auto snd = ex::when_all( //
            ex::just(2),                    // NOLINT
            ex::just(3),                    // NOLINT
            ex::just(5),                    // NOLINT
            ex::just(),                     // NOLINT
            ex::just(std::string{11})       // NOLINT
        );
        auto ret = mcs::this_thread::sync_wait(snd).value();
        EXPECT(ret == std::make_tuple(2, 3, 5, std::string{11}));
    };

    TEST("when_all with just one sender") = [] {
        ex::sender auto snd = ex::when_all( //
            ex::just(2)                     //
        );
        auto [ret] = mcs::this_thread::sync_wait(snd).value();
        EXPECT(ret == 2);
    };

    TEST("when_all with move-only types") = [] {
        ex::sender auto snd = ex::when_all( //
            ex::just(move_only_type{2})     //
        );
        // auto [ret] = mcs::this_thread::sync_wait(snd).value(); //编译错误
        auto [ret] = mcs::this_thread::sync_wait(std::move(snd)).value();
        EXPECT(ret.val == 2);
    };

    TEST("when_all when one sender sends void") = [] {
        ex::sender auto snd = ex::when_all( //
            ex::just(2),                    //
            ex::just()                      //
        );
        auto [ret] = mcs::this_thread::sync_wait(snd).value();
        EXPECT(ret == 2);
    };

    TEST("when_all can be used with just_*") = [] {
        ex::sender auto snd = ex::when_all(       //
            ex::just(2),                          //
            ex::just_error(std::exception_ptr{}), //
            ex::just_stopped()                    //
        );
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};
        auto op = connect(
            snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        start(op);
        // Note: 虽然按顺序指向。但是error 之后，不会继续调用  just_stopped
        EXPECT(c == test::channel::ERROR_CHANNEL);
        {
            ex::sender auto snd = ex::when_all( //
                ex::just(2), ex::just_stopped(), ex::just_error(std::exception_ptr{}));
            bool called{false};
            std::any any;
            test::channel c{test::channel::NO_CALL};
            auto op = connect(
                snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});
            start(op);
            // Note: 虽然按顺序指向，但是 还是 ERROR_CHANNEL。 因为  error 是最后调用
            EXPECT(c == test::channel::ERROR_CHANNEL);
        }
    };

    TEST("when_all_with_variant") = [] {
        auto snd = ex::starts_on(MyScheduler(),
                                 ex::when_all_with_variant( //
                                     ex::just(3),           //
                                     ex::just(0.1415)       // NOLINT
                                     ));
        auto [a, b] = mcs::this_thread::sync_wait(snd).value();
        std::visit(
            [](auto &&value) {
                int v = std::get<0>(value);
                EXPECT(v == 3);
            },
            a);
        std::visit(
            [](auto &&value) {
                auto [v] = value;
                EXPECT(v == 0.1415); // NOLINT
            },
            b);
    };
    TEST("when_all_with_variant split test") = [] {
        auto pre_snd = ex::when_all_with_variant( //
            ex::just(3),                          //
            ex::just(0.1415)                      // NOLINT
        );
        auto snd = ex::starts_on(MyScheduler(), pre_snd);
        auto [a, b] = mcs::this_thread::sync_wait(snd).value();
        std::visit(
            [](auto &&value) {
                int v = std::get<0>(value);
                EXPECT(v == 3);
            },
            a);
        std::visit(
            [](auto &&value) {
                auto [v] = value;
                EXPECT(v == 0.1415); // NOLINT
            },
            b);
    };
    return 0;
}