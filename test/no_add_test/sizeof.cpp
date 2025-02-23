#include <iostream>

int main()
{
    {
        int a = 0;
        int *b = &a;
        int **c = &b;
        static_assert(sizeof(a) == sizeof(int));
        static_assert(sizeof(b) == sizeof(int *));
        static_assert(sizeof(c) == sizeof(decltype(c)));

        {
            struct A
            {
                int a{};
                double v{};
            };
            A a = {};
            A *b = &a;
            A **c = &b;
            static_assert(sizeof(a) == sizeof(A));
            static_assert(sizeof(b) == sizeof(decltype(b)));
            static_assert(sizeof(c) == sizeof(decltype(c)));
        }
        {
            char cBuffer[512];
            static_assert(sizeof(cBuffer) == sizeof(decltype(cBuffer)));
            static_assert(sizeof(cBuffer) == 512); // NOLINT
        }
    }
    std::cout << "main done\n";
    return 0;
}