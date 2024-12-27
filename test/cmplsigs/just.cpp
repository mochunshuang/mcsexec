#include "../../include/execution.hpp"
#include <iostream>
#include <stdexcept>

int main()
{
    using namespace mcs::execution;

    {
        auto obj = just(1, 2.0);
        using T = decltype(obj.get_completion_signatures(empty_env{}));
        static_assert(
            std::is_same_v<T, cmplsigs::completion_signatures<set_value_t(int, double)>>);
        //  value_types_of_t
        static_assert(std::is_same_v<std::variant<std::tuple<int, double>>,
                                     cmplsigs::value_types_of_t<decltype(obj)>>);

        // Note: 空。无异常的时候，返回的是 cmplsigs::empty_variant; 设计用的空类型
        using ET = cmplsigs::error_types_of_t<decltype(obj)>;
        static_assert(std::is_same_v<cmplsigs::empty_variant, ET>);
    }
    {
        auto obj = just_error(std::invalid_argument{"非法参数"});
        using T = decltype(obj.get_completion_signatures(empty_env{}));

        static_assert(
            std::is_same_v<
                T, cmplsigs::completion_signatures<set_error_t(std::invalid_argument)>>);

        // Note: VT 或 ET 空的时候，都叫 cmplsigs::empty_variant
        using VT = cmplsigs::value_types_of_t<decltype(obj)>;
        static_assert(std::is_same_v<cmplsigs::empty_variant, VT>);

        // Note: ET 不需要内置 tuple 类型
        using ET = cmplsigs::error_types_of_t<decltype(obj)>;
        static_assert(std::is_same_v<std::variant<std::invalid_argument>, ET>);
    }
    {
        auto obj = just_stopped();
        using T = decltype(obj.get_completion_signatures(empty_env{}));

        static_assert(
            std::is_same_v<T, cmplsigs::completion_signatures<set_stopped_t()>>);

        // Note: VT 、ET 都是 cmplsigs::empty_variant
        using VT = cmplsigs::value_types_of_t<decltype(obj)>;
        static_assert(std::is_same_v<cmplsigs::empty_variant, VT>);
        using ET = cmplsigs::error_types_of_t<decltype(obj)>;
        static_assert(std::is_same_v<cmplsigs::empty_variant, ET>);

        static_assert(std::is_same_v<VT, ET>);
    }
    std::cout << "hello world\n";
    return 0;
}