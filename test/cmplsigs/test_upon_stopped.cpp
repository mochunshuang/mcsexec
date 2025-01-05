#include "../test_base_head.hpp"

int main()
{
    using namespace mcs::execution; // NOLINT
    TEST("upon_stopped CS:  0") = [] {
        auto sndr = ex::upon_stopped(ex::just_stopped(), []() {});
        using T = ex::snd::completion_signatures_of_t<decltype(sndr), ex::empty_env>;
        static_assert(
            std::is_same_v<
                T, cmplsigs::completion_signatures<
                       recv::set_value_t(), recv::set_error_t(std::exception_ptr)>>);
    };

    TEST("upon_stopped CS: 1") = [] {
        auto sndr = ex::just_stopped() | ex::upon_stopped([]() { return 1; });
        using T = ex::snd::completion_signatures_of_t<decltype(sndr), ex::empty_env>;
        static_assert(
            std::is_same_v<
                T, cmplsigs::completion_signatures<
                       recv::set_value_t(int), recv::set_error_t(std::exception_ptr)>>);
    };

    // Note: 确实满足了，FW，和 handle 的要求，CS是满足的。不是未定义行为
    TEST("upon_stopped CS:  2") = [] {
        auto sndr = ex::just() | ex::upon_stopped([]() { return 1; });
        using T = ex::snd::completion_signatures_of_t<decltype(sndr), ex::empty_env>;
        static_assert(
            std::is_same_v<
                T, cmplsigs::completion_signatures<
                       recv::set_value_t(), recv::set_error_t(std::exception_ptr)>>);
    };

    TEST("upon_stopped CS:  2") = [] {
        auto sndr = ex::just(1) | ex::upon_stopped([]() { return 1; });
        using T = ex::snd::completion_signatures_of_t<decltype(sndr), ex::empty_env>;
        static_assert(
            std::is_same_v<
                T, cmplsigs::completion_signatures<
                       recv::set_value_t(int), recv::set_error_t(std::exception_ptr)>>);
    };

    // Note: [](int){} 肯定是编译器错误了 stopped 不会传值
    TEST("upon_stopped CS:  3") = [] {
        auto sndr [[maybe_unused]] =
            ex::just(1) | ex::upon_stopped([](int) { return 1; });
        // using T = ex::snd::completion_signatures_of_t<decltype(sndr),
        // ex::empty_env>; static_assert(
        //     std::is_same_v<
        //         T, cmplsigs::completion_signatures<
        //                recv::set_value_t(int),
        //                recv::set_error_t(std::exception_ptr)>>);
    };
    return 0;
}