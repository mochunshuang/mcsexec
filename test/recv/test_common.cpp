#include "../test_base_head.hpp"
#include <concepts>
#include <exception>

int main()
{
    using namespace mcs::execution; // NOLINT
    TEST("recv concept test for common recvr") = [] {
        // fun_receiver
        {
            auto fun = [](int) {
            };
            using T = test::fun_receiver<decltype(fun)>;
            static_assert(recv::receiver<T>);
        }

        // logging_receiver
        {
            using T = test::logging_receiver;
            static_assert(recv::receiver<T>);
        }

        // typecat_receiver
        {
            using T = test::typecat_receiver<int>;
            static_assert(recv::receiver<T>);
        }
    };
    return 0;
};