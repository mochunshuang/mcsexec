#include "../test_base_head.hpp"
#include <type_traits>

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

    return 0;
}