#include <iostream>
#include <utility>
#include <string>

template <typename T, typename U>
class B
{
    T m_t;
    U m_u;

  public:
    B(T &&t, U &&u) noexcept : m_t(std::forward<T>(t)), m_u(std::forward<U>(u))
    {
        std::cout << "Constructed with: " << m_t << ", " << m_u << std::endl;
    }
};

template <typename T, typename U>
class B2
{

    T m_t;
    U m_u;

  public:
    template <typename T1, typename U1>
    B2(T1 &&t, U1 &&u) noexcept : m_t(std::forward<T1>(t)), m_u(std::forward<U1>(u))
    {
        std::cout << "Constructed with: " << m_t << ", " << m_u << std::endl;
    }
};
template <typename T1, typename U1>
B2(T1 &&, U1 &&) -> B2<typename std::decay<T1>::type, typename std::decay<U1>::type>;

int main()
{
    int x = 42;
    std::string str = "Hello";

    // 使用左值构造
    B<int &, std::string &> b1(x, str);

    // 使用右值构造
    B<int, std::string> b2(42, "World");
    /**
     * @brief 缺点是。 对于引用，不占用一份
     *
     */
    {
        // 非法的
        // B<int, std::string> b1(x, str);

        int x = 42;
        std::string str = "Hello";

        // 使用左值构造
        B2<int &, std::string &> b1(x, str);

        // 使用右值构造
        B2<int, std::string> b2(42, "World");

        // 使用左值构造，但确保独占一份
        B2<int, std::string> b3(x, str);
    }

    {
        /**
         * @brief T && 修饰参数。说明T是通用类型参数
         *
         */
        auto fun = []<typename T>(T &&t) -> T {
            return t;
        };
        int x = 42;
        std::string str = "Hello";
        int &rx = x;
        auto &rstr = str;
        {
            using T0 = decltype(fun(42));
            using T1 = decltype(fun(x));
            using T2 = decltype(fun(rx));
            using T3 = decltype(fun(std::move(x)));

            static_assert(std::is_same_v<T0, int>);
            static_assert(std::is_same_v<T0, T3>);
            static_assert(std::is_same_v<T1, int &>);
            static_assert(std::is_same_v<T1, T2>);
        }
        {
            using T0 = decltype(fun("Hello"));
            using T1 = decltype(fun(str));
            using T2 = decltype(fun(rstr));
            using T3 = decltype(fun(std::move(str)));
            static_assert(std::is_same_v<T0, const char(&)[6]>);
            static_assert(not std::is_same_v<T0, T3>);

            static_assert(std::is_same_v<T1, std::string &>);
            static_assert(std::is_same_v<T1, T2>);
            static_assert(std::is_same_v<T3, std::string>);
        }
    }

    return 0;
}