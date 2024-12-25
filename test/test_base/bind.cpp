#include <cassert>
#include <iostream>

auto test_bind()
{
    struct A
    {
        int age;
        std::string name;
    };
    return A{1, "name"};
}

struct A
{
    int age;
    std::string name;
};
auto test_bind_2() -> A;

int main()
{
    {
        auto [age, name] = test_bind();
        assert(age == 1);
        assert(name == "name");
    }
    {
        auto [age, name] = test_bind_2();
        assert(age == 1);
        assert(name == "name");
    }
    std::cout << "hello world\n";
    return 0;
}
auto test_bind_2() -> A
{
    return {1, "name"};
}
