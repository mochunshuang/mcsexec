#include <iostream>
#include <chrono>

int main()
{
    std::chrono::steady_clock::time_point m_lastShrinkTime;

    // 假设 if 内部被执行了
    if (m_lastShrinkTime.time_since_epoch().count() == 0)
    {
        m_lastShrinkTime = std::chrono::steady_clock::now();
        std::cout << "m_lastShrinkTime assigned: "
                  << m_lastShrinkTime.time_since_epoch().count() << "\n";
    }

    // 重置 m_lastShrinkTime 为默认值
    m_lastShrinkTime = {};

    // 检查是否重置成功
    if (m_lastShrinkTime.time_since_epoch().count() == 0)
    {
        std::cout << "m_lastShrinkTime reset to 0.\n";
    }
    else
    {
        std::cout << "m_lastShrinkTime not reset to 0.\n";
    }

    return 0;
}