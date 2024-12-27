
/**
 * @brief into_variant adapts a sender with multiple value completion signatures into a
 * sender with just one value completion signature consisting of a variant of tuples.
 *
 */

#include "../../include/execution.hpp"

#include <iostream>

namespace A::B
{

};
// using N = A::B; // 错误，不是类型
namespace N = A::B; // NOLINT

void test_base();
int main()
{
    test_base();
    std::cout << "hello world\n";
    return 0;
}
void test_base()
{
    namespace ex = mcs::execution;
    auto snd = ex::into_variant(ex::just(1));
    using T = decltype(snd.get_completion_signatures(ex::empty_env{}));
    static_assert(std::is_same_v<T, ex::cmplsigs::completion_signatures<
                                        ex::set_value_t(std::variant<std::tuple<int>>),
                                        ex::set_error_t(std::exception_ptr)>>);

    {
        [[maybe_unused]] auto snd =
            ex::into_variant(ex::just(1, 1.0, 1.0F)) |
            ex::then([](std::variant<std::tuple<int, double, float>>) {});
    }
}