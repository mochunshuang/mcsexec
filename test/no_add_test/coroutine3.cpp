#include <coroutine>
#include <chrono>
#include <iostream>
#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>

// NOLINTBEGIN
// 协程的挂起控制
struct suspend_always
{
    bool await_ready() const noexcept
    {
        return false;
    }
    void await_suspend(std::coroutine_handle<>) const noexcept {}
    void await_resume() const noexcept {}
};

// 协程定义
struct Coro
{
    struct promise_type
    {
        // 关键：通过 from_promise 绑定协程句柄
        Coro get_return_object()
        {
            return Coro{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        suspend_always initial_suspend()
        {
            return {};
        } // 初始挂起
        suspend_always final_suspend() noexcept
        {
            return {};
        }
        void return_void() {}
        void unhandled_exception() {}
    };
    std::coroutine_handle<promise_type> handle; // 管理协程句柄
};

Coro test_coroutine()
{
    while (true)
    {
        co_await suspend_always{}; // 每次恢复后立即挂起
    }
}

int main()
{
    auto coro = test_coroutine();
    auto handle = coro.handle;

    // 首次启动协程，进入挂起状态
    if (!handle.done())
        handle.resume();

    constexpr int num_tests = 10000;
    std::vector<double> times;
    times.reserve(num_tests);

    for (int i = 0; i < num_tests; ++i)
    {
        auto start = std::chrono::high_resolution_clock::now();
        handle.resume();
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::nano> elapsed = end - start;
        times.push_back(elapsed.count());
    }

    // 销毁协程避免内存泄漏
    handle.destroy();

    /**
     * @brief
        Min：恢复时间的最小值。
        Max：恢复时间的最大值。
        Mean：恢复时间的平均值。
        Median：恢复时间的中位数。
        Standard Deviation：恢复时间的标准差，反映数据的波动性
     *
     */

    // 计算极值
    double min_time = *std::min_element(times.begin(), times.end());
    double max_time = *std::max_element(times.begin(), times.end());

    // 计算平均值
    double mean_time = std::accumulate(times.begin(), times.end(), 0.0) / times.size();

    // 计算中位数
    std::sort(times.begin(), times.end());
    double median_time = times[times.size() / 2];

    // 计算标准差
    double variance = 0.0;
    for (double time : times)
    {
        variance += std::pow(time - mean_time, 2);
    }
    variance /= times.size();
    double std_dev = std::sqrt(variance);

    std::cout << "Resume 10000 times (ns):\n";
    std::cout << "Min: " << min_time << "\nMax: " << max_time << "\n";
    std::cout << "Mean: " << mean_time << "\nMedian: " << median_time << "\n";
    std::cout << "Standard Deviation: " << std_dev << "\n";

    {
        // 定义 1 毫秒
        auto one_millisecond = std::chrono::milliseconds(1);

        // 将 1 毫秒转换为纳秒
        auto one_millisecond_in_nanoseconds =
            std::chrono::duration_cast<std::chrono::nanoseconds>(one_millisecond);

        // 输出结果
        std::cout << "1 毫秒 (ms) = " << one_millisecond_in_nanoseconds.count()
                  << " 纳秒 (ns)" << std::endl;

        // 验证关系
        if (one_millisecond_in_nanoseconds.count() == 1000000)
        {
            std::cout << "验证成功: 1 毫秒等于 1,000,000 纳秒。" << std::endl;
        }
        else
        {
            std::cout << "验证失败: 1 毫秒不等于 1,000,000 纳秒。" << std::endl;
        }
        /**
         * @brief 纳秒（ns）：通常用于测量非常短的时间间隔，例如 CPU
            指令执行时间、内存访问时间等。

            毫秒（ms）：通常用于测量较长的时间间隔，例如网络延迟、磁盘 I/O 操作等。
         *
         */
    }

    return 0;
}
// NOLINTEND