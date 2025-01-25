#include <iostream>

// NOLINTBEGIN
// 链表节点定义
struct Node
{
    int value;
    Node *next;
};

// 辅助函数：打印链表
void print_list(Node *head)
{
    Node *current = head;
    while (current != nullptr)
    {
        std::cout << current->value << " -> ";
        current = current->next;
    }
    std::cout << "nullptr" << std::endl;
}

// 删除指定值的节点
void delete_node(Node *&head, int value)
{
    Node **indirect = &head; // 二级指针指向头指针

    while (*indirect != nullptr)
    {
        if ((*indirect)->value == value)
        {                                  // 找到目标节点
            *indirect = (*indirect)->next; // 修改指针，跳过目标节点
            return;                        // 删除完成后直接返回
        }
        indirect = &((*indirect)->next); // 移动到下一个节点的 next 指针
    }
}

/**
 * @brief 引用的方式遍历和删除，是错误的。
 *
 * @param head
 * @param value
 */
void delete_node_ref(Node *&head, int value)
{
    Node *&indirect = head; // 使用引用 指向 头结点

    // 遍历链表
    while (indirect != nullptr)
    {
        if (indirect->value == value)
        {
            indirect = indirect->next; // 跳过目标节点
            return;
        }
        indirect = indirect->next; // 移动到下一个节点的引用
    }
}

// 测试代码
int main()
{
    // 在栈上构建链表: 1 -> 2 -> 3 -> 4 -> nullptr
    Node node1 = {1, nullptr};
    Node node2 = {2, nullptr};
    Node node3 = {3, nullptr};
    Node node4 = {4, nullptr};

    node1.next = &node2;
    node2.next = &node3;
    node3.next = &node4;

    Node *head = &node1; // 头节点指向 node1

    std::cout << "初始链表: ";
    print_list(head);

    // 测试删除节点 3
    delete_node(head, 3);
    std::cout << "删除节点 3 后: ";
    print_list(head);

    // 测试删除节点 2
    delete_node(head, 2);
    std::cout << "删除节点 2 后: ";
    print_list(head);

    // 测试删除节点 1
    delete_node(head, 1);
    std::cout << "删除节点 1 后: ";
    print_list(head);

    // 测试删除不存在的节点 5
    delete_node(head, 5);
    std::cout << "删除节点 5 后: ";
    print_list(head);

    // 测试空链表
    Node *empty_head = nullptr;
    delete_node(empty_head, 1);
    std::cout << "删除空链表中的节点 1 后: ";
    print_list(empty_head);

    std::cout << "\n引用版本: \n";
    {
        Node node1 = {1, nullptr};
        Node node2 = {2, nullptr};
        Node node3 = {3, nullptr};
        Node node4 = {4, nullptr};

        node1.next = &node2;
        node2.next = &node3;
        node3.next = &node4;

        Node *head = &node1; // 头节点指向 node1

        std::cout << "初始链表: ";
        print_list(head);

        // 测试删除节点 3
        delete_node_ref(head, 3);
        std::cout << "删除节点 3 后: ";
        print_list(head);

        // 测试删除节点 2
        delete_node_ref(head, 2);
        std::cout << "删除节点 2 后: ";
        print_list(head);

        // 测试删除节点 1
        delete_node_ref(head, 1);
        std::cout << "删除节点 1 后: ";
        print_list(head);

        // 测试删除不存在的节点 5
        delete_node_ref(head, 5);
        std::cout << "删除节点 5 后: ";
        print_list(head);

        // 测试空链表
        Node *empty_head = nullptr;
        delete_node_ref(empty_head, 1);
        std::cout << "删除空链表中的节点 1 后: ";
        print_list(empty_head);

        std::cout << "引用版本: ";
    }

    return 0;
}
// NOLINTEND