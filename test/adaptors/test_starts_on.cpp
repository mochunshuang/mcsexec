#include "../test_base_head.hpp"
#include "../sched/MyScheduler.hpp"

int main()
{
    TEST("starts_on returns a sender") = [] {
        auto snd = ex::starts_on(MyScheduler{}, ex::just(1));
        static_assert(ex::sender<decltype(snd)>);
    };
    TEST("starts_on with environment returns a sender") = [] {
        auto snd = ex::starts_on(MyScheduler{}, ex::just(1));
        static_assert(ex::sender_in<decltype(snd), ex::empty_env>);
    };

    TEST("starts_on simple example") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        auto snd = ex::starts_on(MyScheduler{}, ex::just(1));

        auto op = connect(
            snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});

        EXPECT(not called);
        EXPECT(c == test::channel::NO_CALL);
        start(op);
        EXPECT(called);
        EXPECT(c == test::channel::VALUE_CHANNEL);

        EXPECT(any.has_value());
        auto [ret] = std::any_cast<std::tuple<int>>(any);
        EXPECT(ret == 1);
    };

    TEST("starts_on works when changing threads") = [] {
        ex::static_thread_pool<2> pool{};
        std::atomic<bool> called{false};
        std::thread::id out_id = std::this_thread::get_id();
        std::thread::id id;

        ex::sender auto snd = ex::starts_on(pool.get_scheduler(), ex::just()) //
                              | ex::then([&] { called.store(true); });
        mcs::this_thread::sync_wait(snd);

        EXPECT(out_id != id);
        EXPECT(called.load() == true);
    };

    return 0;
}