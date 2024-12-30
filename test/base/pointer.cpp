#include <cassert>
#include <iostream>

struct MyClass
{
    int *ptr; // NOLINT

    explicit MyClass(int *p) : ptr(p) {}
    MyClass(const MyClass &other) = default; // 默认拷贝构造函数
    MyClass &operator=(const MyClass &other) = default;
    MyClass(MyClass &&other) = default;
    MyClass &operator=(MyClass &&other) = default;
    ~MyClass() = default;
};

void test_class(); // NOLINT

int main()
{
    int obj = 1;
    int *A = &obj; // A指向obj
    int *B = A;    // B指向A所指向的对象
    int *C = B;    // C指向B所指向的对象

    *C = 2; // 通过C修改对象

    // 此时，A、B、C都指向同一个对象，
    std::cout << *A << " " << *B << " " << *C << '\n';
    std::cout << A << " " << B << " " << C << '\n';
    std::cout << &A << " " << &B << " " << &C << '\n';

    assert(*A == *B and *B == *C);
    assert(A == B and B == C);
    assert(&A != &B and &B != &C and &C != &A);
    test_class();
    return 0;
}
void test_class()
{
    int obj = 1;
    MyClass A(&obj); // A的ptr指向obj
    MyClass B = A;   // 浅拷贝，B的ptr指向A的ptr所指向的对象

    *B.ptr = 2;

    std::cout << *A.ptr << " " << *B.ptr << '\n';
    std::cout << A.ptr << " " << B.ptr << '\n';
    std::cout << &A.ptr << " " << &B.ptr << '\n';

    assert(*A.ptr == *B.ptr);
    assert(A.ptr == B.ptr);
    assert(&A.ptr != &B.ptr);
}