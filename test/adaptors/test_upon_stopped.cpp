#include "../test_base_head.hpp"

int main()
{
    TEST("upon_stopped returns a sender") = [] {
        auto snd = ex::upon_stopped(ex::just_stopped(), []() {});
        static_assert(ex::sender<decltype(snd)>);
    };

    TEST("upon_stopped with environment returns a sender") = [] {
        auto snd = ex::upon_stopped(ex::just_stopped(), []() {});
        static_assert(ex::sender_in<decltype(snd), ex::empty_env>);
    };

    TEST("upon_stopped simple example") = [] {
        bool called{false};
        bool called_fun{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};
        auto snd = ex::upon_stopped(ex::just_stopped(), [&]() {
            called_fun = true;
            return 0;
        });

        auto op = connect(
            snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});

        EXPECT(not called);
        EXPECT(not called_fun);
        EXPECT(c == test::channel::NO_CALL);

        start(op);

        EXPECT(called);
        EXPECT(called_fun);
        // Note: upon_stopped 的 fun 的结果 以 V_SIG 传递
        EXPECT(c == test::channel::VALUE_CHANNEL);
    };

    return 0;
}