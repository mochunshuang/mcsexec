#include "../test_base_head.hpp"
#include "../sched/MyScheduler.hpp"

int main()
{

    TEST("CS") = [] {
        auto snd = ex::continues_on(ex::just(1), MyScheduler{});
        using CS = ex::snd::completion_signatures_of_t<decltype(snd)>;
        static_assert(
            ex::tool::eq_set_sigs_v<
                CS, ex::cmplsigs::completion_signatures<
                        ex::set_value_t(int), ex::set_error_t(std::exception_ptr)>>);
    };
    return 0;
}