#include <cassert>
#include <iostream>
#include <thread>

int main()
{
    std::thread::id default_id; // 默认构造的 std::thread::id
    std::cout << "Default thread ID: " << default_id << '\n';

    for (int i = 0; i < 100; ++i) // NOLINT
    {
        std::thread::id default_id2;
        assert(default_id == default_id2);
    }

    assert(default_id == std::thread::id{});

    assert(default_id != std::this_thread::get_id());
    return 0;
}