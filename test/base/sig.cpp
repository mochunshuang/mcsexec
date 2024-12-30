#include <iostream>

int main()
{
    auto fun = []() {
        return 1;
    };
    auto fun1 = [](int a) {
        return a + 1;
    };
    using FunReturnType [[maybe_unused]] = std::invoke_result_t<decltype(fun)>;
    // 这个int 是怎么来的呢？能不能直接从 fun1 取出呢？反正目的是获取fun1的返回值类型
    using Fun1ReturnType [[maybe_unused]] = std::invoke_result_t<decltype(fun1), int>;

    std::cout << "hello world\n";
    return 0;
}