#include <concepts>
#include <iostream>
// NOLINTBEGIN
template <typename T>
    requires(requires(T *t) {
        { t->next } -> std::same_as<T *&>;
    })
class intrusive_forward_list
{
  private:
    T *head = nullptr;

  public:
    void push_front(T *obj)
    {
        obj->next = head;
        head = obj;
    }

    void pop_front()
    {
        if (head)
        {
            head = head->next;
        }
    }

    T *front()
    {
        return head;
    }

    bool empty() const
    {
        return head == nullptr;
    }
};

struct MyNode
{
    int value;
    MyNode *next = nullptr;

    MyNode(int v) : value(v) {}
};

int main()
{
    intrusive_forward_list<MyNode> list;

    MyNode *a = new MyNode(3);
    MyNode *b = new MyNode(2);
    MyNode *c = new MyNode(1);

    list.push_front(a);
    list.push_front(b);
    list.push_front(c);

    std::cout << "Front element: " << list.front()->value << std::endl; // 输出 1

    list.pop_front();
    std::cout << "Front element after pop: " << list.front()->value
              << std::endl; // 输出 2

    // 手动释放内存
    delete a;
    delete b;
    delete c;

    return 0;
}
// NOLINTEND