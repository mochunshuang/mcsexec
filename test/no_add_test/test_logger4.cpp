#include <iostream>
#include <utility>
#include <string_view>
// NOLINTBEGIN
// ------------------------------
// Concat模板：编译期拼接两个const char[N]数组
// 输入：A(长度N)、B(长度M)，均为带'\0'的C字符串
// 输出：拼接后数组(长度N+M-1)，含A的有效字符+B的有效字符+'\0'
// ------------------------------
template <std::size_t N,      // 第一个数组长度（含'\0'）
          std::size_t M,      // 第二个数组长度（含'\0'）
          const char (&A)[N], // 第一个数组（静态存储期）
          const char (&B)[M], // 第二个数组（静态存储期）
          typename IndexSeq   // 索引序列类型（内部使用）
          >
struct Concat;

// 模板特化：展开索引序列实现拼接
template <std::size_t N, std::size_t M, const char (&A)[N], const char (&B)[M],
          std::size_t... I // 索引参数包（0 ~ N+M-2）
          >
struct Concat<N, M, A, B, std::index_sequence<I...>>
{
    static constexpr char value[N + M - 1] = {
        // 索引逻辑：
        // 1. I < N-1 → 取A的有效字符（跳过A的'\0'）
        // 2. I >= N-1且I < N+M-2 → 取B的有效字符（跳过B的'\0'）
        // 3. I == N+M-2 → 补终止符'\0'
        (I < N - 1 ? A[I] : (I == N + M - 2 ? '\0' : B[I - (N - 1)]))...};
};

// ------------------------------
// 辅助函数：简化两个数组的拼接调用
// ------------------------------
template <std::size_t N, std::size_t M, const char (&A)[N], const char (&B)[M]>
constexpr const char (&concat_two())[N + M - 1]
{
    using IndexSequence = std::make_index_sequence<N + M - 1>;
    return Concat<N, M, A, B, IndexSequence>::value;
}

// ------------------------------
// 核心函数：基于Concat模板拼接三个数组
// ------------------------------
template <std::size_t FmtLen,         // 格式串长度（含'\0'）
          std::size_t PreLen,         // 前缀长度（含'\0'）
          std::size_t PostLen,        // 后缀长度（含'\0'）
          const char (&Fmt)[FmtLen],  // 格式串（静态数组）
          const char (&Pre)[PreLen],  // 前缀（静态数组）
          const char (&Post)[PostLen] // 后缀（静态数组）
          >
constexpr std::string_view test()
{
    // 步骤1：拼接前缀 + 格式串
    constexpr auto &pre_fmt = concat_two<PreLen, FmtLen, Pre, Fmt>();
    constexpr std::size_t Len1 = PreLen + FmtLen - 1; // 中间结果长度

    // 步骤2：拼接中间结果 + 后缀
    constexpr std::size_t TotalLen = Len1 + PostLen - 1; // 最终结果长度
    using FinalIndexSeq = std::make_index_sequence<TotalLen>;
    constexpr auto &final_result =
        Concat<Len1, PostLen, pre_fmt, Post, FinalIndexSeq>::value;

    // 返回字符串视图（不含终止符）
    return std::string_view(final_result, TotalLen - 1);
}

// ------------------------------
// 测试数据：静态constexpr数组（存储字符串字面量）
// 解决"字符串字面量不能直接作为模板参数"问题
// ------------------------------
constexpr char str_fmt[] = "a";  // 类型：const char[2]（含'\0'）
constexpr char str_pre[] = "b";  // 类型：const char[2]
constexpr char str_post[] = "c"; // 类型：const char[2]

namespace AnsiColor
{
    constexpr char reset[] = {"\033[0m"};
    constexpr char red[] = {"\033[31m"};
    constexpr char green[] = {"\033[32m"};
    constexpr char yellow[] = {"\033[33m"};
    constexpr char blue[] = {"\033[34m"};
    constexpr char magenta[] = {"\033[35m"};
    constexpr char cyan[] = {"\033[36m"};
    constexpr char bold[] = {"\033[1m"};
} // namespace AnsiColor

// ------------------------------
// 主函数：验证编译期拼接结果
// ------------------------------
int main()
{
    // 1. 非constexpr场景调用
    auto fmt1 = test<sizeof(str_fmt),  // FmtLen = 2
                     sizeof(str_pre),  // PreLen = 2
                     sizeof(str_post), // PostLen = 2
                     str_fmt, str_pre, str_post>();
    std::cout << "非constexpr结果：" << fmt1 << std::endl; // 输出：bac

    // 2. constexpr场景调用（编译期验证）
    {
        constexpr auto fmt2 = test<sizeof(str_fmt), sizeof(str_pre), sizeof(str_post),
                                   str_fmt, str_pre, str_post>();

        // 编译期断言验证拼接结果
        static_assert(fmt2 == "bac", "拼接内容错误！");
        static_assert(fmt2.size() == 3, "拼接长度错误！");

        std::cout << "constexpr结果：" << fmt2 << std::endl; // 输出：bac
    }
    {
        static constexpr char str_fmt2[] = "a"; // NOTE: 需要static
        constexpr auto fmt2 =
            test<sizeof(str_fmt2), sizeof(AnsiColor::blue), sizeof(AnsiColor::reset),
                 str_fmt2, AnsiColor::blue, AnsiColor::reset>();
    }

    std::cout << "main done" << std::endl;
    return 0;
}
// NOLINTEND