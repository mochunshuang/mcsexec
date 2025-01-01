#include "../test_base_head.hpp"

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
        using CS = decltype(ex::get_completion_signatures(snd, ex::empty_env{}));
        static_assert(
            std::is_same_v<
                CS, cmplsigs::completion_signatures<recv::set_value_t(double),
                                                    recv::set_error_t(std::exception_ptr),
                                                    recv::set_stopped_t()>>);
    };

    TEST("upon_error many input error types") = [] {

    };
    return 0;
}