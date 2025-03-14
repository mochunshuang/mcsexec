#include <concepts>
#include <iostream>
#include <type_traits>

// NOLINTBEGIN
template <class... Ts>
struct product_type
{
};

struct set_value_t;

namespace cmplsigs
{
    struct undefine_completion_signatures_for
    {
    };

    template <class Sndr, class... Env>
    struct completion_signatures_for_impl;

    namespace __detail
    {
        template <template <class...> class T, class... Args>
        concept has_completion_type = requires { typename T<Args...>::type; };

        template <class Sndr, class... Env>
        struct __completion_signatures_for;

        template <class Sndr, class... Env>
            requires(
                not has_completion_type<completion_signatures_for_impl, Sndr, Env...>)
        struct __completion_signatures_for<Sndr, Env...>
        {
            using type = undefine_completion_signatures_for;
        };

        template <class Sndr, class... Env>
            requires(has_completion_type<completion_signatures_for_impl, Sndr, Env...>)
        struct __completion_signatures_for<Sndr, Env...>
        {
            using type = typename completion_signatures_for_impl<Sndr, Env...>::type;
        };

    }; // namespace __detail

    template <class Sndr, class... Env>
    using completion_signatures_for = // exposition only
        typename __detail::__completion_signatures_for<Sndr, Env...>::type;
}; // namespace cmplsigs
namespace factories
{
    template <class... Ts>
    struct __just_t
    {
    };
} // namespace factories

namespace snd::__detail
{
    template <class From, class To>
    concept decays_to = std::same_as<std::decay_t<From>, To>;

    // 主模板定义
    template <class Tag, class Data, class... Child>
    struct basic_sender
    {
        static consteval auto get_date()
        {
            return 1;
        }

        template <decays_to<basic_sender> Self, class... Env>
        static consteval auto get_completion_signatures() noexcept
            -> cmplsigs::completion_signatures_for<std::remove_cvref_t<Self>, Env...>
        {
            return {};
        }
    };
} // namespace snd::__detail

// 针对__just_t的特化实现
namespace cmplsigs
{
    template <class... Ts>
    struct completion_signatures
    {
    };
    template <class Completion, typename... T, class... Env>
    struct completion_signatures_for_impl<
        snd::__detail::basic_sender<factories::__just_t<Completion>, product_type<T...>>,
        Env...>
    {
        using type = completion_signatures<Completion(T...)>; // 自定义类型
    };
} // namespace cmplsigs

// 测试用例
int main()
{
    using namespace snd::__detail;
    using namespace factories;

    // 测试默认实现
    using main_sender = basic_sender<int, product_type<>>;
    static_assert(main_sender::get_date() == 1);
    auto main_sig = main_sender::template get_completion_signatures<main_sender>();

    static_assert(
        std::is_same_v<decltype(main_sig), cmplsigs::undefine_completion_signatures_for>);

    // 测试特化版本
    using just_sender = basic_sender<__just_t<set_value_t>, product_type<char, int>>;
    auto just_sig = just_sender::template get_completion_signatures<just_sender>();

    static_assert(just_sender::get_date() == 1);

    // NOTE: 保证了，编译期确定
    static_assert(
        std::is_same_v<decltype(just_sig),
                       cmplsigs::completion_signatures<set_value_t(char, int)>>);

    {
        // 带 env_t 一样
        struct env_t
        {
        };
        static_assert(
            std::is_same_v<decltype(just_sender::template get_completion_signatures<
                                    just_sender, env_t>()),
                           cmplsigs::completion_signatures<set_value_t(char, int)>>);
    }

    std::cout << "All tests passed!\n";
    return 0;
}
// NOLINTEND