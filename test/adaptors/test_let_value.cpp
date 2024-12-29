
#include "../test_base_head.hpp"

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
        static_assert(ex::recv::receiver<test::expect_void_receiver<>>);
        // auto op = ex::conn::connect(std::move(snd), test::expect_void_receiver<>{});
        // The receiver checks that it's called
        // we also check that the function was invoked
        // ex::opstate::start(op);
    };

    return 0;
}