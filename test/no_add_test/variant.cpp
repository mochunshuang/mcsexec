#include <iostream>
#include <variant>

int main()
{
    using T = std::variant<std::monostate, int, int>;
    using T0 = std::variant<std::monostate, int>;
    static_assert(not std::is_same_v<T, T0>);
    std::cout << "main done\n";
    return 0;
}