#include "../test_base_head.hpp"

int main()
{
    using namespace mcs::execution; // NOLINT
    using T [[maybe_unused]] = cmplsigs::__detail::gather_signatures_helper<
        cmplsigs::completion_signatures<recv::set_value_t()>, decayed_tuple,
        std::type_identity_t>::type;

    // 空 cmplsigs::completion_signatures<> 是未定义的
    // using T2 = cmplsigs::__detail::gather_signatures_helper<
    //     cmplsigs::completion_signatures<>, decayed_tuple, std::type_identity_t>::type;

    {
        using T = decltype(just());
        using CS = ex::snd::completion_signatures_of_t<T>;
        using VT = cmplsigs::value_types_of_t<T>;
    }

    return 0;
}