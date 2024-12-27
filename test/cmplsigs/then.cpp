#include "../../include/execution.hpp"
#include <iostream>
#include <stdexcept>

void then_test();
void upon_error();
void upon_stopped();

int main()
{
    then_test();
    upon_error();
    upon_stopped();
    std::cout << "hello world\n";
    return 0;
}

void then_test()
{
    using namespace mcs::execution;

    auto obj = just(1, 2.0) | then([](int a, double b) { return 1.0F; });
    using T = decltype(obj.get_completion_signatures(empty_env{}));

    static_assert(std::is_same_v<T, cmplsigs::completion_signatures<set_value_t(float)>>);

    // Note: 和 then 的一样
    using VT = cmplsigs::value_types_of_t<decltype(obj)>;
    static_assert(std::is_same_v<std::variant<std::tuple<float>>, VT>);
    using ET = cmplsigs::error_types_of_t<decltype(obj)>;
    static_assert(std::is_same_v<cmplsigs::empty_variant, ET>);

    // 测试异常
    
}
void upon_error() {}
void upon_stopped() {}
