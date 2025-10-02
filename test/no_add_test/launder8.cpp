#include <cassert>
#include <iostream>

// NOLINTBEGIN
struct interface
{
    auto type(this auto &&self)
    {
        return self.impl_type();
    }
};
struct int_type : interface
{
    auto impl_type()
    {
        return int{0};
    }
};

int main()
{
    int_type i;
    static_assert(std::is_same_v<int, decltype(i.type())>);
    assert(i.type() == 0);

    interface *base = &i;
    // assert(base->type() == 0); // Runtime type missing, compile time error

    std::cout << "main done\n";
    return 0;
}
// NOLINTEND