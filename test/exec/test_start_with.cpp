#include <version>

#if (__cpp_lib_ranges_starts_ends_with == 202106L)
#include <algorithm>
#include <iostream>
#include <ranges>
#include <string_view>

int main()
{
    using namespace std::literals;

    constexpr auto ascii_upper = [](char8_t c) {
        return u8'a' <= c && c <= u8'z' ? static_cast<char8_t>(c + u8'A' - u8'a') : c;
    };

    constexpr auto cmp_ignore_case = [=](char8_t x, char8_t y) {
        return ascii_upper(x) == ascii_upper(y);
    };

    static_assert(std::ranges::starts_with("const_cast", "const"sv));
    static_assert(std::ranges::starts_with("constexpr", "const"sv));
    static_assert(!std::ranges::starts_with("volatile", "const"sv));

    std::cout << std::boolalpha
              << std::ranges::starts_with(u8"Constantinopolis", u8"constant"sv, {},
                                          ascii_upper, ascii_upper)
              << ' '
              << std::ranges::starts_with(u8"Istanbul", u8"constant"sv, {}, ascii_upper,
                                          ascii_upper)
              << ' '
              << std::ranges::starts_with(u8"Metropolis", u8"metro"sv, cmp_ignore_case)
              << ' '
              << std::ranges::starts_with(u8"Acropolis", u8"metro"sv, cmp_ignore_case)
              << '\n';

    constexpr static auto v = {1, 3, 5, 7, 9};
    constexpr auto odd = [](int x) {
        return x % 2;
    };
    static_assert(std::ranges::starts_with(
        v, std::views::iota(1) | std::views::filter(odd) | std::views::take(3)));
}
#elif
#include <iostream>

int main()
{
    std::cout << "main done\n";
    return 0;
}
#endif