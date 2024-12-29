
#include "../test_base_head.hpp"
#include <cassert>
#include <optional>
#include <tuple>

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
            std::cout << "Value: " << a << " " << b << " " << c << "\n";
        }
    };

    TEST("let_value can be used to transform values") = [] {
        ex::sender auto snd =
            ex::just(13) | ex::let_value([](int x) { return ex::just(x + 4); });
        auto [ret] = mcs::this_thread::sync_wait(std::move(snd)).value();
        EXPECT(ret == 17);
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

    return 0;
}