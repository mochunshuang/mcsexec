
#include "../test_common/test_macro.hpp"
#include <expected>
#include <iostream>

int main()
{

    TEST("auto test") = [] {
        std::cout << " auto test\n";
    };

    return 0;
}