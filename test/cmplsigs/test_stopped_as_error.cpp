#include "../test_base_head.hpp"

using namespace mcs::execution; // NOLINT

int main()
{
    TEST("CS") = [] {
        auto snd = ex::stopped_as_error(ex::just(1), -1);
        using T = snd::completion_signatures_of_t<decltype(snd)>;
        static_assert(
            tool::eq_set_sigs_v<T, cmplsigs::completion_signatures<
                                       set_error_t(int), set_error_t(std::exception_ptr),
                                       set_value_t(int)>>);
    };

    return 0;
}