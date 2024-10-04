#include <cassert>
#include <iostream>

int main()
{
    struct A
    {
    };
    A obj;
    const A *p = &obj;
    std::cout << "p: " << p << '\n';

    A *ptr = const_cast<A *>(p);
    std::cout << "ptr: " << ptr << '\n';

    assert(p == ptr);

    std::cout << "hello world\n";
    return 0;
}