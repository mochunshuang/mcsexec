#include "../test_base_head.hpp"

int main()
{
    TEST("then cs") = [] {
        auto sndr = ex::just() | ex::then([] {});
        using T = decltype(sndr);
        using CS = ex::cmplsigs::get_completion_signatures<T>;
        static_assert(std::is_same_v<CS, ex::cmplsigs::completion_signatures<
                                             ex::recv::set_value_t(),
                                             ex::recv::set_error_t(std::exception_ptr)>>);
        mcs::this_thread::sync_wait(std::move(sndr));
    };

    TEST("then cs with v") = [] {
        auto sndr = ex::just() | ex::then([] { return 1; });
        using T = decltype(sndr);
        using CS = ex::cmplsigs::get_completion_signatures<T>;
        static_assert(std::is_same_v<CS, ex::cmplsigs::completion_signatures<
                                             ex::recv::set_value_t(int),
                                             ex::recv::set_error_t(std::exception_ptr)>>);
        auto [ret] = mcs::this_thread::sync_wait(std::move(sndr)).value();
        EXPECT(ret == 1);
    };

    return 0;
}