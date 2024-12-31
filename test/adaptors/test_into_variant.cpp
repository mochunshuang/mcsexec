#include "../test_base_head.hpp"

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
        ex::sender auto snd = ex::just(11) | ex::into_variant();
    };

    return 0;
}