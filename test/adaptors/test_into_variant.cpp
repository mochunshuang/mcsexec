#include "../test_base_head.hpp"
#include <tuple>
#include <variant>

int main()
{
    TEST("into_variant returns a sender") = [] {
        auto snd = ex::into_variant(ex::just(1));
        static_assert(ex::sender<decltype(snd)>);
    };
    TEST("into_variant with environment returns a sender") = [] {
        auto snd = ex::into_variant(ex::just(1));
        static_assert(ex::sender_in<decltype(snd), mcs::execution::empty_env>);
    };
    TEST("into_variant simple example") = [] {
        bool called{false};
        bool fun_called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        auto snd = ex::into_variant(ex::just(11)) //
                   | ex::then([&](std::variant<std::tuple<int>> x) {
                         fun_called = true;
                         EXPECT(std::get<0>(std::get<0>(x)) == 11);
                     });

        auto op = ex::connect(
            std::move(snd),
            test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        EXPECT(not called);
        EXPECT(c == test::channel::NO_CALL);
        start(op);
        EXPECT(called);
        EXPECT(fun_called);
        EXPECT(c == test::channel::VALUE_CHANNEL);
    };

    TEST("into_variant returning void can we waited on") = [] {
        ex::sender auto snd = ex::just(1) | ex::into_variant();
        auto [ret] = mcs::this_thread::sync_wait(std::move(snd)).value();
        static_assert(std::is_same_v<std::variant<std::tuple<int>>, decltype(ret)>);
        std::visit(
            [](auto &&value) {
                int v = std::get<0>(value);
                EXPECT(v == 1);
            },
            ret);
        auto &[r] = std::get<std::tuple<int>>(ret);
        EXPECT(r == 1);
    };

    TEST("into_variant with senders that sends multiple values at once") = [] {
        ex::sender auto snd = ex::just(3, 0.1415) | ex::into_variant(); // NOLINT
        auto [ret] = mcs::this_thread::sync_wait(std::move(snd)).value();
        static_assert(
            std::is_same_v<std::variant<std::tuple<int, double>>, decltype(ret)>);
        std::visit(
            [](auto &&value) {
                auto &[a, b] = value;
                EXPECT(a == 3);
                EXPECT(b == 0.1415);
            },
            ret);
    };

    TEST("into_variant can be used with just_error") = [] {
        using namespace mcs::execution; // NOLINT
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};
        ex::sender auto snd = ex::just_error(std::string{"err"}) //
                              | ex::into_variant();

        auto op =
            connect(std::move(snd),
                    test::any_receiver{.called = &called, .data = &any, .chanel = &c});

        EXPECT(not called);
        EXPECT(c == test::channel::NO_CALL);
        start(op);
        EXPECT(called);
        EXPECT(c == test::channel::ERROR_CHANNEL);

        auto ret = std::any_cast<std::string>(any);
        EXPECT(ret == "err");
    };

    TEST("into_variant can be used with just_stopped") = [] {
        using namespace mcs::execution; // NOLINT
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};
        ex::sender auto snd = ex::just_stopped() | ex::into_variant();
        auto op =
            connect(std::move(snd),
                    test::any_receiver{.called = &called, .data = &any, .chanel = &c});

        EXPECT(not called);
        EXPECT(c == test::channel::NO_CALL);
        start(op);
        EXPECT(called);
        EXPECT(c == test::channel::STOPDE_CHANNEL);
    };

    return 0;
}