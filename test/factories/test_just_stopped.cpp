#include "../test_base_head.hpp"
#include <stdexcept>
#include <variant>

int main()
{
    using namespace test;           // NOLINT
    using namespace mcs::execution; // NOLINT
    TEST("Simple test for just_stopped") = [] {
        bool called{false};
        test::channel chanel{test::channel::NO_CALL};
        auto op = connect(ex::just_stopped(),
                          void_receiver{.called = &called, .chanel = &chanel});
        EXPECT(not called && chanel == test::channel::NO_CALL);
        start(op);
        EXPECT(called && chanel == test::channel::STOPDE_CHANNEL);
    };

    TEST("just_stopped returns a sender") = [] {
        using T = decltype(ex::just_stopped());
        static_assert(ex::sender<T>, "ex::just_error must return a sender");
        static_assert(ex::sender_in<T, ex::empty_env>,
                      "ex::just_error must return a sender");
        EXPECT(ex::snd::enable_sender<T> == true);
    };

    TEST("cpo for just_stopped") = [] {
        using T = decltype(ex::just_stopped());
        using CO = ex::snd::completion_signatures_of_t<T>;
        static_assert(
            std::is_same_v<
                cmplsigs::completion_signatures<mcs::execution::recv::set_stopped_t()>,
                CO>);
    };

    TEST("value types are properly set for just_stopped") = [] {
        using T = decltype(ex::just_stopped());
        using VT = cmplsigs::value_types_of_t<T>;
        static_assert(std::is_same_v<VT, ex::cmplsigs::empty_variant>);
    };

    TEST("error types are properly set for just_stopped") = [] {
        using T = decltype(ex::just_stopped());
        using ET = cmplsigs::error_types_of_t<T>;
        static_assert(std::is_same_v<ET, ex::cmplsigs::empty_variant>);
    };

    return 0;
}