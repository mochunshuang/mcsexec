// NOLINTBEGIN
#include <variant>
#include <functional>
#include <utility>
#include <cassert>

//---------------------- 基础类型定义 ----------------------
struct Env
{
}; // 示例环境类型

template <typename Sndr, typename Rcvr>
struct basic_operation
{
    // 不可移动/复制
    basic_operation(const basic_operation &) = delete;
    basic_operation(basic_operation &&) = delete;
    basic_operation &operator=(const basic_operation &) = delete;
    basic_operation &operator=(basic_operation &&) = delete;
    ~basic_operation() = default;

    // 构造函数
    basic_operation(Sndr sndr, Rcvr rcvr) : sndr_(std::move(sndr)), rcvr_(std::move(rcvr))
    {
    }

    // 示例成员函数
    void start() & noexcept
    { /* 操作逻辑 */
    }

    Sndr sndr_;
    Rcvr rcvr_;
};

//---------------------- 连接函数模拟 ----------------------
namespace conn
{
    template <typename Sndr, typename Rcvr>
    auto connect(Sndr &&sndr, Rcvr &&rcvr) -> basic_operation<Sndr, Rcvr>
    {
        return {std::forward<Sndr>(sndr), std::forward<Rcvr>(rcvr)};
    }
} // namespace conn

namespace opstate
{
    template <typename O>
    constexpr void start(O &&o) noexcept
        requires(not std::is_rvalue_reference_v<decltype(o)>) && requires(O &o) {
            { o.start() } noexcept -> std::same_as<void>;
        }
    {
        o.start();
    }
} // namespace opstate

//---------------------- emplace_from 包装器 ----------------------
namespace snd::general
{
    template <typename Fun>
    struct emplace_from
    {
        Fun fun;
        using type = std::invoke_result_t<Fun>;

        // 关键转换：触发构造
        operator type() && noexcept(noexcept(std::declval<Fun &&>()()))
        {
            return std::move(fun)();
        }
    };
} // namespace snd::general

//---------------------- 测试用例 ----------------------
int main()
{
    // 定义模拟的 Sender 和 Receiver
    struct Sender
    {
        int id = 42;
    };
    struct Receiver
    {
        Env env{};
    };

    Sender result_sender;
    Receiver rcvr2;

    // 构造 lambda 生成器（捕获引用）
    auto mkop2 = [&]() noexcept {
        return conn::connect(std::move(result_sender), std::move(rcvr2));
    };

    // 定义 variant 类型（必须提前知道所有可能类型）
    using OpVariant = std::variant<std::monostate, basic_operation<Sender, Receiver>>;

    OpVariant ops2;

    // 关键步骤：原地构造不可移动对象
    auto &op2 = ops2.emplace<decltype(mkop2())>(
        snd::general::emplace_from<decltype(mkop2)>{std::move(mkop2)});
    opstate::start(op2);

    // 验证构造结果
    assert(ops2.index() == 1);          // 确认存储的是 basic_operation
    assert(&std::get<1>(ops2) == &op2); // 确认引用一致性

    // 测试 noexcept 属性
    static_assert(noexcept(std::declval<snd::general::emplace_from<decltype(mkop2)>>()
                               .operator basic_operation<Sender, Receiver>())); // 应通过

    // 生命周期验证（确保捕获的引用有效）
    assert(op2.sndr_.id == 42); // 假设 basic_operation 公开成员
}

// NOLINTEND