#include "../test_base_head.hpp"
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
#include <string>

int main()
{
    using namespace test;           // NOLINT
    using namespace mcs::execution; // NOLINT

    TEST("Simple test for just") = [] {
        static_assert(std::is_constructible_v<
                      test::expect_value_receiver<mcs::execution::empty_env, int>,
                      test::expect_value_receiver<mcs::execution::empty_env, int> &>);
        auto o1 = connect(just(1), expect_value_receiver(1));
        start(o1);
        auto o2 = connect(just(2), expect_value_receiver(2));
        start(o2);
        auto o3 = connect(just(3), expect_value_receiver(3));
        start(o3);

        auto o4 = connect(just(std::string("this")),
                          expect_value_receiver(std::string("this")));
        start(o4);
        auto o5 = connect(just(std::string("that")),
                          expect_value_receiver(std::string("that")));
        start(o5);
    };

    TEST("just returns a sender") = [] {
        using t = decltype(ex::just(1));
        static_assert(ex::sender<t>, "ex::just must return a sender");
        EXPECT(ex::sender<t> == true);
        EXPECT(ex::snd::enable_sender<t> == true);
    };

    TEST("just can handle multiple values") = [] {
        bool executed{false};
        auto f = [&](int x, double d) {
            EXPECT(x == 3);
            EXPECT(d == 0.14);
            executed = true;
        };
        auto op = ex::connect(ex::just(3, 0.14), make_fun_receiver(std::move(f)));
        start(op);
        EXPECT(executed);
    };

    TEST("value types are properly set for just") = [] {
        static_assert(std::is_same_v<cmplsigs::value_types_of_t<decltype(ex::just(1))>,
                                     std::variant<std::tuple<int>>>);
        static_assert(
            std::is_same_v<
                std::variant<std::tuple<int, double, std::string>>,
                cmplsigs::value_types_of_t<decltype(ex::just(1, 1.0, std::string{}))>>);
    };

    TEST("error types are properly set for just") = [] {
        using ET = cmplsigs::error_types_of_t<decltype(ex::just(1))>;
        static_assert(std::is_same_v<cmplsigs::error_types_of_t<decltype(ex::just(1))>,
                                     ex::cmplsigs::empty_variant>);
    };

    TEST("cpo for just") = [] {
        using CO = cmplsigs::get_completion_signatures<decltype(ex::just(1, 1.0))>;
        static_assert(std::is_same_v<cmplsigs::completion_signatures<
                                         mcs::execution::recv::set_value_t(int, double)>,
                                     CO>);
    };

    TEST("called status for just") = [] {
        bool called{false};
        auto recv = test::value_receiver{&called, 1, 1.0};
        static_assert(recv::receiver<decltype(recv)>);

        EXPECT(not called);
        auto op = connect(just(1, 1.0), std::move(recv));
        EXPECT(not called);

        start(op);
        EXPECT(called);
    };

    return 0;
}