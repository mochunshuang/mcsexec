
#include "../test_common/test_macro.hpp"

int add(int a, int b)
{
    return a + b;
}
int main()
{
    EXPECT(add(1, 2) == 3);
    return 0;
}