#include <cassert>

class MyClass
{
  public:
    static constexpr int value = 42; // NOLINT
};

int main()
{
    static_assert(sizeof(MyClass) == 1);
    return 0;
}