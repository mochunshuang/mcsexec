#include <iostream>
#include <new> // for std::bad_alloc
#include <chrono>

class MyClass // NOLINT
{
  public:
    MyClass()
    {
        std::cout << "Constructor called!\n";
    }
    ~MyClass()
    {
        std::cout << "Destructor called!\n";
    }
    void doSomething() {}
};

int main()
{
    // new 失败， 构造函数和析构函数都不会被调用
    try
    {

        MyClass *obj = new MyClass[1000000000000]; // NOLINT
        delete[] obj;                              // 这行代码不会被执行
    }
    catch (const std::bad_alloc &e)
    {
        std::cerr << "new failed: " << e.what() << '\n';
    }
    {
        // std::nothrow 的局限性：std::nothrow 只能用于避免 new
        // 抛出异常，但不能处理其他异常（例如构造函数中抛出的异常）。
        // std::nothrow，new 失败时返回 nullptr
        MyClass *obj = new (std::nothrow) MyClass[1000000000000]; // NOLINT

        if (obj == nullptr)
        {
            std::cerr << "new failed: Memory allocation failed!\n";
        }
        else
        {
            delete[] obj; // 如果分配成功，释放内存
        }
    }

    class MyClass // NOLINT
    {
      public:
        MyClass() {}
        ~MyClass() {}
        void doSomething() {}
    };
    {
        auto *ptr = new MyClass();
        ptr->~MyClass();
        // 循环使用 ptr 和 单独new /delete 的性能，开销对象 1000次，不能有编译期优化
        auto start = std::chrono::high_resolution_clock::now();

        const int iterations = 1000; // NOLINT
        for (int i = 0; i < iterations; ++i)
        {
            ptr->~MyClass();     // 手动调用析构函数
            new (ptr) MyClass(); // 使用 placement new 重新构造对象
            ptr->doSomething();
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Loop reuse time: " << elapsed.count() << " seconds\n";
        delete ptr; // 释放内存
    }
    {
        auto *ptr = new MyClass();
        ptr->~MyClass();
        // 循环使用 ptr 和 单独new /delete 的性能，开销对象 1000次，不能有编译期优化
        auto start = std::chrono::high_resolution_clock::now();

        const int iterations = 1000; // NOLINT
        for (int i = 0; i < iterations; ++i)
        {
            // ptr->~MyClass();     // 手动调用析构函数
            new (ptr) MyClass(); // 使用 placement new 重新构造对象
            ptr->doSomething();
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "no  ptr->~MyClass(), Loop reuse time: " << elapsed.count()
                  << " seconds\n";
        delete ptr; // 释放内存
    }
    {
        const int iterations = 1000; // NOLINT
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < iterations; ++i)
        {
            auto *ptr = new MyClass(); // 分配内存
            ptr->doSomething();
            delete ptr; // 释放内存
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = end - start;
        std::cout << "Separate new/delete time: " << elapsed.count() << " seconds\n";
    }
    // 结论，差10 倍。

    return 0;
}