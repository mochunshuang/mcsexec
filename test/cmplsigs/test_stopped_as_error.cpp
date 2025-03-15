#include "../test_base_head.hpp"

using namespace mcs::execution; // NOLINT

int main()
{
    TEST("CS") = [] {
        auto snd = ex::stopped_as_error(ex::just(1), -1);
        using T = snd::completion_signatures_of_t<decltype(snd)>;
        static_assert(
            tool::is_same_v<T, cmplsigs::completion_signatures<set_value_t(int)>>);
    };

    TEST("CS2") = [] {
        auto snd = ex::stopped_as_error(ex::just_stopped(), -1.0);
        using T = snd::completion_signatures_of_t<decltype(snd)>;
        static_assert(
            tool::is_same_v<T, cmplsigs::completion_signatures<set_error_t(double)>>);
    };

    return 0;
}