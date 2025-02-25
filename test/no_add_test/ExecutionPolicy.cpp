#include <algorithm>
#include <cstdint>
#include <execution>
#include <iostream>
#include <stdint.h>
#include <vector>
#include <chrono>
#include <ranges>

// 定义一个简单的 lambda 函数
auto lambda = []() {
    int dummy = 0; // 防止编译器优化
    ++dummy;
    (void)dummy;
};

// 测试 for 循环的性能
void test_for_loop()
{
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < 10000000; ++i)
    {
        lambda();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "For loop time: " << elapsed.count() << " seconds\n";
}

// 测试 while 循环的性能
void test_while_loop()
{
    constexpr int N = 10000000; // 减少循环次数但增大单次计算量
    constexpr auto range = std::views::iota(0, N);
    auto begin = range.begin();
    auto end = range.end();

    auto start = std::chrono::high_resolution_clock::now();

    while (begin != end)
    {
        lambda();
        ++begin;
    }
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end_time - start;
    std::cout << "While loop time: " << elapsed.count() << " seconds\n";
}

// 测试 std::for_each + std::execution::par 的性能
void test_for_each_par()
{

    constexpr int N = 10000000; // 减少循环次数但增大单次计算量
    constexpr auto range = std::views::iota(0, N);

    auto start = std::chrono::high_resolution_clock::now();

    std::for_each(std::execution::par, range.begin(), range.end(),
                  [&](int) { lambda(); });

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "std::for_each + par time: " << elapsed.count() << " seconds\n";
}

/**
 * @brief 简单的for 循环，反而最好
 *
 * @return int
 */
int main()
{
    std::cout << "Testing performance for 10,000,000 operations:\n";

    test_for_loop();
    test_while_loop();
    test_for_each_par();

    {
        const int begin = 0;      // NOLINT
        const int end = 10000000; // NOLINT

        // 测试 for 循环
        auto start_for = std::chrono::high_resolution_clock::now();
        {
            [[maybe_unused]] std::uint64_t value = 0;

            for (auto i = begin; i != end; ++i)
            {
                // 空操作
                value++;
            }
        }
        auto end_for = std::chrono::high_resolution_clock::now();
        auto duration_for =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_for - start_for)
                .count();

        // 测试 while 循环
        auto start_while = std::chrono::high_resolution_clock::now();
        {
            [[maybe_unused]] std::uint64_t value = 0;

            auto i = begin;
            while (i != end)
            {
                value++;
                ++i;
            }
            // for (auto i = begin; i != end; ++i)
            // {
            //     // 空操作
            //     value++;
            // }
        }
        auto end_while = std::chrono::high_resolution_clock::now();
        auto duration_while =
            std::chrono::duration_cast<std::chrono::milliseconds>(end_while - start_while)
                .count();

        // 输出结果: 多次发现，擦不多
        std::cout << "for 循环耗时: " << duration_for << " 毫秒\n";
        std::cout << "while 循环耗时: " << duration_while << " 毫秒\n";
    }
    // Note: 性能开销：
    // 对于简单的操作（如输出），并行化可能不会带来性能提升，甚至可能增加开销。

    return 0;
}