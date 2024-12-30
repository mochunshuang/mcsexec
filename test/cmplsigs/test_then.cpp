#include "../test_base_head.hpp"

int main()
{
    TEST("then cs") = [] {
        auto sndr = ex::just() | ex::then([] {});
        using T = decltype(sndr);
        using CS = ex::cmplsigs::get_completion_signatures<T>;
        static_assert(
            std::is_same_v<CS,
                           ex::cmplsigs::completion_signatures<ex::recv::set_value_t()>>);
    };

    return 0;
}