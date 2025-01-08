#include <cassert>
#include <iostream>
#include <utility>

// 链表节点结构
struct Node
{
    int value;                                   // NOLINT
    Node *next;                                  // NOLINT
    Node(int val) : value(val), next(nullptr) {} // NOLINT
};

// 队列类
class Queue // NOLINT
{
  private:
    Node *front; // NOLINT
    Node *rear;  // NOLINT

  public:
    Queue() : front(nullptr), rear(nullptr) {}

    [[nodiscard]] bool isEmpty() const
    {
        return front == nullptr;
    }

    void push_back(int value) // NOLINT
    {
        Node *newNode = new Node(value); // 创建新节点
        if (isEmpty())
        {
            front = rear = newNode;
        }
        else
        {
            rear->next = newNode;
            rear = newNode;
        }
    }

    // 从队列头部移除元素并返回该节点
    Node *pop_front() // NOLINT
    {
        if (isEmpty())
        {
            std::cout << "Queue is empty! Cannot pop.\n";
            return nullptr;
        }
        Node *temp = std::exchange(front, front->next);
        if (front == nullptr)
        {
            rear = nullptr;
        }
        return temp; // 返回被移除的节点
    }

    // 析构函数，释放队列中所有节点的内存
    ~Queue()
    {
        while (!isEmpty())
        {
            Node *temp = pop_front();
            delete temp;
        }
    }
};

int main()
{
    Queue q;
    // NOLINTBEGIN
    q.push_back(10);
    q.push_back(20);
    q.push_back(30);

    Node *node1 = q.pop_front();
    if (node1 != nullptr)
    {
        assert(node1->value == 10);
        delete node1;
    }

    q.push_back(40);
    q.push_back(50);

    Node *node2 = q.pop_front();
    if (node2)
    {
        assert(node2->value == 20);
        delete node2;
    }

    Node *node3 = q.pop_front();
    if (node3)
    {
        assert(node3->value == 30);
        delete node3;
    }

    Node *node4 = q.pop_front();
    if (node4)
    {
        assert(node4->value == 40);
        delete node4;
    }

    Node *node5 = q.pop_front(); // 队列已空，无法移除
    if (node5)
    {
        assert(node5->value == 50);
        delete node5;
    }
    // NOLINTEND
    return 0;
}