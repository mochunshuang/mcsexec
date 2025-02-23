#include <cassert>
#include <iostream>

int main()
{
    {
#if 0
        struct A
        {
            A *pre = nullptr;
            A *next = nullptr;
            int a = 0;
        };

        struct B
        {

            B() = default;
            A *t{};
            ~B() noexcept
            {
                if (t != nullptr)
                {
                    auto n = t->next;
                    assert(n != nullptr);

                    delete t;
                }
            }
        };
        // Note: 引用局部变量，要玩
        B b;
        {
            A a{};
            A c{};
            A obj{};
            obj.pre = &a;
            obj.next = &c;

            b.t = &obj;
        }
#endif
    }
    {
        struct A
        {
            int num;
        };
        A a;
        // assert(a.num == 0);
        std::cout << "a.num == 0: " << (bool)(a.num == 0) << '\n';
    }
    {
        struct A
        {
            int num;
            double s{};
        };
        A a;
        // Note: 未定义行为
        // assert(a.num == 0);
        std::cout << "a.num == 0: " << (bool)(a.num == 0) << '\n';
    }
    {
        struct A
        {
            int num{};
            double s;
        };
        A a;
        // Note: 未定义行为的证明
        // assert(a.s != 0);
        std::cout << "a.s == 0: " << (bool)(a.s == 0) << '\n';

        A *ptr{};
        assert(ptr == nullptr);
    }
    std::cout << "main done\n";
    return 0;
}