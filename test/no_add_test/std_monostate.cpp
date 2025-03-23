#include <optional>
#include <variant>
#include <iostream>

// NOLINTBEGIN
// 定义一个不可移动和不可复制的类型
struct NonMovableNonCopyable
{
    NonMovableNonCopyable() = default;
    NonMovableNonCopyable(const NonMovableNonCopyable &) = delete;
    NonMovableNonCopyable &operator=(const NonMovableNonCopyable &) = delete;
    NonMovableNonCopyable(NonMovableNonCopyable &&) = delete;
    NonMovableNonCopyable &operator=(NonMovableNonCopyable &&) = delete;
};

int main()
{
    // 定义一个 std::variant，包含 std::monostate 和不可移动/不可复制的类型
    using MyVariant = std::variant<std::monostate, NonMovableNonCopyable, int>;

    // 默认初始化 std::variant
    MyVariant v{};

    // 检查当前持有的类型
    if (std::holds_alternative<std::monostate>(v))
    {
        std::cout << "The variant is default-initialized to std::monostate.\n";
    }

    // NOTE: 可以做函数返回吗？
    auto fun = [] {
        return MyVariant{};
    };
    auto ret = fun(); // 可以

    // NOTE: 作为成员？
    struct A
    {
        MyVariant v;
        int a;
        std::optional<int> error;
    };

    auto a = A{{}}; // OK,等价于： A{.v= {}}
    (void)(a);

    // NOTE: using 可以传递
    auto fun2 = [] {
        struct B
        {
            using type = int;
        };
        return B{};
    };
    using T = decltype(fun2())::type;
    static_assert(std::is_same_v<T, int>);

    // NOTE:  variant +  tuple。 用 variant 构造 tuple。
    {
        // 定义一个 std::variant，包含 std::monostate 和 std::tuple<int, std::string>
        using MyVariant = std::variant<std::monostate, std::tuple<int, std::string>>;

        // 创建一个 std::variant 对象
        MyVariant v;

        // 使用 emplace 在 std::variant 中构造一个 std::tuple
        auto &tuple = v.template emplace<std::tuple<int, std::string>>(42, "Hello");

        // 修改 tuple 中的值
        std::get<0>(tuple) = 100;
        std::get<1>(tuple) = "World";

        // 访问 std::variant 中的 tuple
        if (std::holds_alternative<std::tuple<int, std::string>>(v))
        {
            auto &stored_tuple = std::get<std::tuple<int, std::string>>(v);
            std::cout << "Tuple: (" << std::get<0>(stored_tuple) << ", "
                      << std::get<1>(stored_tuple) << ")\n";
        }
    }

    return 0;
}
// NOLINTEND