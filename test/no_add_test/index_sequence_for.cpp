#include <iostream>
#include <utility> // for std::index_sequence, std::index_sequence_for

// NOLINTBEGIN
template <class... Env>
static constexpr void check_types()
{
    struct Invoid
    {
    };
    throw Invoid{};
}

template <std::size_t... Is>
constexpr static void test(std::index_sequence<Is...> /*unused*/)
{
    // Use fold expression to print the indices
    ((std::cout << Is << ' '), ...);
    std::cout << '\n';
}

template <std::size_t... Is>
constexpr static void test2(std::index_sequence<Is...> /*unused*/)
{
    // Use fold expression to print the indices
    (check_types<Is>(), ...);
    std::cout << '\n';
}

int main()
{
    using T = std::index_sequence_for<int, double>;
    static_assert(std::is_same_v<T, std::integer_sequence<std::size_t, 0, 1>>); // NOLINT
    using T0 = std::index_sequence<0, 1>;
    static_assert(std::is_same_v<T, T0>);

    // Call test function with the index sequence
    test(T{});

    std::cout << "main done\n";

    test(std::index_sequence_for<>{}); // 没有作用
    test2(std::index_sequence_for<>{});

    // test2(T{}); // 没有在这里爆红，不满足要求的

    return 0;
}
// NOLINTEND