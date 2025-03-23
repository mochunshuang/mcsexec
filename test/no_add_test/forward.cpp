// NOLINTBEGIN
#include <iostream>
#include <utility>

class Tracker
{
  public:
    Tracker()
    {
        std::cout << "Default Constructed\n";
    }
    Tracker(const Tracker &)
    {
        std::cout << "Copy Constructed\n";
    }
    Tracker(Tracker &&) noexcept
    {
        std::cout << "Move Constructed\n";
    }
    ~Tracker()
    {
        std::cout << "Destructed\n";
    }
};

/**
 * @brief good
Default Constructed
Move Constructed
Destructed
Destructed
 *
 * @return int
 */
int auto_xx()
{
    // 定义10层lambda调用链，每层使用 auto&& 参数和完美转发
    auto lambda = [](auto &&a) -> decltype(auto) {
        return std::forward<decltype(a)>(a);
    };
    Tracker obj;
    auto result = lambda(lambda(
        lambda(lambda(lambda(lambda(lambda(lambda(lambda(lambda(std::move(obj)))))))))));

    return 0;
}

/**
 * @brief bad
Move Constructed
Move Constructed
Move Constructed
Move Constructed
Move Constructed
Move Constructed
Move Constructed
Move Constructed
Move Constructed
Move Constructed
Move Constructed
 * @return int
 */
int auto_()
{
    // 定义10层lambda调用链，按值传递
    auto lambda = [](auto a) -> decltype(auto) {
        return a;
    };

    Tracker obj;
    auto result = lambda(lambda(
        lambda(lambda(lambda(lambda(lambda(lambda(lambda(lambda(std::move(obj)))))))))));

    return 0;
}
/**
 * @brief bad
Move Constructed
Move Constructed
Move Constructed
Move Constructed
Move Constructed
Move Constructed
Move Constructed
Move Constructed
Move Constructed
Move Constructed
Move Constructed
 * @return int
 */
int auto_auto_xx()
{
    // 定义混合参数的lambda调用链
    auto lambda_forward = [](auto &&a) -> decltype(auto) {
        return std::forward<decltype(a)>(a);
    };
    auto lambda_value = [](auto a) {
        return a;
    };

    Tracker obj;
    auto result = lambda_forward(lambda_value(
        lambda_forward(lambda_value(lambda_forward(lambda_value(lambda_forward(
            lambda_value(lambda_forward(lambda_value(std::move(obj)))))))))));

    return 0;
}

template <typename Sndr>
void test(Sndr &&sndr)
{
    // 结构化绑定 + 完美转发
    std::cout << "bind but not  uesd\n";
    auto &&[tag, data, child] = std::forward<Sndr>(sndr);

    //
    std::cout << "start uesd\n";
    {
        auto new_child = std::forward_like<Sndr>(child); // 此处是否会触发移动构造？
    }
    std::cout << "test end \n";
}

/**
 * @brief good
Default Constructed
Move Constructed
Destructed
Destructed
 * @return int
 */
int main()
{
    // 定义10层全 auto&& 调用链
    auto l1 = [](auto &&a) -> decltype(auto) {
        return std::forward<decltype(a)>(a);
    };
    auto l2 = l1, l3 = l1, l4 = l1, l5 = l1, l6 = l1, l7 = l1, l8 = l1, l9 = l1, l10 = l1;

    Tracker obj;
    auto result = l10(l9(l8(l7(l6(l5(l4(l3(l2(l1(std::move(obj)))))))))));

    // NOTE: auto&& + std::forward<Sndr>(sndr);  不会有任何副作用
    {
        // 创建一个包含 Tracker 的 tuple
        std::tuple<int, std::string, Tracker> sndr(42, "test", Tracker{});

        // 传递左值和右值分别测试
        std::cout << "===== Pass as lvalue =====" << std::endl;
        test(sndr); // 左值传递

        std::cout << "\n===== Pass as rvalue =====" << std::endl;
        test(std::move(sndr)); // 右值传递
    }
    // NOTE: auto && 是万能引用
    {
        auto &&a = 0; // 合法，引用右值

        int v = 1;
        auto &&b = v; // 合法，引用左值

        auto &&c = b; // 合法，引用左值。 "引用是别名"。名字再多也是 v
    }
    return 0;
}
// NOLINTEND