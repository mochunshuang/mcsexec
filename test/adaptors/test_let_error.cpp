
#include "../test_base_head.hpp"
#include <algorithm>
#include <cassert>
#include <exception>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>

template <bool error_code>
struct two_errror_sender
{
    template <typename Revr>
    struct state
    {
        using operation_state_concept = ::mcs::execution::operation_state_t;

        Revr rcvr; // NOLINT

        explicit state(Revr &&r) noexcept : rcvr{std::move(r)} {};

        ~state() noexcept = default;
        state(state &&) = delete;
        state(const state &) = delete;
        state &operator=(state &&) = delete;
        state &operator=(const state &) = delete;

        void start() & noexcept
        {
            if constexpr (error_code)
            {
                ::mcs::execution::recv::set_error(
                    std::move(rcvr), std::make_error_code(std::io_errc::stream));
            }
            else
            {
                ::mcs::execution::recv::set_error(
                    std::move(rcvr), std::make_exception_ptr(
                                         std::logic_error{"exception_ptr description"}));
            }
        }
    };
    using sender_concept = ::mcs::execution::sender_t;
    using completion_signatures = ::mcs::execution::completion_signatures<
        ::mcs::execution::set_value_t(), ::mcs::execution::set_error_t(std::error_code),
        ::mcs::execution::set_error_t(std::exception_ptr)>;

    using indices_for = std::index_sequence_for<>;

    template <::mcs::execution::receiver Recv>
    constexpr auto connect(Recv recv) noexcept -> state<Recv>
    {
        return state<Recv>{std::move(recv)};
    }
};
static_assert(::mcs::execution::sender<two_errror_sender<true>>);

int main()
{
    using namespace mcs::execution;
    {
        auto sndr =
            ex::let_error(ex::just(), [](std::exception_ptr) { return ex::just(); });
        TEST("let_error returns a sender") = [&] {
            static_assert(ex::sender<decltype(sndr)>);
        };
        TEST("let_error with environment returns a sender") = [&] {
            static_assert(ex::sender_in<decltype(sndr), ex::empty_env>);
        };
    }
    TEST("let_error simple example") = [] {
        bool called{false};
        bool fun_called{false};
        std::any data;
        auto snd = ex::let_error(ex::just_error(std::exception_ptr{}),
                                 [&](const std::exception_ptr &) {
                                     fun_called = true;
                                     return ex::just();
                                 });
        test::channel chanel{test::channel::NO_CALL};
        auto op = connect(
            std::move(snd),
            test::any_receiver{.called = &called, .data = &data, .chanel = &chanel});
        EXPECT(not fun_called && not called && chanel == test::channel::NO_CALL);
        start(op);
        EXPECT(fun_called && called && chanel == test::channel::VALUE_CHANNEL);
    };

    TEST("let_error simple example reference") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};
        auto snd = ex::let_error(ex::split(ex::just_error(std::exception_ptr{})),
                                 [&](const std::exception_ptr &) {
                                     return ex::just(404); // NOLINT
                                 });

        auto op = ex::connect(
            std::move(snd),
            test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        EXPECT(not called);
        EXPECT(c == test::channel::NO_CALL);
        start(op);
        EXPECT(called);
        EXPECT(c == test::channel::VALUE_CHANNEL);

        auto [ret] = std::any_cast<std::tuple<int>>(any);
        EXPECT(ret == 404);
    };

    TEST("let_error can be piped") = [] {
        ex::sender auto snd [[maybe_unused]] =
            ex::just() | ex::let_error([](std::exception_ptr &&) { return ex::just(); });
    };

    TEST("let_error CS") = [] {
        ex::sender auto snd [[maybe_unused]] =
            ex::just() | ex::let_error([](std::exception_ptr &&) { return ex::just(); });
        using T = decltype(snd);
        using CS = ex::snd::completion_signatures_of_t<T>;
        // NOTE: ex::let_error的 fun 不会被调用
        static_assert(std::is_same_v<CS, mcs::execution::cmplsigs::completion_signatures<
                                             mcs::execution::recv::set_value_t()>>);
        {
            ex::sender auto snd [[maybe_unused]] =
                ex::just() | ex::let_error([](std::exception_ptr &&) {
                    return ex::just(1, 1.0, 1.0F);
                });
            using T = decltype(snd);
            using CS = ex::snd::completion_signatures_of_t<T>;
            static_assert(
                tool::eq_set_sigs_v<CS, mcs::execution::cmplsigs::completion_signatures<
                                            recv::set_value_t()>>);
        }
    };
    TEST("let_error CS 1") = [] {
        ex::sender auto snd [[maybe_unused]] =
            ex::just() | ex::then([]() {}) |
            ex::let_error([](std::exception_ptr &) { return ex::just(); });
        using T = decltype(snd);
        using CS = ex::snd::completion_signatures_of_t<T>;
        static_assert(
            tool::is_same_v<
                CS, mcs::execution::cmplsigs::completion_signatures<
                        recv::set_value_t(), recv::set_error_t(std::exception_ptr)>>);
    };
    TEST("let_error CS 2") = [] {
        ex::sender auto snd [[maybe_unused]] =
            ex::just() | ex::then([]() noexcept(true) {}) |
            ex::let_error([](std::exception_ptr &&) { return ex::just(); });
        using T = decltype(snd);
        using CS = ex::snd::completion_signatures_of_t<T>;
        static_assert(tool::is_same_v<CS, mcs::execution::cmplsigs::completion_signatures<
                                              recv::set_value_t()>>);
    };
    TEST("let_error CS 3") = [] {
        ex::sender auto snd [[maybe_unused]] =
            ex::just() | ex::then([]() noexcept(false) {}) |
            ex::let_error([](std::exception_ptr &) { return ex::just(1.0); });
        using T = decltype(snd);
        using CS = ex::snd::completion_signatures_of_t<T>;
        // NOTE: ex::then() may_throw or no_throw
        static_assert(
            tool::is_same_v<CS, mcs::execution::cmplsigs::completion_signatures<
                                    recv::set_value_t(), recv::set_value_t(double),
                                    recv::set_error_t(std::exception_ptr)>>);
    };
    TEST("let_error CS 4") = [] {
        ex::sender auto snd [[maybe_unused]] =
            ex::just() | ex::then([]() noexcept(false) {}) |
            ex::let_error(
                [](std::exception_ptr &) noexcept(true) { return ex::just(1.0); });
        using T = decltype(snd);
        using CS = ex::snd::completion_signatures_of_t<T>;
        // NOTE: ex::then() may_throw or no_throw
        // NOTE: ex::let_error with fun no_throw
        static_assert(
            tool::is_same_v<CS, mcs::execution::cmplsigs::completion_signatures<
                                    recv::set_value_t(), recv::set_value_t(double)>>);
    };
    TEST("let_error simple example ") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        ex::sender auto snd = ex::just() //
                              | ex::then([]() -> std::string {
                                    throw std::logic_error{"error description"};
                                    return {};
                                }) //
                              | ex::let_error([](std::exception_ptr eptr) {
                                    try
                                    {
                                        std::rethrow_exception(std::move(eptr));
                                    }
                                    catch (const std::exception &e)
                                    {
                                        return ex::just(std::string{e.what()});
                                    }
                                });

        auto op = ex::connect(
            std::move(snd),
            test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        EXPECT(not called);
        EXPECT(c == test::channel::NO_CALL);
        start(op);
        EXPECT(called);
        EXPECT(c == test::channel::VALUE_CHANNEL);

        auto [ret] = std::any_cast<std::tuple<std::string>>(any);
        EXPECT(ret == "error description");
    };

    TEST("let_error returning void can be waited on") = [] {
        ex::sender auto snd =
            ex::just_error(std::exception_ptr{}) |
            ex::let_error([](const std::exception_ptr &) { return ex::just(); });
        mcs::this_thread::sync_wait(std::move(snd));
        {

            ex::sender auto snd =
                ex::just_error(std::exception_ptr{}) |
                ex::let_error([](const std::exception_ptr &) { return ex::just(1); });
            auto [ret] = mcs::this_thread::sync_wait(std::move(snd)).value();
            EXPECT(ret == 1);
        }
        {
            // Note: the input sender has exactly one value completion signature
            // set_value_t(int)
            ex::sender auto snd =
                ex::just()                    //
                | ex::then([] { return 13; }) //
                | ex::let_error([&](std::exception_ptr) { return ex::just(0); });
            auto [ret] = mcs::this_thread::sync_wait(std::move(snd)).value();
            EXPECT(ret == 13);
            static_assert(std::is_same_v<decltype(ret), int>);
        }

        //  如果不一样
        {
            // Note: sync_wait mandates that the input sender has exactly one value
            // completion signature.
            // Note: 只能是 一个 值完成签名，因此。肯定是解决不了的。 string 和 int
            // 冲突
            {
                // ex::sender auto snd =
                //     ex::just()                                   //
                //     | ex::then([] { return std::string("13"); }) //
                //     | ex::let_error([&](std::exception_ptr) { return ex::just(0);
                //     });
                // auto [ret] = mcs::this_thread::sync_wait(std::move(snd)).value();
                // EXPECT(ret == 13);
                // static_assert(std::is_same_v<decltype(ret), int>);
            }
        }
    };

    TEST("let_error can be used to transform errors") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        ex::sender auto snd =
            ex::just_error(1) //
            | ex::let_error(
                  [](int error_code) -> decltype(ex::just_error(std::exception_ptr{})) {
                      char buf[20];
                      std::snprintf(buf, 20, "%d", error_code);
                      throw std::logic_error(buf);
                  });

        auto op = ex::connect(
            std::move(snd),
            test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        EXPECT(not called);
        EXPECT(c == test::channel::NO_CALL);
        start(op);
        EXPECT(called);
        EXPECT(c == test::channel::ERROR_CHANNEL);
        EXPECT(any.has_value());

        // 取出 throw std::logic_error(buf); 的信息，通过重新抛异常
        try
        {
            auto ret = std::any_cast<std::exception_ptr>(any);
            std::rethrow_exception(ret);
        }
        catch (const std::logic_error &e)
        {
            EXPECT(std::string_view(e.what()) == "1");
        }
    };
    TEST("let_error can throw, and yield a different error type") = [] {
        auto error_code = 404;
        {
            bool called{false};
            std::any any;
            test::channel c{test::channel::NO_CALL};
            auto snd = ex::just_error(error_code) //
                       | ex::let_error([](int x) {
                             if (x % 2 == 0)
                                 throw std::logic_error{"err"};
                             return ex::just_error(x);
                         });
            auto op = ex::connect(
                std::move(snd),
                test::any_receiver{.called = &called, .data = &any, .chanel = &c});
            start(op);
            EXPECT(called);
            EXPECT(c == test::channel::ERROR_CHANNEL);
            EXPECT(any.has_value());
            auto ret = std::any_cast<std::exception_ptr>(any);
        }
        error_code = 501;
        {
            bool called{false};
            std::any any;
            test::channel c{test::channel::NO_CALL};
            auto snd = ex::just_error(error_code) //
                       | ex::let_error([](int x) {
                             if (x % 2 == 0)
                                 throw std::logic_error{"err"};
                             return ex::just_error(x);
                         });
            auto op = ex::connect(
                std::move(snd),
                test::any_receiver{.called = &called, .data = &any, .chanel = &c});
            start(op);
            EXPECT(called);
            EXPECT(c == test::channel::ERROR_CHANNEL);
            EXPECT(any.has_value());
            auto ret = std::any_cast<int>(any);
            EXPECT(ret == 501);
        }
    };
    TEST("let_error can be used with just_stopped") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        bool fun_called{false};

        ex::sender auto snd = ex::just_stopped() //
                              | ex::let_error([&](std::exception_ptr) {
                                    fun_called = true;
                                    return ex::just(17);
                                });
        auto op =
            connect(std::move(snd),
                    test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        start(op);
        EXPECT(not fun_called);
        EXPECT(c == test::channel::STOPDE_CHANNEL);
    };
    TEST("let_error function is not called on regular flow") = [] {
        bool called{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};

        bool fun_called{false};

        ex::sender auto snd = ex::just()                    //
                              | ex::then([] { return 13; }) //
                              | ex::let_error([&](std::exception_ptr) {
                                    fun_called = true;
                                    return ex::just(0);
                                });
        auto op =
            connect(std::move(snd),
                    test::any_receiver{.called = &called, .data = &any, .chanel = &c});
        start(op);
        EXPECT(not fun_called);
        EXPECT(c == test::channel::VALUE_CHANNEL);
    };

    TEST("with sync_wait") = [] {
        ex::sender auto snd = ex::just() //
                              | ex::then([]() -> std::string {
                                    throw std::logic_error{"error description"};
                                    return {};
                                }) //
                              | ex::let_error([](std::exception_ptr eptr) {
                                    try
                                    {
                                        std::rethrow_exception(std::move(eptr));
                                    }
                                    catch (const std::exception &e)
                                    {
                                        return ex::just(std::string{e.what()});
                                    }
                                });
        auto [ret] = mcs::this_thread::sync_wait(snd).value();
        EXPECT(ret == "error description");
    };

    TEST("with sync_wait 2 ") = [] {
        {
            constexpr auto let_error_noexcept = false; // NOLINT
            ex::sender auto snd =
                ex::just() //
                | ex::then([]() -> std::string {
                      throw std::logic_error{"error description"};
                      return {};
                  }) //
                | ex::let_error([](std::exception_ptr eptr) noexcept(let_error_noexcept) {
                      try
                      {
                          std::rethrow_exception(std::move(eptr));
                      }
                      catch (const std::exception &e)
                      {
                          return ex::just(std::string{e.what()});
                      }
                  });
            using S = ex::snd::completion_signatures_of_t<decltype(snd)>;

            if constexpr (let_error_noexcept)
            {
                bool v = std::is_same_v<
                    S, ex::completion_signatures<ex::set_value_t(std::string)>>;
                assert(v);
            }
            else
            {
                bool v = std::is_same_v<
                    S, ex::completion_signatures<ex::set_value_t(std::string),
                                                 ex::set_error_t(std::exception_ptr)>>;
                assert(v);
            }
        }
        {
            constexpr auto let_error_noexcept = false; // NOLINT
#define test_error_code 1
            ex::sender auto snd =
#if (test_error_code)
                ex::just_error(std::make_error_code(std::io_errc::stream))
#else
                ex::just_error(std::make_exception_ptr(
                    std::logic_error{"exception_ptr description"}))
#endif
                | ex::then([]() -> std::string {
                      throw std::logic_error{"error description"};
                      return {};
                  }) |
                ex::upon_error([](auto eptr) noexcept(let_error_noexcept) {
                    if constexpr (std::is_same_v<decltype(eptr), std::error_code>)
                    {
                        return std::string{"error_code"};
                    }
                    else
                    {
                        try
                        {
                            std::rethrow_exception(std::move(eptr));
                        }
                        catch (const std::exception &e)
                        {
                            return std::string{e.what()};
                        }
                    }
                });
            using S = ex::snd::completion_signatures_of_t<decltype(snd)>;
            static_assert(
                std::is_same_v<
                    S, ex::completion_signatures<ex::set_value_t(std::string),
                                                 ex::set_error_t(std::exception_ptr)>>);
            auto [ret] = mcs::this_thread::sync_wait(snd).value();
#if (test_error_code)
            EXPECT(ret == "error_code");
#else
            EXPECT(ret == "exception_ptr description");
#endif
        }

        // NOTE: 另一种表达。  结论：必须一次性 处理前面 所有错误类型
        // 处理。所有异常的请求。我编译期限制了，是这样的
        {
            constexpr auto let_error_noexcept = true; // NOLINT

#define test_error_code 1

            auto snd = ex::just() | ex::let_value([] {
#if (test_error_code)
                           return two_errror_sender<true>();
#else
                           return two_errror_sender<false>();

#endif
                       }) |
                       ex::then([]() -> std::string { return {}; }) |
                       ex::upon_error([](auto eptr) noexcept(let_error_noexcept) {
                           if constexpr (std::is_same_v<decltype(eptr), std::error_code>)
                           {
                               return std::string{"error_code"};
                           }
                           else
                           {
                               try
                               {
                                   std::rethrow_exception(std::move(eptr));
                               }
                               catch (const std::exception &e)
                               {
                                   return std::string{e.what()};
                               }
                           }
                       });
            using S = ex::snd::completion_signatures_of_t<decltype(snd)>;
            static_assert(
                std::is_same_v<S,
                               ex::completion_signatures<ex::set_value_t(std::string)>>);
            auto [ret] = mcs::this_thread::sync_wait(snd).value();
#if (test_error_code)
            EXPECT(ret == "error_code");
#else
            EXPECT(ret == "exception_ptr description");
#endif
        }
    };

    return 0;
}