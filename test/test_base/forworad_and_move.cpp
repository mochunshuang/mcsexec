#include <iostream>
#include <type_traits>
#include <utility>

// 定义 Type 类
class Type
{
  public:
    // 默认构造函数
    Type() : value(42), valid(true) {}
    explicit Type(int v) : value(v), valid(true) {}
    ~Type()
    {
        std::cout << "~Type(): ";
        printInfo();
    }

    // 移动构造函数
    Type(Type &&other) noexcept : value(other.value), valid(other.valid)
    {
        std::cout << "Type(Type &&other) noexcept: ";
        other.valid = false; // 移动后将源对象标记为无效
    }

    // 移动赋值运算符
    Type &operator=(Type &&other) noexcept
    {
        if (this != &other)
        {
            std::cout << "Type &operator=(Type &&other) noexcept: ";
            value = other.value;
            valid = other.valid;
            other.valid = false; // 移动后将源对象标记为无效
        }
        return *this;
    }

    // 打印信息
    void printInfo() const
    {
        std::cout << "Value: " << value << ", Valid: " << (valid ? "true" : "false")
                  << std::endl;
    }

  private:
    int value;
    bool valid;
};

// 假设 bar 函数接受 Type 类型的参数
void bar(Type &&obj)
{
    // 这里可以对 obj 进行操作，但不会使其处于未定义状态
    std::cout << "bar called with: ";
    obj.printInfo();
}

void bar_2(const Type &obj)
{
    std::cout << "bar_2 called with: ";
    obj.printInfo();
}

template <typename T>
void foo(T &&data)
{
    static_assert(std::is_rvalue_reference_v<decltype(data)>);

    // 使用 std::forward 进行完美转发
    bar(std::forward<T>(data));

    // 输出 data 的信息
    std::cout << "After bar, data is: ";
    data.printInfo();

    bar_2(std::forward<T>(data));
} // 这里，才释放 data

template <typename T>
void life_time(T &&data)
{
    std::cout << "life_time(T &&data): start \n";
    std::cout << "\ndata before: forward \n";
    data.printInfo();
    [](T &&data) {
        return data;
    }(std::forward<T>(data));
    std::cout << "data after: forward \n";
    data.printInfo();
    std::cout << "\n ";
    /**
     * @brief 如果 forward 变成 move；【最好forward之后，就不能使用了】
     *
     */
    // Note: 小心 std::forward<T> => std::move

    std::cout << "life_time(T &&data): end \n";
}
int main()
{
    Type t; // Note: 声明，还是初始化，说不清楚
    foo(std::move(t));
    t.printInfo();
    /**
     * @brief note， 以上不会调用 移动构造函数。 因此都是Valid: true
     *
     */
    std::cout << "\n ";
    {
        Type t;
        auto other = std::move(t); // 调用 移动构造函数
        t.printInfo();
        other.printInfo();
    }
    std::cout << "\n ";
    {
        Type t(2);
        foo(std::move(t));
        t.printInfo();
    }
    std::cout << "\n";
    {
        std::cout << "life_time: start outside \n";
        life_time(Type{});
        std::cout << "life_time: end outside \n";
    }
    return 0;
}