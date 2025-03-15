#include "../test_base_head.hpp"
#include <string>

// NOLINTBEGIN
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

    TEST("stopped_as_optional shall not work with just(), as no std::optional<void>") =
        [] {
            // ex::sender auto snd [[maybe_unused]] = ex::stopped_as_optional(ex::just());
            // mcs::this_thread::sync_wait(snd).value();
        };

    TEST("stopped_as_optional shall work with single-value just senders") = [] {
        ex::sender auto snd = ex::stopped_as_optional(ex::just(1, 1.0));
        auto [ret] = mcs::this_thread::sync_wait(snd).value();
        auto [a, b] = ret.value();
        EXPECT(a == 1);
        EXPECT(b == 1.0);
    };

    TEST("stopped_as_optional shall  work with single-value senders") = [] {
        ex::sender auto snd [[maybe_unused]] = ex::stopped_as_optional(
            ex::just(1, 1.0) | ex::let_error([](std::exception_ptr &&) {
                return ex::just(std::string{"error"});
            }));
        // mcs::this_thread::sync_wait(snd);
        using S = decltype(snd);
        using CS = ex::snd::completion_signatures_of_t<S>;
        static_assert(
            std::is_same_v<CS, ex::cmplsigs::completion_signatures<ex::set_value_t(
                                   std::optional<std::tuple<int, double>>)>>);
        auto [ret] = mcs::this_thread::sync_wait(snd).value();
        auto [a, b] = ret.value();
        EXPECT(a == 1);
        EXPECT(b == 1.0);
        // NOTE: ex::let_error pre sndr no std::exception_ptr
    };

    TEST("stopped_as_optional pipeable") = [] {
        ex::sender auto snd = ex::just(1) | ex::stopped_as_optional();
        auto [ret] = mcs::this_thread::sync_wait(snd).value();
        EXPECT(ret.value() == 1);
    };

    TEST("compile error") = [] {
        // std::exception_ptr &&  改成 std::exception_ptr & 才行，内部引用过程结果
        // using T = std::string;
        // ex::sender auto snd [[maybe_unused]] = ex::stopped_as_optional(
        //     ex::just(1, 1.0) | ex::then([](int, double) { return T("msg"); }) |
        //     ex::let_error([](std::exception_ptr &&) { return ex::just(T("error")); }));

        // using S = decltype(snd);
        // static_assert(ex::single_sender<S, ex::empty_env>);
        // using CS = ex::snd::completion_signatures_of_t<S>;
        // // auto ret = mcs::this_thread::sync_wait(snd);
        // static_assert(
        //     std::is_same_v<CS,
        //                    ex::completion_signatures<
        //                        ex::set_value_t(std::optional<std::basic_string<char>>),
        //                        mcs::execution::recv::set_error_t(std::exception_ptr)>>);
        // auto ret = mcs::this_thread::sync_wait(snd);
        // auto [a] = ret.value();
        // EXPECT(a == T("msg"));
    };
    TEST("no") = [] {
        static_assert(
            ex::snd::general::MATCHING_SIG<ex::set_error_t(std::exception_ptr),
                                           ex::set_error_t(std::exception_ptr)>);
        static_assert(
            ex::snd::general::MATCHING_SIG<ex::set_error_t(std::exception_ptr),
                                           ex::set_error_t(std::exception_ptr &&)>);
    };

    return 0;
}
// NOLINTNEXTLINE