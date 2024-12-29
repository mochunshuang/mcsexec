#include "../test_base_head.hpp"
#include <stdexcept>
#include <variant>

int main()
{
    using namespace test;           // NOLINT
    using namespace mcs::execution; // NOLINT
    TEST("Simple test for just_error") = [] {
        int err_code = 400; // NOLINT
        bool called{false};
        auto op = connect(ex::just_error(err_code),
                          error_receiver{.called = &called, .error = err_code});

        EXPECT(not called);
        start(op);
        EXPECT(called);
        // std::exception_ptr
        {
            bool called{false};
            auto error = std::invalid_argument("invalid arg");
            std::exception_ptr eptr = std::make_exception_ptr(error);

            auto op = connect(ex::just_error(eptr),
                              error_receiver{.called = &called, .error = eptr});
            EXPECT(not called);
            start(op);
            EXPECT(called);
        }
    };

    TEST("just_error returns a sender") = [] {
        using t = decltype(ex::just_error(1));
        static_assert(ex::sender<t>, "ex::just_error must return a sender");
        static_assert(ex::sender_in<t, ex::empty_env>,
                      "ex::just_error must return a sender");
        EXPECT(ex::snd::enable_sender<t> == true);
    };

    TEST("cpo for just_error") = [] {
        using CO = cmplsigs::get_completion_signatures<decltype(ex::just_error(
            std::exception_ptr{}))>;
        static_assert(
            std::is_same_v<cmplsigs::completion_signatures<
                               mcs::execution::recv::set_error_t(std::exception_ptr)>,
                           CO>);
    };

    TEST("value types are properly set for just_error") = [] {
        using T = decltype(ex::just_error(std::exception_ptr{}));
        using VT = cmplsigs::value_types_of_t<T>;
        static_assert(std::is_same_v<VT, ex::cmplsigs::empty_variant>);
    };

    TEST("error types are properly set for just_error") = [] {
        using T = decltype(ex::just_error(std::exception_ptr{}));
        using ET = cmplsigs::error_types_of_t<T>;
        static_assert(std::is_same_v<ET, std::variant<std::exception_ptr>>);
    };

    return 0;
}