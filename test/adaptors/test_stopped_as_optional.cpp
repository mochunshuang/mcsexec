#include "../test_base_head.hpp"
#include <string>

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

    TEST("stopped_as_optional shall work with just(), as no std::optional<void>") = [] {
        ex::sender auto snd [[maybe_unused]] = ex::stopped_as_optional(ex::just());
        // mcs::this_thread::sync_wait(snd).value();
    };

    TEST("stopped_as_optional shall work with multi-value just senders") = [] {
        ex::sender auto snd = ex::stopped_as_optional(ex::just(1, 1.0));
        auto [ret] = mcs::this_thread::sync_wait(snd).value();
        auto [a, b] = ret.value();
        EXPECT(a == 1);
        EXPECT(b == 1.0);
    };

    TEST("stopped_as_optional shall not work with multi-value senders") = [] {
        ex::sender auto snd [[maybe_unused]] = ex::stopped_as_optional(
            ex::just(1, 1.0) | ex::let_error([](std::exception_ptr &&) {
                return ex::just(std::string{"error"});
            }));
        // mcs::this_thread::sync_wait(snd);
        // Note: because no std::optional<std::tuple<int, double>,std::string>
    };

    TEST("stopped_as_optional pipeable") = [] {
        ex::sender auto snd = ex::just(1) | ex::stopped_as_optional();
        auto [ret] = mcs::this_thread::sync_wait(snd).value();
        EXPECT(ret.value() == 1);
    };

    return 0;
}