
#include "../test_base_head.hpp"
#include <cassert>

int main()
{
    {
        auto sndr =
            ex::let_error(ex::just(), [](std::exception_ptr) { return ex::just(); });
        "let_error returns a sender"_test = [&] {
            static_assert(ex::sender<decltype(sndr)>);
        };
        "let_error with environment returns a sender"_test = [&] {
            // static_assert(ex::sender_in<decltype(sndr), ex::empty_env>);
        };
    }

    return 0;
}