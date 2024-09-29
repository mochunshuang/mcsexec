#include <iostream>
#include <utility>

// 不能定义，否则转发失败
// void foo(int x)
// {
//     std::cout << "左值引用: " << x << std::endl;
// }

void foo(int &x)
{
    std::cout << "左值引用: " << x << std::endl;
}

void foo(int &&x)
{
    std::cout << "右值引用: " << x << std::endl;
}

template <typename T>
void wrapper(T &&t)
{
    foo(std::forward<T>(t));
}

int main()
{
    int x = 42;

    // 传递左值
    wrapper(x); // 输出: 左值引用: 42

    // 传递右值
    wrapper(42); // 输出: 右值引用: 42

    // Note: 请仅仅对引用forward。否则可能是错误的来源 std::forward.等同于move
    using T = decltype(std::forward<decltype(x)>(x));
    static_assert(std::is_same_v<T, int &&>);
    static_assert(std::is_same_v<decltype(std::move(x)), int &&>);

    struct only_move
    {
        int age; // NOLINT
        explicit only_move(int age) : age{age} {}

        only_move(const only_move &) = delete;
        only_move &operator=(const only_move &) = delete;
        ~only_move() = default;

        only_move(only_move &&other) noexcept : age(other.age)
        {
            std::cout << "only_move(only_move &&other) \n";
            other.age = 0;
        }

        only_move &operator=(only_move &&other) noexcept
        {
            std::cout << "only_move &operator=(only_move &&other) \n";
            if (this != &other)
            {
                age = other.age;
                other.age = 0;
            }
            return *this;
        }

        auto fun() & // NOLINT
        {
            std::cout << "funcall &\n";
        }
        auto fun() && // NOLINT
        {
            std::cout << "funcall &&\n";
        }
    };
    only_move o(1);
    std::move(o); // 不会调用任何构造函数
    std::forward<decltype(o)>(o);

    // 加上=号，就不一样了
    // only_move(only_move &&other)
    // auto obj = std::forward<decltype(o)>(o);
    std::forward<decltype(o)>(o).fun(); // 不会调用移动构造.但是会调用&&.
    o.fun();                            //&
    /**
     * @brief 总结不要随意对非引用类型，forward；否则是错误的来源
     *
     */

    return 0;
}