#include <iostream>
#include "./transform_completion_signatures.h"

// NOLINTBEGIN
// 测试1: 基础转换测试 - 默认转换器
constexpr void test_default_transform()
{
    using namespace std;

    // 测试value转换
    auto cs1 = completion_signatures<set_value_t(int)>{};
    static_assert(valid_completion_signatures<decltype(cs1)>);
    auto res1 = transform_completion_signatures(cs1);
    static_assert(res1.contains<set_value_t(int)>);

    // // 测试error转换
    auto cs2 = completion_signatures<set_error_t(exception_ptr)>{};
    auto res2 = transform_completion_signatures(cs2);
    static_assert(res2.contains<set_error_t(exception_ptr)>);

    // // 测试stopped转换
    auto cs3 = completion_signatures<set_stopped_t()>{};
    auto res3 = transform_completion_signatures(cs3);
    static_assert(res3.contains<set_stopped_t()>);
}

// 测试2: 自定义转换器测试
constexpr void test_custom_transform()
{
    using namespace std;

    // 自定义value转换器：添加double版本 // NOLINTNEXTLINE
    constexpr auto custom_value = []<class... As>() {
        return completion_signatures<set_value_t(As...), set_value_t(double)>();
    };

    // 自定义error转换器：包装错误类型 // NOLINTNEXTLINE
    constexpr auto custom_error = []<class E>() {
        return completion_signatures<set_error_t(pair<E, E>)>();
    };

    auto cs = completion_signatures<set_value_t(int), set_error_t(char)>{};
    auto res = transform_completion_signatures(cs, custom_value, custom_error);

    static_assert(res.contains<set_value_t(int)>);
    static_assert(res.contains<set_value_t(double)>);
    static_assert(res.contains<set_error_t(pair<char, char>)>);

    static_assert(
        std::is_same_v<decltype(res),
                       completion_signatures<set_value_t(int), set_value_t(double),
                                             set_error_t(std::pair<char, char>)>>);
}

// 测试3: 多签名合并测试
constexpr void test_multiple_signatures_merge()
{
    using namespace std;

    auto cs = completion_signatures<set_value_t(int), set_error_t(exception_ptr),
                                    set_stopped_t()>{};

    constexpr auto value_xform = []<class... As>() {
        return completion_signatures<set_value_t(As...)>();
    };

    constexpr auto error_xform = []<class E>() {
        return completion_signatures<set_error_t(E), set_error_t(string)>();
    };

    auto res = transform_completion_signatures(cs, value_xform, error_xform);

    static_assert(res.contains<set_value_t(int)>);
    static_assert(res.contains<set_error_t(exception_ptr)>);
    static_assert(res.contains<set_error_t(string)>);
    static_assert(res.contains<set_stopped_t()>);
    static_assert(std::is_same_v<
                  decltype(res),
                  completion_signatures<set_value_t(int), set_error_t(std::exception_ptr),
                                        set_error_t(std::string), set_stopped_t()>

                  >);
    {
        // NOTE: 有重复的会被删除
        auto other = completion_signatures<set_value_t(double), set_stopped_t()>{};
        auto res2 =
            transform_completion_signatures(cs, value_xform, error_xform, {}, other);
        static_assert(
            std::is_same_v<
                decltype(res2),
                completion_signatures<set_value_t(int), set_error_t(std::exception_ptr),
                                      set_error_t(std::string), set_stopped_t(),
                                      set_value_t(double)>>);
        // NOTE: 加法判断，没有问题
        static_assert(std::is_same_v<decltype(res2), decltype(res + other)>);
    }
}

// 测试4: 无效签名检测测试
constexpr void test_invalid_signature_detection()
{
    using namespace std;

    // 构造返回无效签名的转换器,编译期错误
    // constexpr auto invalid_xform = []<class... As>() {
    //     return completion_signatures<set_value_t(As...), int>(); // int不是有效签名
    // };

    // auto cs = completion_signatures<set_value_t(float)>{};

    // 验证转换会触发无效签名检测
    // using result_type = decltype(transform_completion_signatures(cs, invalid_xform));
    // static_assert(is_base_of_v<invalid_completion_signature, result_type>);
}

// 测试5: 附加完成签名合并测试
constexpr void test_additional_completions_merge()
{
    using namespace std;

    auto cs = completion_signatures<set_value_t(int)>{};
    auto other = completion_signatures<set_value_t(double), set_stopped_t()>{};

    auto res = transform_completion_signatures(
        cs, value_transform_default, error_transform_default,
        completion_signatures<>{}, // 默认stopped处理
        other);

    static_assert(res.contains<set_value_t(int)>);
    static_assert(res.contains<set_value_t(double)>);
    static_assert(res.contains<set_stopped_t()>);
    //
    static_assert(
        std::is_same_v<decltype(res),
                       completion_signatures<set_value_t(int), set_value_t(double),
                                             set_stopped_t()>>);
    //
    auto cs2 = res + completion_signatures<set_value_t(int &&)>{};
    static_assert(std::is_same_v<decltype(res), decltype(cs2)>);

    {
        // NOTE: 一般化
        auto res =
            completion_signatures<>{} + completion_signatures<set_value_t(int &&)>{};
        static_assert(
            std::is_same_v<decltype(res), completion_signatures<set_value_t(int)>>);
        // 签名是一致的
        static_assert(MATCHING_SIG<set_value_t(int &&), set_value_t(int)>);

        static_assert(not MATCHING_SIG<set_value_t(int &), set_value_t(int)>);
    }
}

int main()
{
    // 编译时测试
    test_default_transform();
    test_custom_transform();
    test_multiple_signatures_merge();
    // test_invalid_signature_detection();
    test_additional_completions_merge();

    // 运行时输出验证结果
    std::cout << "All static assertions passed!\n";
    return 0;
}
// NOLINTEND