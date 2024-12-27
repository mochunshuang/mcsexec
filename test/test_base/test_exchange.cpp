#include <atomic>
#include <iostream>
#include <utility>

enum class disposition
{
    started, // NOLINT
    error,   // NOLINT
    stopped  // NOLINT
};

/**
 * @brief
 * Note: exchange 总 返回旧值
 * @return int
 */
int main()
{
    std::atomic<disposition> state{disposition::started};

    // 使用 exchange 设置新值，并获取旧值
    disposition old_state = state.exchange(disposition::error);

    // 输出结果
    std::cout << "Old state: " << static_cast<int>(old_state) << "\n";
    std::cout << "New state: " << static_cast<int>(state.load()) << "\n";

    {
        // 使用 relaxed 内存顺序
        [[maybe_unused]] disposition old_state =
            state.exchange(disposition::error, std::memory_order_relaxed);
    }

    // Note: 非原子操作的exchange
    {
        int x = 10; // NOLINT

        // 使用 std::exchange 替换 x 的值，并保存旧值
        int old_x = std::exchange(x, 20); // NOLINT

        std::cout << "Old value of x: " << old_x << "\n"; // 输出 10
        std::cout << "New value of x: " << x << "\n";     // 输出 20
    }

    return 0;
}