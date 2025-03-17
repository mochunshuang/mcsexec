#include "../test_base_head.hpp"
#include <iostream>
#include <stdexcept>
#include <string_view>

int main()
{
    using namespace mcs::execution; // NOLINT

    TEST("upon_error returns a sender") = [] {
        auto snd = ex::upon_error(ex::just_error(std::exception_ptr{}),
                                  [](const std::exception_ptr &) {});
        static_assert(ex::sender<decltype(snd)>);
    };
    TEST("upon_error with environment returns a sender") = [] {
        auto snd = ex::upon_error(ex::just_error(std::exception_ptr{}),
                                  [](const std::exception_ptr &) {});
        static_assert(ex::sender_in<decltype(snd), empty_env>);
    };
    TEST("upon_error simple example") = [] {
        bool called{false};
        bool called_fun{false};
        std::any any;
        test::channel c{test::channel::NO_CALL};
        auto snd = ex::upon_error(ex::just_error(std::exception_ptr{}),
                                  [&](const std::exception_ptr &) {
                                      called_fun = true;
                                      return 0;
                                  });
        auto op =
            connect(std::move(snd),
                    test::any_receiver{.called = &called, .data = &any, .chanel = &c});

        EXPECT(not called);
        EXPECT(not called_fun);
        EXPECT(c == test::channel::NO_CALL);

        start(op);

        EXPECT(called);
        EXPECT(called_fun);
        EXPECT(c == test::channel::VALUE_CHANNEL);
    };

    TEST("upon_error with no-error input sender") = [] {
        auto snd = ex::upon_error(ex::just(), [](auto e) -> double { return 0.0; });
        static_assert(ex::sender<decltype(snd)>);
        using S = decltype(snd);
        static_assert(ex::sender<S>);
        // Note: compile error
        // Note: []() -> double { return 0.0; } cant not handle set_error_t(e)
        // using CS = decltype(ex::get_completion_signatures(snd, ex::empty_env{}));
        // static_assert(
        //     tool::eq_set_sigs_v<
        //         CS, cmplsigs::completion_signatures<recv::set_value_t(double),
        //                                             recv::set_error_t(std::exception_ptr),
        //                                             recv::set_value_t()>>);
        {
            auto snd = ex::upon_error(
                ex::just(), [](const std::exception_ptr &e) -> double { return 0.0; });
            static_assert(ex::sender<decltype(snd)>);
            using S = decltype(snd);
            static_assert(ex::sender<S>);
            // Note: OK; std::exception_ptr &e cat match pre_sndr sendr e
            using CS = ex::snd::completion_signatures_of_t<S>;
            // NOTE: ex::just() 不会抛异常，因此 upon_error 的 fun
            // 不会被调用，编译期就确定了
            static_assert(
                std::is_same_v<CS, cmplsigs::completion_signatures<recv::set_value_t()>>);
            mcs::this_thread::sync_wait(snd);
        }
        {
            auto snd =
                ex::upon_error(ex::just() | ex::then([] {}),
                               [](const std::exception_ptr &e) -> double { return 0.0; });
            using CS = ex::snd::completion_signatures_of_t<decltype(snd)>;
            static_assert(ex::tool::eq_set_sigs_v<
                          CS, cmplsigs::completion_signatures<
                                  recv::set_value_t(), recv::set_value_t(double),
                                  recv::set_error_t(std::exception_ptr)>>);
            // mcs::this_thread::sync_wait(snd); //不是单一返回值编译器错误
        }
        {
            auto snd =
                ex::upon_error(ex::just() | ex::then([] {}),
                               [](const std::exception_ptr &e) -> double { return 0.0; });
            // mcs::this_thread::sync_wait(snd); //不是单一返回值编译器错误
            auto sndr = snd | ex::then([](auto &&...v) {
                            if constexpr (sizeof...(v) == 1)
                            {
                                UNEXPECT("cant not");
                            }
                            else
                            {
                                std::cout << "call from pre then....\n";
                            }
                        });
            mcs::this_thread::sync_wait(sndr);
        }

        {
            auto snd =
                ex::just() | ex::then([] { throw std::logic_error{"error"}; }) //
                | ex::upon_error(

                      [](std::exception_ptr e) -> double {
                          std::cout << "pre call from exception....\n";
                          try
                          {
                              std::rethrow_exception(e); // 重新抛出异常
                          }
                          catch (const std::logic_error &ex) // 专门捕获 std::logic_error
                          {
                              std::cout << "catch  exception....\n";
                              assert(std::string(ex.what()) == "error");
                          }
                          catch (...)
                          {
                              return 0;
                          }
                          return 1.1; // NOLINT
                      });

            using CS = ex::snd::completion_signatures_of_t<decltype(snd)>;
            static_assert(
                std::is_same_v<CS, ex::completion_signatures<
                                       ex::set_value_t(), ex::set_value_t(double),
                                       ex::set_error_t(std::exception_ptr)>>);

            // NOTE: auto 处理，多个  set_value_t
            auto sndr = snd | ex::then([](auto &&...v) {
                            if constexpr (sizeof...(v) == 1)
                            {
                                std::cout << "then: call from exception....\n";
                                EXPECT(std::tuple<double>{1.1} == std::tuple{v...});
                                ((std::cout << v), ...);
                                std::cout << '\n';
                            }
                            else
                            {
                                UNEXPECT("cant not");
                            }
                        });
            // Note: 异常已经捕获，不需要 try-catch 了
            mcs::this_thread::sync_wait(sndr);
        }
    };

    TEST("upon_error returns a sender with logic_error") = [] {
        auto snd0 = ex::just_error(std::logic_error{"my error"});
        // NOTE: 以下编译期报错，因为  logic_error 和 exception_ptr 不匹配
#if false
        auto snd = ex::upon_error(snd0, [](const std::exception_ptr &e) {
            try
            {
                std::rethrow_exception(e);
            }
            catch (const std::logic_error &ex)
            {
                std::cout << "catch  my logic_error....\n";
                EXPECT(std::string_view("my error") == std::string_view(ex.what()));
            }
            catch (...)
            {
                UNEXPECT("error");
            }
        });
#endif
        // 编译期错误
        // auto snd = snd0 | ex::upon_error([](const std::exception_ptr &e) {});
    };

    return 0;
}