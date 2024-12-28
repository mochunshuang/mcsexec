
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
void test_base2();
int main()
{
    test_base();
    test_base2();
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
        using T = decltype(snd.get_completion_signatures(ex::empty_env{}));
        static_assert(
            std::is_same_v<T,
                           ex::cmplsigs::completion_signatures<
                               ex::set_value_t(), ex::set_error_t(std::exception_ptr)>>);
    }
    {
        [[maybe_unused]] auto snd = ex::into_variant(ex::just(1, 1.0, 1.0F));
        using T = decltype(snd.get_completion_signatures(ex::empty_env{}));
        static_assert(
            std::is_same_v<
                T, ex::cmplsigs::completion_signatures<
                       ex::set_value_t(std::variant<std::tuple<int, double, float>>),
                       ex::set_error_t(std::exception_ptr)>>);
    }
}
void test_base2()
{
    namespace ex = mcs::execution;
    auto snd = ex::when_all(ex::into_variant(ex::just(1)));
    using T = decltype(snd.get_completion_signatures(ex::empty_env{}));
    static_assert(std::is_same_v<ex::cmplsigs::completion_signatures<
                                     ex::set_value_t(std::variant<std::tuple<int>>),
                                     ex::set_error_t(std::exception_ptr)>,
                                 T>);
    {
        auto snd = ex::when_all(ex::just(1), ex::just(1.0));
        using T = decltype(snd.get_completion_signatures(ex::empty_env{}));
        static_assert(
            std::is_same_v<
                ex::cmplsigs::completion_signatures<ex::set_value_t(int, double),
                                                    ex::set_error_t(std::exception_ptr)>,
                T>);
    }
    {
        auto snd =
            ex::when_all(ex::into_variant(ex::just(1)), ex::into_variant(ex::just(1.0)));
        using T = decltype(snd.get_completion_signatures(ex::empty_env{}));
        // TODO 总感觉不对
        // Note: 或许是 into_variant 理解有问题？又或者本就如此？
        // Note: 没有错：variant_type 是 value_types_of_t，就是 td::variant<std::tuple<T>>
        // Note: type_identity<value_types_of_t<child-type<Sndr>,env_of_t<Rcvr>>>
        /**
        //Note: variant_type(decayed_tuple<Args...> ; 说明了一切
         recv::set_value(std::move(rcvr),
                                        variant_type(decayed_tuple<Args...>{
                                            std::forward<Args>(args)...}));
         */
        static_assert(
            std::is_same_v<ex::cmplsigs::completion_signatures<
                               ex::set_value_t(std::variant<std::tuple<int>>,
                                               std::variant<std::tuple<double>>),
                               ex::set_error_t(std::exception_ptr)>,
                           T>);
    }
    {
        auto snd =
            ex::when_all(ex::into_variant(ex::just(1)), ex::into_variant(ex::just(1.0)));
        using T = decltype(snd.get_completion_signatures(ex::empty_env{}));

        auto snd2 = ex::when_all_with_variant(ex::just(1), ex::just(1.0));
        using T2 = decltype(snd2.get_completion_signatures(ex::empty_env{}));

        static_assert(std::is_same_v<T, T2>);
    }
}