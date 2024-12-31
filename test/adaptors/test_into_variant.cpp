#include "../test_base_head.hpp"

int main()
{
    TEST("into_variant returns a sender") = [] {
        auto snd = ex::into_variant(ex::just(1));
        static_assert(ex::sender<decltype(snd)>);
    };
    TEST("into_variant with environment returns a sender") = [] {
        auto snd = ex::into_variant(ex::just(1));
        static_assert(ex::sender_in<decltype(snd), mcs::execution::empty_env>);
    };
    TEST("into_variant simple example") = [] {
    };

    return 0;
}