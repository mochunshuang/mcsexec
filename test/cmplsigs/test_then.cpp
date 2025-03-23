#include "../test_base_head.hpp"

int main()
{
    TEST("then cs") = [] {
        auto sndr = ex::just() | ex::then([] {});
        using T = decltype(sndr);
        using CS = ex::snd::completion_signatures_of_t<T>;
        static_assert(std::is_same_v<CS, ex::cmplsigs::completion_signatures<
                                             ex::recv::set_value_t(),
                                             ex::recv::set_error_t(std::exception_ptr)>>);
        mcs::this_thread::sync_wait(std::move(sndr));
    };

    TEST("then cs with v") = [] {
        auto sndr = ex::just() | ex::then([] { return 1; });
        using T = decltype(sndr);
        using CS = ex::snd::completion_signatures_of_t<T>;
        static_assert(std::is_same_v<CS, ex::cmplsigs::completion_signatures<
                                             ex::recv::set_value_t(int),
                                             ex::recv::set_error_t(std::exception_ptr)>>);
        auto [ret] = mcs::this_thread::sync_wait(std::move(sndr)).value();
        EXPECT(ret == 1);
    };

    TEST("then cs with v2") = [] {
        using CS = ex::cmplsigs::completion_signatures<
            ex::recv::set_value_t(int), ex::recv::set_error_t(std::exception_ptr)>;
        using CS2 = ex::cmplsigs::completion_signatures<ex::recv::set_value_t(int &&)>;

        using CS3 = decltype(CS{} + CS2{});
        static_assert(std::is_same_v<CS, CS3>);
    };

    return 0;
}