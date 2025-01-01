#include "../test_base_head.hpp"

int main()
{
    using namespace mcs::execution; // NOLINT
    TEST("let_stopped returns a sender") = [] {
        auto snd = ex::let_stopped(ex::just(), [] { return ex::just(); });
        static_assert(ex::sender<decltype(snd)>);
    };

    return 0;
}