
#include <boost/ut.hpp>

int add(int a, int b)
{
    return a + b;
}
int main()
{
    boost::ut::expect(add(1, 2) == 3);
    return 0;
}