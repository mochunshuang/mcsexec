#include <atomic>
#include <cassert>
#include <iostream>
#include <string>

// NOLINTBEGIN
// 定义 inplace_callback_base 结构体
struct inplace_callback_base
{
    inplace_callback_base *next;
    int value;        // 添加一个值成员变量
    std::string name; // 添加一个名称成员变量

    // 构造函数
    inplace_callback_base(int val, const std::string &n)
        : next(nullptr), value(val), name(n)
    {
    }
};

std::atomic<inplace_callback_base *> register_list{nullptr};

// 头插法插入
void push(inplace_callback_base *new_node)
{
    if (!new_node)
        return;

    // 将新节点的 next 指向当前链表的头节点
    new_node->next = register_list.load(std::memory_order_relaxed);

    // 使用 CAS 操作将 register_list 更新为新节点
    while (!register_list.compare_exchange_weak(
        new_node->next, new_node, std::memory_order_release, std::memory_order_relaxed))
    {
        // CAS 失败，重试
    }

    // 打印插入的节点信息
    std::cout << "Pushed node: " << new_node->name << " (value = " << new_node->value
              << ")" << std::endl;
}

// 头部弹出
inplace_callback_base *pop()
{
    inplace_callback_base *old_head = register_list.load(std::memory_order_relaxed);

    // 如果链表为空，直接返回 nullptr
    if (!old_head)
    {
        std::cout << "Pop failed: list is empty." << std::endl;
        return nullptr;
    }

    // 使用 CAS 操作将 register_list 更新为 old_head->next
    while (!register_list.compare_exchange_weak(
        old_head, old_head->next, std::memory_order_release, std::memory_order_relaxed))
    {
        // CAS 失败，重试
    }

    // 打印弹出的节点信息
    std::cout << "Popped node: " << old_head->name << " (value = " << old_head->value
              << ")" << std::endl;

    // 返回弹出的节点
    return old_head;
}

// 示例用法
int main()
{
    assert(register_list.is_lock_free());

    // 创建几个节点
    inplace_callback_base node1(10, "Node1");
    inplace_callback_base node2(20, "Node2");
    inplace_callback_base node3(30, "Node3");

    // 插入节点
    push(&node1);
    push(&node2);
    push(&node3);

    // 弹出节点
    inplace_callback_base *popped_node = pop();
    if (popped_node)
    {
        std::cout << "Successfully popped: " << popped_node->name << std::endl;
    }

    // 再次弹出节点
    popped_node = pop();
    if (popped_node)
    {
        std::cout << "Successfully popped: " << popped_node->name << std::endl;
    }

    // 再次弹出节点
    popped_node = pop();
    if (popped_node)
    {
        std::cout << "Successfully popped: " << popped_node->name << std::endl;
    }

    // 尝试弹出空链表
    popped_node = pop();

    return 0;
}
// NOLINTEND