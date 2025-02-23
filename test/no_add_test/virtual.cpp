#include <iostream>
using namespace std;

// 基类，包含虚函数
class Base
{
    int a{};

  public:
    virtual void foo()
    {
        cout << "Base::foo()" << endl;
    }
};

// 派生类，重写虚函数
class Derived : public Base
{
  public:
    void foo() override
    {
        cout << "Derived::foo()" << endl;
    }
};

// 非虚函数类，用于对比
class NonVirtual
{
};

int main()
{
    // 验证对象大小：带虚函数的类会多一个vptr
    cout << "Size of Base: " << sizeof(Base) << endl; // 输出 8 (64位) 或 4 (32位)
    cout << "Size of NonVirtual: " << sizeof(NonVirtual) << endl; // 输出 1
    cout << "Size of void*: " << sizeof(void *) << endl;          // 确认指针大小

    Base b;
    Derived d;

    // 获取vptr地址（对象首地址即vptr）
    void **vptr_b = reinterpret_cast<void **>(&b);
    void *vtbl_b = *vptr_b;
    cout << "Base: " << &b << endl;
    cout << "Base's vptr: " << vtbl_b << endl;

    void **vptr_d = reinterpret_cast<void **>(&d);
    void *vtbl_d = *vptr_d;
    cout << "Derived: " << &d << endl;
    cout << "Derived's vptr: " << vtbl_d << endl;

    // NOTE:
    // 每个类（而不是每个对象）拥有独立的虚函数表。所有该类的对象共享同一个虚函数表。
    cout << "Base's vtbl == Derived's vtbl: " << (bool)(vtbl_b == vtbl_d) << endl;
    {
        // 创建多个 Base 对象
        Base b1, b2, b3;

        // 获取每个对象的 vptr
        void **vptr_b1 = reinterpret_cast<void **>(&b1);
        void *vtbl_b1 = *vptr_b1;

        void **vptr_b2 = reinterpret_cast<void **>(&b2);
        void *vtbl_b2 = *vptr_b2;

        void **vptr_b3 = reinterpret_cast<void **>(&b3);
        void *vtbl_b3 = *vptr_b3;

        // 输出每个对象的 vptr 和虚函数表地址
        cout << "b1: " << &b1 << ", vptr: " << vptr_b1 << ", vtbl: " << vtbl_b1 << endl;
        cout << "b2: " << &b2 << ", vptr: " << vptr_b2 << ", vtbl: " << vtbl_b2 << endl;
        cout << "b3: " << &b3 << ", vptr: " << vptr_b3 << ", vtbl: " << vtbl_b3 << endl;

        // 比较虚函数表地址
        cout << "b1's vtbl == b2's vtbl: " << (vtbl_b1 == vtbl_b2) << endl;
        cout << "b1's vtbl == b3's vtbl: " << (vtbl_b1 == vtbl_b3) << endl;
    }

    // 定义函数指针类型（假设调用约定匹配）
    typedef void (*FuncPtr)(Base *);

    // 从vtable中获取第一个虚函数地址
    FuncPtr base_foo = reinterpret_cast<FuncPtr>(reinterpret_cast<void **>(vtbl_b)[0]);
    cout << "Manual call on Base: ";
    base_foo(&b); // 输出 Base::foo()

    FuncPtr derived_foo = reinterpret_cast<FuncPtr>(reinterpret_cast<void **>(vtbl_d)[0]);
    cout << "Manual call on Derived: ";
    derived_foo(&d); // 输出 Derived::foo()

    // 通过基类指针调用派生类对象的虚函数
    Base *pd = &d;
    void **vptr_pd = reinterpret_cast<void **>(pd);
    void *vtbl_pd = *vptr_pd;
    FuncPtr pd_foo = reinterpret_cast<FuncPtr>(reinterpret_cast<void **>(vtbl_pd)[0]);
    cout << "Manual call through Base pointer: ";
    pd_foo(pd); // 输出 Derived::foo()

    return 0;
}