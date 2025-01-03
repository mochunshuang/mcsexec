#include "../test_base_head.hpp"
#include "../sched/MyScheduler.hpp"
#include <stdexcept>

int main()
{
    TEST("continues_on returns a sender") = [] {
        auto snd = ex::continues_on(ex::just(1), MyScheduler{});
        static_assert(ex::sender<decltype(snd)>);
    };

    TEST("continues_on with environment returns a sender") = [] {
        auto snd = ex::continues_on(ex::just(1), MyScheduler{});
        static_assert(ex::sender_in<decltype(snd), ex::empty_env>);
    };

    TEST("continues_on simple example") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        auto snd = ex::continues_on(ex::just(1), MyScheduler{});

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

    TEST("continues_on can be piped") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        auto snd = ex::just(1) | ex::continues_on(MyScheduler{});

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

    TEST("continues_on works when changing threads") = [] {
        ex::static_thread_pool<2> pool{};

        std::atomic<bool> called{false};
        std::thread::id out_id = std::this_thread::get_id();
        std::thread::id id;

        ex::sender auto snd = ex::continues_on(ex::just(), pool.get_scheduler()) //
                              | ex::then([&] { called.store(true); });
        mcs::this_thread::sync_wait(snd);
        EXPECT(out_id != id);
        EXPECT(called.load() == true);
    };

    TEST("continues_on can be called with rvalue ref scheduler") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        auto snd = ex::continues_on(ex::just(1), MyScheduler{});
        auto op = ex::connect(
            snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        start(op);
        EXPECT(called);
        EXPECT(c == test::channel::VALUE_CHANNEL);

        EXPECT(any.has_value());
        auto [ret] = std::any_cast<std::tuple<int>>(any);
        EXPECT(ret == 1);
    };

    TEST("continues_on can be called with rvalue ref scheduler") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};
        const MyScheduler sched; // NOLINT

        auto snd = ex::continues_on(ex::just(1), sched);
        auto op = ex::connect(
            snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        start(op);
        EXPECT(called);
        EXPECT(c == test::channel::VALUE_CHANNEL);

        EXPECT(any.has_value());
        auto [ret] = std::any_cast<std::tuple<int>>(any);
        EXPECT(ret == 1);
    };

    TEST("continues_on can be called with ref scheduler") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};
        MyScheduler sched; // NOLINT

        auto snd = ex::continues_on(ex::just(1), sched);
        auto op = ex::connect(
            snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        start(op);
        EXPECT(called);
        EXPECT(c == test::channel::VALUE_CHANNEL);

        EXPECT(any.has_value());
        auto [ret] = std::any_cast<std::tuple<int>>(any);
        EXPECT(ret == 1);
    };

    TEST("continues_on forwards set_error calls") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};
        MyScheduler sched; // NOLINT

        auto snd = ex::continues_on(
            ex::just(1) | ex::then([](int) { throw std::logic_error{"error"}; }), sched);
        auto op = ex::connect(
            snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        start(op);
        EXPECT(called);
        EXPECT(c == test::channel::ERROR_CHANNEL);

        EXPECT(any.has_value());

        try
        {
            auto ret = std::any_cast<std::exception_ptr>(any);
            std::rethrow_exception(ret);
        }
        catch (const std::logic_error &e)
        {
            EXPECT(std::string_view(e.what()) == "error");
        }
    };

    TEST("continues_on forwards set_stopped calls") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};
        MyScheduler sched; // NOLINT

        auto snd = ex::continues_on(ex::just_stopped(), sched);
        auto op = ex::connect(
            snd, test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        start(op);
        EXPECT(called);
        EXPECT(c == test::channel::STOPDE_CHANNEL);
    };

    return 0;
}