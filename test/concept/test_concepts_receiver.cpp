#include "../test_base_head.hpp"

using namespace mcs::execution; // NOLINT

int main()
{

    TEST("base") = [] {
        using completion_signatures = cmplsigs::completion_signatures<
            set_value_t(), set_error_t(std::exception_ptr), set_stopped_t()>;

        struct MyReceiver
        {
            using receiver_concept = receiver_t;

            void set_value() && {}                   // NOLINT
            void set_error(std::exception_ptr) && {} // NOLINT
            void set_stopped() && {}                 // NOLINT

            ex::empty_env get_env() const noexcept
            {
                return {};
            }
        };

        using namespace mcs::execution::recv;
        static_assert(receiver<MyReceiver>);
    };

    return 0;
}