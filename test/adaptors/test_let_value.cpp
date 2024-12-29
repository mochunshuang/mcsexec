
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
        test::Channel chanel{test::Channel::NO_CALL};
        auto op = ex::conn::connect(
            std::move(snd), test::void_receiver{.called = &called2, .chanel = &chanel});
        EXPECT(not called && not called2 && chanel == test::Channel::NO_CALL);
        start(op);
        EXPECT(called && called2 && chanel == test::Channel::VALUE_CHANNEL);
    };

    return 0;
}