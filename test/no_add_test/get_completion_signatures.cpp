
#include <concepts>
#include <iostream>

// NOLINTBEGIN
template <class... Ts>
struct product_type
{
};

namespace factories
{
    template <class... Ts>
    struct __just_t
    {
    };
} // namespace factories

namespace cmplsigs
{
    template <class... Ts>
    struct completion_signatures
    {
    };
} // namespace cmplsigs

namespace snd::__detail
{
    template <class From, class To>
    concept decays_to = std::same_as<std::decay_t<From>, To>;

    // 公共基类模板
    template <class Derived>
    struct basic_sender_base
    {
        static consteval auto get_date()
        {
            return 1; // 公共实现
        }
    };

    // 主模板（继承基类）
    template <class Tag, class Data, class... Child>
    struct basic_sender : basic_sender_base<basic_sender<Tag, Data, Child...>>
    {
        // 通过CRTP访问基类
        using base_type = basic_sender_base<basic_sender>;
        using base_type::get_date;

        template <decays_to<basic_sender> Self, class... Env>
        static consteval auto get_completion_signatures();
    };

    // 特化版本（继承同一个基类）
    template <class Completion, typename... T>
    struct basic_sender<factories::__just_t<Completion>, product_type<T...>>
        : basic_sender_base<
              basic_sender<factories::__just_t<Completion>, product_type<T...>>>
    {
        using base_type = basic_sender_base<basic_sender>;
        using base_type::get_date; // 继承基类实现

        template <decays_to<basic_sender> Self, class... Env>
        static consteval auto get_completion_signatures()
        {
            // 自定义实现
            return cmplsigs::completion_signatures<Completion(T...)>{};
        }
    };

    // NOTE: 特化和偏特化 一点关系都没有
    // NOTE: 妄想继承主模板的能力，只能 CRTP模式
    template <>
    struct basic_sender<int, int>
    {
    };
} // namespace snd::__detail

int main()
{
    using namespace snd::__detail;
    using namespace factories;

    // 测试主模板
    using main_sender = basic_sender<int, product_type<>>;
    static_assert(main_sender::get_date() == 1);

    // 测试特化版本
    struct set_value_t;
    using just_sender = basic_sender<__just_t<set_value_t>, product_type<float, char>>;
    static_assert(just_sender::get_date() == 1);                      // 继承基类方法
    auto sig = just_sender::get_completion_signatures<just_sender>(); // 调用特化版本

    static_assert(
        std::is_same_v<decltype(sig),
                       cmplsigs::completion_signatures<set_value_t(float, char)>>);

    {
        using Sndr = basic_sender<int, int>;
        // NOTE: 语法错误。 主模板，特化版 一点关系都没有
        //  static_assert(Sndr::get_date() == 1);
    }

    std::cout << "All tests passed!\n";
    return 0;
}
// NOLINTEND