#include <algorithm>
#include <cassert>
#include <iostream>
#include <string_view>
#include <type_traits>
#include <utility>
#include <random>

class MyString
{
  public:
    // 默认构造函数
    MyString() : data(nullptr), length(0)
    {
        std::cout << "MyString()" << std::endl;
    }

    // 带参数的构造函数
    explicit MyString(std::string_view str)
        : data(new char[str.size() + 1]), length(str.size())
    {
        std::copy(str.begin(), str.end(), data);
        data[length] = '\0'; // 确保字符串以 null 结尾
        std::cout << "explicit MyString(): " << data << std::endl;
    }

    // 拷贝构造函数
    MyString(const MyString &other)
        : data(new char[other.length + 1]), length(other.length), copyed(true) // Note:
    {
        std::copy(other.data, other.data + other.length, data);
        data[length] = '\0'; // 确保字符串以 null 结尾

        std::cout << "MyString(const MyString &other): " << data << std::endl;
    }

    // 移动构造函数
    MyString(MyString &&other) noexcept : data(other.data), length(other.length)
    {
        other.data = nullptr;
        other.length = 0;
        std::cout << "MyString(MyString &&other): " << data << std::endl;
    }

    // 拷贝赋值运算符
    MyString &operator=(const MyString &other)
    {
        if (this != &other)
        {
            delete[] data; // 释放原有资源
            data = new char[other.length + 1];
            std::copy(other.data, other.data + other.length, data);
            data[other.length] = '\0';
            length = other.length;

            // Note:
            copyed = true;
            std::cout << "operator=(const MyString &other): " << data << std::endl;
        }
        return *this;
    }

    // 移动赋值运算符
    MyString &operator=(MyString &&other) noexcept
    {
        if (this != &other)
        {
            delete[] data; // 释放原有资源
            data = other.data;
            length = other.length;
            other.data = nullptr;
            other.length = 0;
            std::cout << "operator=(MyString &&other): " << data << std::endl;
        }
        return *this;
    }

    // 析构函数
    ~MyString() noexcept
    {
        delete[] data;
        std::cout << "~MyString()" << std::endl;
    }

    // 打印函数
    void print() const
    {
        if (data != nullptr)
        {
            std::cout << "print: " << data << std::endl;
        }
        else
        {
            std::cout << "print: (null)" << std::endl;
        }
    }

  private:
    char *data;    // NOLINT
    size_t length; // NOLINT

  public:
    bool copyed = false; // NOLINT
};

template <typename T>
decltype(auto) fun_fwd_0(T &&data)
{
    return std::forward<T>(data); // Note: 一样是bug
}
/**
 * @brief Note： 这是错误的的来源
 *
 * @tparam T
 * @param data
 * @return auto
 */
template <typename T>
auto fun_fwd_1(const T &data) // Note： 这是错误的的来源。无论是
{
    // e:\0_github_project\mcsexec\mcsexec\test\test_base\Extend.cpp:112:27: error:
    // binding reference of type 'std::remove_reference<MyString>::type&' {aka
    // 'MyString&'} to 'const MyString' discards qualifiers

    // return std::forward<T>(data);
    // invalid 'static_cast' from type 'const MyString' to type 'MyString&&'
    // return static_cast<T &&>(data);

    // return static_cast<T &&>();
    // return T{data}; // Note: 返回值是 无法优化的。 Bug的来源。 一定发生复制
    return data;
}

// 函数1：使用 const 引用延长临时对象的生命周期
void printMyStringConstRef(const MyString &str)
{
    assert(not str.copyed);
    std::cout << "\tConst Ref: start";
    str.print();
}

// 函数2：使用右值引用延长临时对象的生命周期
void printMyStringRvalueRef(MyString &&str)
{
    assert(not str.copyed);
    std::cout << "\tRvalue Ref: ";
    str.print();
}

// 函数3：验证 lvalue 引用
void printMyStringLvalueRef(MyString &str)
{
    assert(not str.copyed);
    std::cout << "\tLvalue Ref: ";
    str.print();
}

void printMyString(MyString str)
{
    assert(str.copyed); // Note: copy
    std::cout << "\tLvalue Ref: ";
    str.print();
}

template <typename T>
void fun_fwd(T &&data, int count)
{
    // end
    if (count == 0)
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 1);
        if (dis(gen) == 0)
        {
            std::cout << "end 0: \n";
            printMyStringConstRef(std::forward<T>(data));
        }
        else
        {
            std::cout << "end 1: \n";
            printMyStringRvalueRef(std::forward<T>(data));
        }
        // Note: 最后使用的时候，&& => & 的参数是可以的
        printMyStringLvalueRef(data);
        printMyString(data);
        return;
    }

    fun_fwd(fun_fwd_0(std::forward<T>(data)), --count);

    // Note: const T&，会发生复制。 好像也没有返回值优化. 失效了
#if 0
    if (count % 2 == 0)
    {
        fun_fwd(fun_fwd_0(std::forward<T>(data)), --count);
    }
    else
    {
        fun_fwd(fun_fwd_1(std::forward<T>(data)), --count);
    }
#endif
}

template <typename T>
decltype(auto) test(T &&data)
{
    return std::forward<T>(data);
}
template <typename T>
void recursion_fwd(T &&data, int count)
{
    // std::cout << "recursion_fwd: " << count << "\n";
    if (count == 0)
    {
        printMyStringRvalueRef(std::forward<T>(data));
        printMyStringConstRef(std::forward<T>(data));
        // Note: 确实是延长声明周期了。居然能访问 &
        printMyStringLvalueRef(data);
        return;
    }
    // Note: bug的来源： count--。 返回的是旧值count
    recursion_fwd(std::forward<T>(data), --count);
}

int main()
{
    /**
     * @brief T&& => const T& ,是没有复制的
     *
     */
    // 1. 使用 const 引用绑定到临时对象
    printMyStringConstRef(MyString("Hello, Const Ref!"));

    // 2. 使用右值引用绑定到临时对象
    printMyStringRvalueRef(MyString("Hello, Rvalue Ref!"));

    // 3. 验证 lvalue 引用
    MyString lvalueStr = MyString("Hello, Lvalue Ref!");
    printMyStringLvalueRef(lvalueStr);
    printMyStringConstRef(lvalueStr); // Note: T => const T&是可以的

    std::cout << "\ntest: const T && start\n";
    const MyString k_lvalue_str = MyString("Hello, const T!");
    // printMyStringLvalueRef(k_lvalue_str);//Note: const T => T& 是非法的
    // printMyStringRvalueRef(std::move(k_lvalue_str)); // Note: 非法。 不会生成 T&&
    using T = decltype(std::move(k_lvalue_str));
    static_assert(std::is_same_v<const MyString &&, T>);
    [](const MyString &&a) {
        return a;
    }(std::move(k_lvalue_str));
    // Note: [const MyString && => const MyString &] 会copy
    std::cout << "\nconst MyString &a : start\n";
    [](const MyString &a) {
        return a; // copy
    }(std::move(k_lvalue_str));
    std::cout << "\ntest: const T && end\n";
    /**
     * @brief 总之 move const变量是不可能的，会copy
     *
     */

// Note: 编译期错误
// printMyStringLvalueRef(MyString("Hello, Lvalue Ref!"));
// printMyStringLvalueRef(std::move(lvalueStr));

// test //Note: 扩展生命周期： 增加右值的声明周期
#if 1
    std::cout << "test: fun_fwd start";
    fun_fwd(MyString("fun_fwd 8!"), 8);
    fun_fwd(MyString("fun_fwd 9"), 9);
    fun_fwd(MyString("fun_fwd 10"), 10);
    std::cout << "test: fun_fwd end";
#endif
    // Note: 测试递归：const& T出来的，forword 不了。会发生复制
    std::cout << "\ntest: recursion_fwd start\n";
    recursion_fwd(MyString("fun_fwd 8!"), 8);       // NOLINT
    recursion_fwd(MyString("fun_fwd 9!"), 9);       // NOLINT
    recursion_fwd(MyString("fun_fwd 1000!"), 1000); // NOLINT

    /**
     * @brief 总结只有右值，才不会发生复制。 最后&& => const T&,T&&, &, 都可以
     *
     */

    std::cout << "\ntest: const T &data\n";
    auto fun = []<typename T>(const T &data) {
        return data;
    };
    MyString t("[]<typename T>(const T &data)!");

    std::cout << "\ntest: const T &data start\n";
    auto data = fun(t);
    std::cout << "\ntest: const T &data end\n";
    /**
     * @brief 总结： const T &，没有返回值优化。 会发生复制
     *
     */
    assert(data.copyed);

    {
        std::cout << "\ntest: const T &data ============== \n";
        auto fun = []<typename T>(const T &data) -> decltype(auto) {
            return data;
        };
        MyString t("[]<typename T>(const T &data)! -> decltype(auto)");

        std::cout << "\ntest: const T &data start\n";
        decltype(auto) data = fun(t); // {}内一次析构
        // auto data2 = fun(t); // {}内两次析构，Note: 原因是外部data2 加了一次析构.
        // 发生复制

        /**
         * @brief 总结： decltype(auto) + dada 返回值能避免重复。 auto +const T &
         * 总会发生复制
         *  decltype(auto) 用 auto 接收，发送复制
         *
         */
        assert(not data.copyed);
        // assert(data2.copyed);

        std::cout << "\ntest: const T &data end\n";
    }

    {
        MyString t("[]<typename T>(const T &data)! -> decltype(auto)");

        // move
        std::cout << "\ntest: data = std::forward<decltype(t)>(t) \n";
        // move
        auto fun = [data = std::forward<decltype(t)>(t)]() mutable {
            return std::move(data);
        };

        // auto data = fun();
        // assert(not data.copyed);

        // Note: 这个包装，又调用了一次 MyString的move 增加 一次析构
        // Note: 返回值优化，不会增加复制
        decltype(auto) data = [&]() -> decltype(auto) {
            return fun();
        }();
        assert(not data.copyed);

        std::cout << "\ntest: data = std::forward<decltype(t)>(t) end\n";
        /**
         * @brief 构造函数的次数 和 析构函数的次数 不一定相等
         *
         */
    }

    std::cout << "\n main done\n";
    return 0;
}