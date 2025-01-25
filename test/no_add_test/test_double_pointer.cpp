
#include <cassert>

// NOLINTBEGIN
void test_single_pointer()
{
    int a = 10;
    int *old_head = &a;       // 一级指针
    int *exc_node = old_head; // 复制指针值

    int b = 20;
    old_head = &b; // update

    assert(*old_head == 20);
    assert(*exc_node == 10);
}

void test_double_pointer()
{
    int a = 10;
    int *old_head = &a;         // 一级指针
    auto *exc_node = &old_head; // 复制指针值

    int b = 20;
    old_head = &b; // update

    assert(*old_head == 20);
    assert(**exc_node == 20);
}

struct Node
{
    int value;
    Node *next;
};

void test_double_pointer_with_struct()
{
    Node node1 = {10, nullptr};
    Node node2 = {20, nullptr};

    Node *old_head = &node1;     // 一级指针，指向 node1
    Node **exc_node = &old_head; // 二级指针，指向 old_head

    // 断言初始状态
    assert((*exc_node)->value == 10);
    assert(old_head->value == 10);

    // 修改 old_head 的指向
    old_head = &node2;
    assert(old_head->value == 20);
    assert((*exc_node)->value == 20);
}

void test_reference_with_struct()
{
    Node node1 = {10, nullptr}; // 创建第一个节点
    Node node2 = {20, nullptr}; // 创建第二个节点

    Node *old_head = &node1;   // 一级指针，指向 node1
    auto &exc_node = old_head; // 引用，绑定到 old_head

    // 断言初始状态
    assert(exc_node->value == 10); // exc_node 是 old_head 的引用，指向 node1
    assert(old_head->value == 10); // old_head 指向 node1

    // 修改 old_head 的指向
    old_head = &node2;             // old_head 现在指向 node2
    assert(old_head->value == 20); // old_head 指向 node2，值为 20
    assert(exc_node->value == 20); // exc_node 是 old_head 的引用，也指向 node2

    // 通过引用修改 old_head 的指向
    exc_node = &node1;             // 通过引用修改 old_head 的指向
    assert(old_head->value == 10); // old_head 现在指向 node1
    assert(exc_node->value == 10); // exc_node 也指向 node1
}

// NOLINTEND
int main()
{
    test_single_pointer();
    test_double_pointer();
    test_double_pointer_with_struct();
    test_reference_with_struct();
    return 0;
}