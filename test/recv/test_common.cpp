#include "../test_base_head.hpp"
#include <exception>

int main()
{
    using namespace mcs::execution; // NOLINT
    TEST("recv concept test for common recvr") = [] {
        should("base_expect_receiver") = [] {
            using T = ::test::base_expect_receiver<>;
            static_assert(recv::receiver<T>);
        };

        // expect_error
        {
            using T = test::expect_error_receiver_ex<std::exception_ptr>;
            static_assert(recv::receiver<T>);
        }
        {
            using T = test::expect_error_receiver<>;
            static_assert(recv::receiver<T>);
        }

        // expect_stopped
        {
            using T = test::expect_stopped_receiver_ex<>;
            static_assert(recv::receiver<T>);
        }
        {
            using T = test::expect_stopped_receiver<>;
            static_assert(recv::receiver<T>);
        }

        // value_receiver
        {
            using T = test::expect_value_receiver_ex<int>;
            static_assert(recv::receiver<T>);
        }
        {
            using T = test::expect_value_receiver<>;
            static_assert(recv::receiver<T>);
        }

        // value_receiver
        {
            using T = test::expect_void_receiver_ex<>;
            static_assert(recv::receiver<T>);
        }
        {
            using T = test::expect_void_receiver<>;
            static_assert(recv::receiver<T>);
        }

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