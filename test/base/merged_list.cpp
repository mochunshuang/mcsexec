#include <type_traits>
#include <iostream>
#include "../test_base_head.hpp"

// 默认的 type_list 实现
template <typename... Ts>
struct type_list
{
};

template <template <typename...> class TypeList, typename List1, typename List2>
struct merge_two_type_lists;

template <template <typename...> class TypeList, typename... Ts1, typename... Ts2>
struct merge_two_type_lists<TypeList, TypeList<Ts1...>, TypeList<Ts2...>>
{
    using type = TypeList<Ts1..., Ts2...>;
};

template <template <typename...> class TypeList, typename... Lists>
struct merge_type_lists;

template <template <typename...> class TypeList>
struct merge_type_lists<TypeList>
{
    using type = TypeList<>;
};

// 特化：处理单个 type_list
template <template <typename...> class TypeList, typename List>
struct merge_type_lists<TypeList, List>
{
    using type = List;
};

// 特化：处理多个 type_list
template <template <typename...> class TypeList, typename List1, typename List2,
          typename... Rest>
struct merge_type_lists<TypeList, List1, List2, Rest...>
{
    using merged_first_two = typename merge_two_type_lists<TypeList, List1, List2>::type;
    using type = typename merge_type_lists<TypeList, merged_first_two, Rest...>::type;
};

// 测试辅助函数
template <typename T>
void test_merge(const char *name)
{
    std::cout << "Test " << name << ": ";
    if (std::is_same_v<T, type_list<>>)
    {
        std::cout << "Empty list\n";
    }
    else
    {
        std::cout << "Success\n";
    }
}

void merge_CPO(); // NOLINT

int main()
{
    using list0 = type_list<>;
    using list1 = type_list<int, float>;
    using list2 = type_list<double, char>;
    using list3 = type_list<bool, long>;
    using list4 = type_list<>;

    using merged_list1 =
        merge_type_lists<type_list, list0, list1, list2, list3, list4>::type;

    static_assert(
        std::is_same_v<merged_list1, type_list<int, float, double, char, bool, long>>,
        "Merged list is incorrect!");

    // 测试 1: 空列表
    using test1 = merge_type_lists<type_list>::type;
    static_assert(std::is_same_v<test1, type_list<>>, "Test 1 failed");
    test_merge<test1>("Empty list");

    // 测试 2: 单个空列表
    using test2 = merge_type_lists<type_list, type_list<>>::type;
    static_assert(std::is_same_v<test2, type_list<>>, "Test 2 failed");
    test_merge<test2>("Single empty list");

    // 测试 3: 单个非空列表
    using test3 = merge_type_lists<type_list, type_list<int, float>>::type;
    static_assert(std::is_same_v<test3, type_list<int, float>>, "Test 3 failed");
    test_merge<test3>("Single non-empty list");

    // 测试 4: 多个空列表
    using test4 =
        merge_type_lists<type_list, type_list<>, type_list<>, type_list<>>::type;
    static_assert(std::is_same_v<test4, type_list<>>, "Test 4 failed");
    test_merge<test4>("Multiple empty lists");

    // 测试 5: 多个非空列表
    using test5 = merge_type_lists<type_list, type_list<int>, type_list<float>,
                                   type_list<double>>::type;
    static_assert(std::is_same_v<test5, type_list<int, float, double>>, "Test 5 failed");
    test_merge<test5>("Multiple non-empty lists");

    // 测试 6: 混合空列表和非空列表
    using test6 = merge_type_lists<type_list, type_list<>, type_list<int>, type_list<>,
                                   type_list<float>>::type;
    static_assert(std::is_same_v<test6, type_list<int, float>>, "Test 6 failed");
    test_merge<test6>("Mixed empty and non-empty lists");

    // 测试 7: 复杂混合列表
    using test7 =
        merge_type_lists<type_list, type_list<int>, type_list<>, type_list<float, double>,
                         type_list<>, type_list<char>>::type;
    static_assert(std::is_same_v<test7, type_list<int, float, double, char>>,
                  "Test 7 failed");
    test_merge<test7>("Complex mixed lists");
    merge_CPO();

    return 0;
};
void merge_CPO()
{
    using namespace mcs::execution; //
    using VT =
        cmplsigs::completion_signatures<recv::set_value_t(), recv::set_value_t(int),
                                        recv::set_value_t()>;
    using ET = cmplsigs::completion_signatures<recv::set_error_t(int)>;
    using ST = cmplsigs::completion_signatures<recv::set_stopped_t()>;
    using RT = merge_type_lists<cmplsigs::completion_signatures, VT, ET, ST>::type;
    static_assert(
        std::is_same_v<
            RT, cmplsigs::completion_signatures<
                    recv::set_value_t(), recv::set_value_t(int), recv::set_value_t(),
                    recv::set_error_t(int), recv::set_stopped_t()>>);
    using UT = tfxcmplsigs::unique_variadic_template<RT>::type;
    static_assert(std::is_same_v<UT, cmplsigs::completion_signatures<
                                         recv::set_value_t(), recv::set_value_t(int),
                                         recv::set_error_t(int), recv::set_stopped_t()>>);
}