#include "../test_base_head.hpp"
#include "../sched/MyScheduler.hpp"

#include <concepts>

#include <chrono>

using my_sender = MyScheduler::MySender;

int main()
{

    TEST("split returns a sender") = [] {
        auto snd = ex::split(ex::just(1));
        using Snd = decltype(snd);
        static_assert(ex::snd::enable_sender<Snd>);
        static_assert(ex::snd::sender<Snd>);
        static_assert(std::same_as<ex::env_of_t<Snd>, ex::empty_env>);
    };
    TEST("split with environment returns a sender") = [] {
        auto snd = ex::split(ex::just(1));
        using Snd = decltype(snd);
        static_assert(ex::snd::sender_in<Snd, ex::empty_env>);
    };
    TEST("split simple example") = [] {
        auto snd = ex::split(ex::just(1));

        bool called_1{false};
        std::any any1;
        test::channel c1{test::channel::NO_CALL};
        bool called_2{false};
        std::any any2;
        test::channel c2{test::channel::NO_CALL};

        auto op1 = ex::connect(
            snd, test::any_receiver{.called = &called_1, .data = &any1, .chanel = &c1});
        auto op2 = ex::connect(
            snd, test::any_receiver{.called = &called_2, .data = &any2, .chanel = &c2});
        start(op1);
        start(op2);

        EXPECT(called_1);
        EXPECT(called_2);
        EXPECT(c1 == test::channel::VALUE_CHANNEL);
        EXPECT(c2 == test::channel::VALUE_CHANNEL);

        auto [ret1] = std::any_cast<std::tuple<int>>(any1);
        auto [ret2] = std::any_cast<std::tuple<int>>(any2);
        EXPECT(ret1 == ret2 && ret1 == 1);
    };

    TEST("split executes predecessor sender once") = [] {
        TEST("when parameters are passed") = [] {
            int counter{};
            auto snd = ex::split(ex::just() //
                                 | ex::then([&] {
                                       counter++;
                                       return counter;
                                   }));
            bool called_1{false};
            std::any any1;
            test::channel c1{test::channel::NO_CALL};
            bool called_2{false};
            std::any any2;
            test::channel c2{test::channel::NO_CALL};

            auto op1 = ex::connect(
                snd,
                test::any_receiver{.called = &called_1, .data = &any1, .chanel = &c1});
            auto op2 = ex::connect(
                snd,
                test::any_receiver{.called = &called_2, .data = &any2, .chanel = &c2});

            EXPECT(counter == 0);
            start(op1);
            EXPECT(counter == 1);
            start(op2);

            // The receiver will ensure that the right value is produced
            EXPECT(counter == 1);

            auto [ret1] = std::any_cast<std::tuple<int>>(any1);
            auto [ret2] = std::any_cast<std::tuple<int>>(any2);
            EXPECT(ret1 == ret2 && ret1 == 1);
        };
        TEST("without parameters") = [] {
            int counter{};
            auto snd = ex::split(ex::just() | ex::then([&] { counter++; }));
            bool called_1{false};
            std::any any1;
            test::channel c1{test::channel::NO_CALL};
            bool called_2{false};
            std::any any2;
            test::channel c2{test::channel::NO_CALL};

            auto op1 = ex::connect(
                snd,
                test::any_receiver{.called = &called_1, .data = &any1, .chanel = &c1});
            auto op2 = ex::connect(
                snd,
                test::any_receiver{.called = &called_2, .data = &any2, .chanel = &c2});

            start(op1);
            start(op2);
            EXPECT(counter == 1);
        };
    };
    TEST("split passes lvalue references") = [] {
        auto split = ex::split(ex::just(42)); // NOLINT
        auto then = split                     //
                    | ex::then([](const int &cval) {
                          int &val = const_cast<int &>(cval); // NOLINT
                          const int prev_val = val;           // NOLINT
                          val /= 2;
                          return prev_val;
                      });
        auto [ret] = mcs::this_thread::sync_wait(then).value();
        EXPECT(ret == 42);
        {
            auto [ret] = mcs::this_thread::sync_wait(split).value();
            EXPECT(ret == 21);
        }
    };
    TEST("split forwards errors") = [] {
        TEST("of exception_ptr type") = [] {
            auto snd = ex::split(ex::just_error(std::exception_ptr{}));

            bool called{false};
            std::any any;
            test::channel c{test::channel::NO_CALL};
            auto op = connect(
                std::move(snd),
                test::any_receiver{.called = &called, .data = &any, .chanel = &c});

            start(op);
            EXPECT(called);
            EXPECT(c == test::channel::ERROR_CHANNEL);
            EXPECT(any.has_value());

            auto ret [[maybe_unused]] = std::any_cast<std::exception_ptr>(any);
        };
        TEST("of any type") = [] {
            auto snd = ex::split(ex::just_error(1));

            bool called{false};
            std::any any;
            test::channel c{test::channel::NO_CALL};
            auto op = connect(
                std::move(snd),
                test::any_receiver{.called = &called, .data = &any, .chanel = &c});

            start(op);
            EXPECT(called);
            EXPECT(c == test::channel::ERROR_CHANNEL);
            EXPECT(any.has_value());

            auto ret [[maybe_unused]] = std::any_cast<int>(any);
        };
    };

    TEST("split forwards stop signal") = [] {
        auto snd = ex::split(ex::just_stopped());

        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};
        auto op =
            connect(std::move(snd),
                    test::any_receiver{.called = &called, .data = &any, .chanel = &c});

        start(op);
        EXPECT(called);
        EXPECT(c == test::channel::STOPDE_CHANNEL);
    };

    TEST("split forwards external stop signal (1)") = [] {
        // ex::inplace_stop_source ssource;
        // bool called = false;
        // int counter{};
        // auto split = ex::split(ex::just() | ex::then([&] { called = true; }));
        // TODO(mcs): 待实现
        // auto sndr = ex::write(ex::upon_stopped(std::move(split),
        //                                        [&] {
        //                                            ++counter;
        //                                            return 42;
        //                                        }),
        //                       ex::prop{ex::get_stop_token, ssource.get_token()});
    };

    TEST("split forwards results from a different thread") = [] {
        ex::static_thread_pool<1> pool{};
        auto split = ex::schedule(pool.get_scheduler()) //
                     | ex::then([] {
                           using namespace std::chrono_literals;
                           std::this_thread::sleep_for(1ms);
                           return 2;
                       }) //
                     | ex::split();

        auto [val] = mcs::this_thread::sync_wait(split).value();
        EXPECT(val == 2);
    };

    TEST("split can be an rvalue") = [] {
        auto [val] = mcs::this_thread::sync_wait(ex::just(2) | ex::split() |
                                                 ex::then([](int v) { return v; }))
                         .value();
        EXPECT(val == 2);
    };

    TEST("split into then") = [] {
        TEST("split with move only input sender of temporary") = [] {
            auto pre_snd = ex::split(ex::just(move_only_type{0}));
            auto snd =
                std::move(pre_snd) | ex::then([](const move_only_type &) { return; });
            mcs::this_thread::sync_wait(std::move(snd));
        };
        TEST("split with move only input sender by moving in") = [] {
            auto snd0 = ex::just(move_only_type{});
            auto snd =
                ex::split(std::move(snd0)) | ex::then([](const move_only_type &) {});
            mcs::this_thread::sync_wait(std::move(snd));
        };
        TEST("split with copyable rvalue input sender") = [] {
            auto snd = ex::split(ex::just(copy_and_movable_type{0})) |
                       ex::then([](const copy_and_movable_type &) {});
            mcs::this_thread::sync_wait(std::move(snd));
        };
        TEST("split with copyable lvalue input sender") = [] {
            auto snd0 = ex::just(copy_and_movable_type{0});
            auto snd = ex::split(snd0) | ex::then([](const copy_and_movable_type &) {});
            mcs::this_thread::sync_wait(std::move(snd));
        };
        TEST("lvalue split move only sender") = [] {
            auto multishot [[maybe_unused]] = ex::split(ex::just(move_only_type{0}));
            // Note: basic-sender<Tag, decay_t<Data>, decay_t<Child>...>
            // Note: T& -> T fail for move_only_type no copy constructor
            // Note: then can't build basic-sender<Tag, decay_t<Data>,
            // decay_t<Child>...> Note: below compile fail auto snd = multishot |
            // ex::then([](const move_only_type &) {});
            // mcs::this_thread::sync_wait(snd);
        };
        TEST("lvalue split copyable sender") = [] {
            auto multishot = ex::split(ex::just(copy_and_movable_type{0}));
            auto snd = multishot | ex::then([](const copy_and_movable_type &) {});

            mcs::this_thread::sync_wait(snd);

            using Sndr = decltype(multishot);
            static_assert(
                ex::snd::sender_in_of<Sndr, ex::empty_env, copy_and_movable_type>);
            ;
            static_assert(ex::snd::sender_of<Sndr, copy_and_movable_type>);

            using Base = copy_and_movable_type;
            using CS [[maybe_unused]] =
                ex::cmplsigs::value_types_of_t<Sndr, ex::empty_env,
                                               mcs::execution::snd::value_signature,
                                               std::type_identity_t>;

            static_assert(ex::snd::sender_of<decltype(multishot), copy_and_movable_type>);
            static_assert(
                ex::sender_of<decltype(multishot), const copy_and_movable_type>);
            // Note: NORMALIZE 是原理
            static_assert(std::same_as<ex::set_value_t(Base),
                                       ex::set_value_t(const copy_and_movable_type)>);
            // Note: NORMALIZE 是原理
            static_assert(not std::same_as<Base, const copy_and_movable_type>);
            static_assert(ex::sender_of<decltype(multishot), copy_and_movable_type &&>);

            static_assert(
                not ex::sender_of<decltype(multishot), copy_and_movable_type &>);
            static_assert(
                not ex::sender_of<decltype(multishot), const copy_and_movable_type &>);
            // NOTE: NORMALIZE 是原理。 && 当看不见即可
            static_assert(
                ex::sender_of<decltype(multishot), const copy_and_movable_type &&>);
            using T0 = const copy_and_movable_type &&;
            using NORMALIZE_T = ex::snd::general::__detail::remove_rvalue_reference_t<T0>;
            static_assert(std::is_same_v<NORMALIZE_T, const copy_and_movable_type>);
        };
    };

    TEST("split move-only and copyable senders") = [] {
        {
            using TestType = move_only_type;
            int called = 0;
            auto multishot [[maybe_unused]] = ex::just(TestType(10)) // NOLINT
                                              | ex::then([&](TestType obj) {
                                                    ++called;
                                                    return TestType(obj.val + 1);
                                                }) |
                                              ex::split();
            // Note: 失败
            //  auto wa = ex::when_all(
            //      ex::then(multishot, [](const TestType &obj) { return obj.val; }),
            //      ex::then(multishot, [](const TestType &obj) { return obj.val * 2;
            //      }), ex::then(multishot, [](const TestType &obj) { return obj.val *
            //      3; }));

            // auto [v1, v2, v3] = mcs::this_thread::sync_wait(std::move(wa)).value();

            // EXPECT(called == 1);
            // EXPECT(v1 == 11);
            // EXPECT(v2 == 22);
            // EXPECT(v3 == 33);
        }
        {
            using TestType = copy_and_movable_type;
            int called = 0;
            auto multishot = ex::just(TestType(10)) // NOLINT
                             | ex::then([&](TestType obj) {
                                   ++called;
                                   return TestType(obj.val + 1);
                               }) |
                             ex::split();
            auto wa = ex::when_all(
                ex::then(multishot, [](const TestType &obj) { return obj.val; }),
                ex::then(multishot, [](const TestType &obj) { return obj.val * 2; }),
                ex::then(multishot, [](const TestType &obj) { return obj.val * 3; }));

            auto [v1, v2, v3] = mcs::this_thread::sync_wait(std::move(wa)).value();

            EXPECT(called == 1);
            EXPECT(v1 == 11);
            EXPECT(v2 == 22);
            EXPECT(v3 == 33);
        }
        {
            // Note: 只能等待在处理了。可以是指针等。共享即可
            using TestType = move_only_type; // NOLINTNEXTLINE
            auto [pre_data] = mcs::this_thread::sync_wait(ex::just(TestType(10))).value();

            int called = 0;
            auto multishot = ex::just(pre_data.val) // NOLINT
                             | ex::then([&](int val) {
                                   ++called;
                                   return TestType(val + 1);
                               }) |
                             ex::split();
            auto wa = ex::when_all(
                ex::then(multishot, [](const TestType &obj) { return obj.val; }),
                ex::then(multishot, [](const TestType &obj) { return obj.val * 2; }),
                ex::then(multishot, [](const TestType &obj) { return obj.val * 3; }));

            auto [v1, v2, v3] = mcs::this_thread::sync_wait(std::move(wa)).value();

            EXPECT(called == 1);
            EXPECT(v1 == 11);
            EXPECT(v2 == 22);
            EXPECT(v3 == 33);
        }
    };

    TEST("split into when_all") = [] {
        int counter{};
        auto snd = ex::split(ex::just() //
                             | ex::then([&] {
                                   counter++;
                                   return counter;
                               }));
        auto wa = ex::when_all(snd | ex::then([](auto) { return 10; }),  // NOLINT
                               snd | ex::then([](auto) { return 20; })); // NOLINT
        EXPECT(counter == 0);
        auto [v1, v2] = mcs::this_thread::sync_wait(std::move(wa)).value();
        EXPECT(counter == 1);
        EXPECT(v1 == 10);
        EXPECT(v2 == 20);
    };

    TEST("split can nest") = [] {
        auto split_1 = ex::just(42) | ex::split(); // NOLINT
        auto split_2 = split_1 | ex::split();

        auto [v1] = mcs::this_thread::sync_wait(split_1 //
                                                | ex::then([](const int &cv) {
                                                      int &v =
                                                          const_cast<int &>(cv); // NOLINT
                                                      return v = 1;
                                                  }))
                        .value();

        auto [v2] = mcs::this_thread::sync_wait(split_2 //
                                                | ex::then([](const int &cv) {
                                                      int &v =
                                                          const_cast<int &>(cv); // NOLINT
                                                      return v = 2;
                                                  }))
                        .value();

        auto [v3] = mcs::this_thread::sync_wait(split_1).value();

        EXPECT(v1 == 1);
        EXPECT(v2 == 2);
        EXPECT(v3 == 1);
    };

    TEST("split accepts a custom sender") = [] {
        auto snd1 = my_sender();
        auto snd2 [[maybe_unused]] = ex::split(snd1);
    };
    return 0;
}