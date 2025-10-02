
#include <cassert>
#include <cstddef>
#include <iostream>
#include <new>
#include <type_traits>

template <typename T>
concept scheduler_concept = requires(T t) {
    { t.schedule() } noexcept;
};

struct run_loop
{
};

struct sender
{
    run_loop *loop;
};

struct scheduler
{
    run_loop *loop;
    auto schedule() noexcept
    {
        return sender{loop};
    }
};

int main()
{
    static constexpr auto k_max_size = sizeof(void *) * 2;
    // 使用C++23推荐的方式替代std::aligned_storage_t
    alignas(std::max_align_t) std::byte storage[k_max_size];

    run_loop loop;
    // 在存储中构造sender对象
    ::new (&storage) sender(&loop);

    // 获取原始对象指针
    auto *original = std::launder(reinterpret_cast<sender *>(&storage));
    assert(original->loop == &loop);

    // 复制存储的内容：
    alignas(std::max_align_t) std::byte copy[k_max_size];

    // 1. 通过原始对象的复制构造函数在新存储中创建副本
    ::new (&copy) sender(*original);

    // 2. 验证复制结果
    auto *copied = std::launder(reinterpret_cast<sender *>(&copy));
    assert(copied->loop == &loop); // 复制的是指针值，指向同一个loop
    assert(copied != original);    // 确保是不同的对象

    // 清理：显式销毁对象（对于POD类型不是必须，但好习惯）
    std::destroy_at(original);
    std::destroy_at(copied);

    std::cout << "Schedule called successfully" << '\n';
    return 0;
}
