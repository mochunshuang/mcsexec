#include "../test_base_head.hpp"
#include <string>
#include <tuple>

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

    return 0;
}