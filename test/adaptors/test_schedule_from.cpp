#include "../test_base_head.hpp"

#include "../sched/MyScheduler.hpp"
#include <any>
#include <thread>
#include <tuple>

int main()
{
    // Note: schedule_from is not meant to be used in user code
    TEST("schedule_from returns a sender") = [] {
        auto snd = mcs::execution::adapt::schedule_from(MyScheduler{}, ex::just(1));
        static_assert(ex::sender<decltype(snd)>);
    };

    TEST("schedule_from with environment returns a sender") = [] {
        auto snd = mcs::execution::adapt::schedule_from(MyScheduler{}, ex::just(1));
        static_assert(ex::sender_in<decltype(snd), ex::empty_env>);
    };

    TEST("schedule_from simple example") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        auto snd = mcs::execution::adapt::schedule_from(MyScheduler{}, ex::just(1));
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

    TEST("schedule_from works when changing threads") = [] {
        ex::static_thread_pool<2> pool{};

        std::atomic<bool> called{false};
        std::thread::id out_id = std::this_thread::get_id();
        std::thread::id id;
        // lunch some work on the thread pool
        ex::sender auto snd =
            mcs::execution::adapt::schedule_from(pool.get_scheduler(), ex::just()) //
            | ex::then([&] {
                  id = std::this_thread::get_id();
                  called.store(true);
              });
        mcs::this_thread::sync_wait(snd);
        EXPECT(out_id != id);
        EXPECT(called.load() == true);
    };

    TEST("CS") = [] {
        auto snd = mcs::execution::adapt::schedule_from(MyScheduler{}, ex::just(1));
        using CS = ex::snd::completion_signatures_of_t<decltype(snd)>;
        static_assert(ex::tool::eq_set_sigs_v<
                      CS, ex::cmplsigs::completion_signatures<ex::set_value_t(int)>>);
    };

    return 0;
}