#include <cassert>
#include <tuple>
#include <type_traits>

int main()
{
    int a = 1;
    auto b = 1.0;

    auto t = std::tuple<int, double>{a, b};
    // Note: std::tie(a, b) 返回一个引用元组，类型为 std::tuple<const int&, const double&>
    static_assert(std::is_same_v<decltype(std::tie(a, b)), std::tuple<int &, double &>>);
    assert(t == std::tie(a, b));
    return 0;
}