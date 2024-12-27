#include <algorithm>
#include <iostream>
#include "../include/execution.hpp"

#include <iostream>
#include <format>

#include <iostream>

void base();
int main()
{
    base();
    std::cout << "hello world\n";
    return 0;
}
void base()
{
    using namespace mcs::execution;
    using namespace std::chrono_literals;

    sender auto input = just();
    sender auto multi_shot = split(input);

    // 增加延迟的任务
    sender auto task1 = then(multi_shot, [] {
        auto start = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(2s); // 延迟 2 秒
        auto end = std::chrono::steady_clock::now();
        std::cout
            << "First continuation completed in "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
            << " ms\n";
    });

    sender auto task2 = then(multi_shot, [] {
        auto start = std::chrono::steady_clock::now();
        std::this_thread::sleep_for(1s); // 延迟 1 秒
        auto end = std::chrono::steady_clock::now();
        std::cout
            << "Second continuation completed in "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
            << " ms\n";
    });

    // 使用 when_all 组合任务
    sender auto both = when_all(task1, task2);

    // 记录 when_all 的开始时间
    auto when_all_start = std::chrono::steady_clock::now();
    mcs::this_thread::sync_wait(std::move(both));
    auto when_all_end = std::chrono::steady_clock::now();

    // 输出 when_all 的完成时间
    std::cout << "when_all completed in "
              << std::chrono::duration_cast<std::chrono::milliseconds>(when_all_end -
                                                                       when_all_start)
                     .count()
              << " ms\n";
}