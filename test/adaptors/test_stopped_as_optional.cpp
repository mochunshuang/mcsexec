#include "../test_base_head.hpp"

int main()
{
    TEST("stopped_as_optional returns a sender") = [] {
        auto snd = ex::stopped_as_optional(ex::just(1));
        static_assert(ex::sender<decltype(snd)>);
    };

    TEST("stopped_as_optional with environment returns a sender") = [] {
        auto snd = ex::stopped_as_optional(ex::just(1));
        static_assert(ex::sender_in<decltype(snd)>);
    };

    TEST("stopped_as_optional simple example") = [] {
        ex::sender auto snd = ex::stopped_as_optional(ex::just(1));
        auto [ret] = mcs::this_thread::sync_wait(snd).value();
        EXPECT(ret.value() == 1);
    };

    return 0;
}