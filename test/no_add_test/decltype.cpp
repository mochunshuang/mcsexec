#include <cassert>
#include <variant>
#include <type_traits>

template <typename T>
struct decayed_tuple
{
};

int main()
{
    bool has_epr = false;

    // Lambda 捕获局部变量 has_epr
    auto as_tuple = [&](auto pf) {
        using Sig = decltype(pf);
        if constexpr (std::is_same_v<Sig, int (*)(float)>)
        {
            has_epr = true; // 这行代码实际永远不会执行
        }
        return decayed_tuple<Sig>{};
    };

    // 在 decltype 中使用 lambda（不会实际执行 lambda 体）
    using T = std::variant<std::monostate,
                           decltype(as_tuple(static_cast<int (*)(float)>(nullptr))) // OK
                           >;

    static_assert(
        std::is_same_v<T, std::variant<std::monostate, decayed_tuple<int (*)(float)>>>);

    assert(has_epr == false);
    return 0;
}