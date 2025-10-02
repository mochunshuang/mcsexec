#include <cstddef>
#include <utility> // 包含std::index_sequence

// 核心模板：正确接收索引序列类型（解决类型不匹配）
template <std::size_t N,      // 第一个字符串长度（含\0）
          std::size_t M,      // 第二个字符串长度（含\0）
          const char (&A)[N], // 第一个字符串
          const char (&B)[M], // 第二个字符串
          typename IndexSeq   // 索引序列类型（如std::index_sequence<0,1,...>）
          >
struct Concat; // 前向声明

// 模板特化：展开索引序列
template <std::size_t N, std::size_t M, const char (&A)[N], const char (&B)[M],
          std::size_t... I // 索引参数包（由IndexSeq展开）
          >
struct Concat<N, M, A, B, std::index_sequence<I...>>
{
    // 编译期拼接数组（长度N+M-1，含\0）
    static constexpr char value[N + M - 1] = {
        // 对每个索引I：
        // 1. 若I < N-1 → 取A[I]（A的有效字符）
        // 2. 若I >= N-1且I < N+M-2 → 取B[I-(N-1)]（B的有效字符）
        // 3. 若I == N+M-2 → 补'\0'（终止符）
        (I < N - 1 ? A[I] : (I == N + M - 2 ? '\0' : B[I - (N - 1)]))...};
};

// ------------------------------
// 测试：编译期拼接两个char[N]
// ------------------------------
// 输入1：char[4]（"abc"+'\0'）
constexpr char str1[] = "abc";
// 输入2：char[4]（"def"+'\0'）
constexpr char str2[] = "def";

// 计算拼接后总长度（4+4-1=7）
constexpr std::size_t TotalLen = 4 + 4 - 1;

// 生成索引序列0~6（共7个索引），传递给Concat模板
using ConcatResult = Concat<4, 4, str1, str2, std::make_index_sequence<TotalLen>>;

// 获取拼接结果（char[7]类型）
constexpr const char (&concat_str)[TotalLen] = ConcatResult::value;

// ------------------------------
// 编译期验证（全部通过）
// ------------------------------
static_assert(concat_str[0] == 'a', "首字符错误");     // 验证第一个字符
static_assert(concat_str[5] == 'f', "有效字符尾错误"); // 验证最后一个有效字符
static_assert(concat_str[6] == '\0', "缺少终止符\\0"); // 验证终止符
static_assert(sizeof(concat_str) == 7, "总长度应为7"); // 验证总长度
static_assert(__builtin_memcmp(concat_str, "abcdef", 6) == 0, "拼接内容错误"); // 整体验证

int main()
{
    return 0; // 编译通过即证明拼接成功
}
