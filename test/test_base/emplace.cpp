#include <iostream>
#include <utility>

struct MyType
{
    int a;
    std::string b;

    MyType(int a, std::string b) : a(a), b(std::move(b)) {}

    MyType(const MyType &) = delete;
    MyType(MyType &&) = delete;
    MyType &operator=(const MyType &) = delete;
    MyType &operator=(MyType &&) = delete;
    ~MyType() = default;

    void print() const
    {
        std::cout << "int: " << a << std::endl;
        std::cout << "string: " << b << std::endl;
    }
};

template <typename T>
struct B
{
    T v;
};

/**
 * @brief  variant: 可以运行时动态改变的类型。 tuple 不行
 *
 */
void test_variant();
void test_turple();

int main()
{
    test_variant();
    test_turple();
    std::cout << "hello world\n";
    return 0;
}
void test_variant()
{
    std::cout << "\ntest_variant\n";
    std::variant<int, MyType> var;

    // 使用 emplace 构造一个 MyType，传递构造参数
    var.emplace<MyType>(42, "Hello");

    // 访问 MyType 的成员
    if (std::holds_alternative<MyType>(var))
    {
        MyType &mt = std::get<MyType>(var);
        std::cout << "MyType: a = " << mt.a << ", b = " << mt.b
                  << std::endl; // 输出: MyType: a = 42, b = Hello
    }

    // 使用 emplace 构造一个 int
    var.emplace<int>(42);
    std::cout << "Current value: " << std::get<int>(var) << std::endl; // 输出: 42

    {
        // 还是 variant牛逼
        auto fun = []() {
            return MyType{42, "hello"};
        };
        std::variant<decltype(fun)> var2;
        var2.emplace<decltype(fun)>(fun);
    }
}

/**
 * @brief turple 碰到 没有移动构造的就是死棋。 不可能的，原地构造都做不到
 *
 */
void test_turple()
{
    std::cout << "\nest_turple\n";
    // 创建一个 tuple
    std::tuple<int, std::string> myTuple(42, "Hello, World!");

    // 使用 std::make_from_tuple 构造 MyType 对象
    auto obj = std::make_from_tuple<MyType>(myTuple);

    // 打印 MyType 对象的成员
    obj.print();

    {
        struct A
        {
            std::tuple<MyType> t;

            // 构造函数，使用 std::make_from_tuple 原地构造 MyType
            // A(int a, std::string b) : t(std::make_tuple(MyType(a, std::move(b)))) {}
            // 构造函数，使用 std::tuple 的构造函数原地构造 MyType
            // A(int a, std::string b) : t(std::tuple<MyType>(a, std::move(b))) {}

            // 构造函数，使用 std::tuple 的 emplace 方法原地构造 MyType
            // A(int a, std::string b)
            // {
            //     std::get<0>(t) = MyType(a, std::move(b));
            // }

            // A(int a, std::string b) : t(std::tuple<MyType>(a, std::move(b))) {}
        };

        // std::tuple<MyType> a = std::tuple(42, "Hello, World!");

        // 创建 A 对象，并原地构造 MyType
        // A a(42, "Hello, World!");

        // 访问并打印 MyType 对象的成员
        // std::get<0>(a.t).print();

        // std::tuple<MyType> myTypeTuple(std::make_from_tuple<MyType>(myTuple));
    }
    {

        // std::tuple<MyType> a{MyType{42, "hello"}}; // 不行
        // auto a = std::tuple<MyType>{MyType{42, "hello"}}; // 不行

        // std::tuple<MyType>{MyType{42, "hello"}}; // 不行

        // std::tuple<MyType>{{42, "hello"}}; // 不行

        // 使用 std::make_tuple 创建包含构造参数的 tuple
        // auto params = std::make_tuple(42, "hello");

        // // 使用 std::apply 将参数传递给 MyType 的构造函数
        // auto myTypeTuple = std::apply(
        //     [](auto... args) { return std::tuple<MyType>{MyType{args...}}; }, params);

        // 直接在 std::tuple<MyType> 中构造 MyType 对象
        // std::tuple<MyType> myTypeTuple{MyType{42, "hello"}};

        // // 访问并打印 MyType 对象的成员
        // std::get<0>(myTypeTuple).print();

        // auto a = std::tuple<MyType>{[]() {
        //     return MyType{42, "hello"};
        // }};
    }

    // Note: 以上操作都是不行的。没有移动构造。 死刑，死棋，别想了
    B b{MyType{42, "hello"}}; // OK
    auto a = B{[]() {
        return MyType{42, "hello"};
    }()}; // OK
    B c{[]() {
        return MyType{42, "hello"};
    }()}; // OK

    static_assert(std::is_same_v<decltype(a), decltype(b)>);
    static_assert(std::is_same_v<decltype(a), decltype(c)>);
    static_assert(std::is_same_v<decltype(a), B<MyType>>);

    // std::tuple<B<MyType>> myTypeTuple{B{MyType{42, "hello"}}}; //不可能。 死刑
}