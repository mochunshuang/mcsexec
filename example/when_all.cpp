
#include <algorithm>
#include <iostream>
#include "../include/execution.hpp"

#include <iostream>
#include <format>

void test_base();
void test_base2();
void test_base3();
int main()
{
    test_base();
    test_base2();
    test_base3();
    std::cout << "hello world\n";
    return 0;
}
#if 1
void test_base()
{
    using namespace mcs::execution;
    sender auto sends_1 = just(1);                    // NOLINT
    sender auto sends_abc = just(std::string("abc")); // NOLINT
    {
        using T = snd::completion_signatures_of_t<decltype(sends_1)>;
        using T2 = snd::completion_signatures_of_t<decltype(sends_abc)>;

        static_assert(std::is_same_v<completion_signatures<set_value_t(int)>, T>);
        static_assert(
            std::is_same_v<completion_signatures<set_value_t(std::string)>, T2>);
        // 空处理
        struct A
        {
            using type = void; // NOLINT
        };
        static_assert(std::is_same_v<set_value_t(), set_value_t(A::type)>);

        using T3 = cmplsigs::error_types_of_t<decltype(sends_1)>;
        static_assert(std::is_same_v<mcs::execution::cmplsigs::empty_variant, T3>);
    }

    sender auto both = when_all(sends_1, sends_abc);
    {
        using T [[maybe_unused]] = snd::completion_signatures_of_t<decltype(both)>;
        using T2 = cmplsigs::value_types_of_t<decltype(both)>;
        // Note: 默认外部模板：variant，内部模板tuple
        // Note: tuple<Ts...> => set_value_t<Ts...> 即可 算出一路的完成签名
        static_assert(
            std::is_same_v<std::variant<std::tuple<int, std::basic_string<char>>>, T2>);

        using T3 = cmplsigs::error_types_of_t<decltype(both)>;

        static_assert(std::is_same_v<cmplsigs::empty_variant, T3>);
    }
    sender auto final = then(both, [](auto... args) {
        std::cout << std::format("the two args: {}, {}\n", args...);
    });
    mcs::this_thread::sync_wait(std::move(final));
}
#endif

void test_base2()
{
    using namespace mcs::execution;
    sender auto sends_1 = just(); // NOLINT
    sender auto sends_2 = just(); // NOLINT
    sender auto both = when_all(sends_1, sends_2);
    sender auto final = then(
        both, [](auto... args) { std::cout << std::format("the two args: done\n"); });
    mcs::this_thread::sync_wait(std::move(final));
}

void test_base3()
{
    using namespace mcs::execution;
    sender auto sends_1 =
        just() | then([] { std::cout << std::format("sends_1: done\n"); }); // NOLINT
    sender auto sends_2 = just(2) | then([](int v) {
                              std::cout << std::format("sends_2: done\n");
                              return v * 2;
                          }); // NOLINT

    auto lambda = [](int v) {
        std::cout << std::format("sends_2: done\n");
        return v * 2;
    };
    static_assert(noexcept(noexcept(lambda(2))));

    mcs::this_thread::sync_wait(std::move(sends_2));

    sender auto both = when_all(sends_1, sends_2);
    sender auto final = then(
        both, [](auto... args) { std::cout << std::format("one args: {}\n", args...); });
    mcs::this_thread::sync_wait(std::move(final));
}