#include <print>

int main()
{
    int age = 25;         // NOLINT
    double height = 1.75; // NOLINT
    std::print("I am {} years old and {:.2f} meters tall.\n", age, height);
    // 输出: I am 25 years old and 1.75 meters tall.
    std::println();
    std::print("...\n");
    return 0;
}