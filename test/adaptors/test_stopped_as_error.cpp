#include "../test_base_head.hpp"
#include <any>

using namespace mcs::execution; // NOLINT

int main()
{
    TEST("stopped_as_error returns a sender") = [] {
        auto snd = ex::stopped_as_error(ex::just(1), -1);
        static_assert(ex::sender<decltype(snd)>);
    };
    TEST("stopped_as_error with environment returns a sender") = [] {
        auto snd = ex::stopped_as_error(ex::just(11), -1);
        static_assert(ex::sender_in<decltype(snd), empty_env>);
    };
    TEST("stopped_as_error simple example") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        ex::sender auto snd = ex::stopped_as_error(ex::just_stopped(), -1);
        auto op = connect(
            snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});

        start(op);

        EXPECT(called);
        EXPECT(c == test::channel::ERROR_CHANNEL);

        auto ret = std::any_cast<int>(any);
        EXPECT(ret == -1);
    };

    TEST("stopped_as_error pipeable") = [] {
        auto snd = ex::just(1) | ex::stopped_as_error(-1);
        using CS [[maybe_unused]] = snd::completion_signatures_of_t<decltype(snd)>;
        auto [ret] = mcs::this_thread::sync_wait(snd).value();
        EXPECT(ret == 1);
    };
    return 0;
}