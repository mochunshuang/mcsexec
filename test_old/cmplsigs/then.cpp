#include "../../include/execution.hpp"
#include <iostream>
#include <stdexcept>
#include <utility>

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

    static_assert(
        std::is_same_v<T, cmplsigs::completion_signatures<set_error_t(std::exception_ptr),
                                                          set_value_t(float)>>);

    // Note: 和 then 的一样
    using VT = cmplsigs::value_types_of_t<decltype(obj)>;
    static_assert(std::is_same_v<std::variant<std::tuple<float>>, VT>);
    using ET = cmplsigs::error_types_of_t<decltype(obj)>;
    static_assert(std::is_same_v<std::variant<std::__exception_ptr::exception_ptr>, ET>);

    // 测试异常
    // Note: 不知道函数的参数数量和类型，无法直接获取其 noexcept 签名。
    // 因此 then 自带异常签名
    {
        auto lambda = [](int /*a*/, double /*b*/) noexcept(false) {
            return 1.0F;
        };
        // Note: 一层就能算出
        static_assert(noexcept(noexcept(lambda(1, 1.0))));
        static_assert(not noexcept(lambda(1, 1.0)));
        // Note: 可以编译器算出释放有异常
        static_assert(not noexcept(lambda(std::declval<int>(), std::declval<double>())));
        {
            auto fun = []() noexcept(noexcept(lambda(1, 1.0))) {
            };
            static_assert(not noexcept(fun()));
        }

        auto obj = just(1, 2.0) | then(lambda);
        using T = decltype(obj.get_completion_signatures(empty_env{}));
        using ET = cmplsigs::error_types_of_t<decltype(obj)>;
    }
}
void upon_error() {}
void upon_stopped() {}
