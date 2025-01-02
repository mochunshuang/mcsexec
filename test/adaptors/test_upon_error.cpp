#include "../test_base_head.hpp"
#include <stdexcept>

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
        EXPECT(c == test::channel::ERROR_CHANNEL);
    };

    TEST("upon_error with no-error input sender") = [] {
        auto snd = ex::upon_error(ex::just(), []() -> double { return 0.0; });
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
            using CS = decltype(ex::get_completion_signatures(snd, ex::empty_env{}));
            // TODO(mcs): just() 不空有异常因此，不可能 添加 double 的 SCS
            static_assert(
                std::is_same_v<
                    CS, cmplsigs::completion_signatures<
                            recv::set_value_t(), recv::set_error_t(std::exception_ptr)>>);
            mcs::this_thread::sync_wait(snd);
        }
        {
            auto snd =
                ex::upon_error(ex::just() | ex::then([] {}),
                               [](const std::exception_ptr &e) -> double { return 0.0; });
            using CS = decltype(ex::get_completion_signatures(snd, ex::empty_env{}));
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

                      [](std::exception_ptr &&e) -> double {
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
                          // upon_error => set_error_t(double)
                          return 1.1; // 会继续抛异常，
                      });

            // mcs::this_thread::sync_wait(snd); //不是单一返回值编译器错误

            auto sndr = snd | ex::then([](auto &&...v) {
                            if constexpr (sizeof...(v) == 1)
                            {
                                std::cout << "call from exception....\n";
                                EXPECT(std::tuple<double>{1.0} == std::tuple{v...});
                            }
                            else
                            {
                                UNEXPECT("cant not");
                            }
                        });
            using CS = decltype(ex::get_completion_signatures(sndr, ex::empty_env{}));

            using T = decltype(std::make_exception_ptr(1));
            try
            {
                mcs::this_thread::sync_wait(sndr);
                int a = 1;
            }
            catch (double ex)
            {
                std::cout << "sync_wait: call from exception double: " << ex << "  \n";
            }
        }
    };
    return 0;
}