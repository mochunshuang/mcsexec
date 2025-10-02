#include <iostream>
#include <cstring>
// https://clang.llvm.org/docs/UsersManual.html#finding-clang-runtime-libraries
void memory_leak_example()
{
    // 内存泄漏示例
    char *leak = new char[100];
    strcpy(leak, "This will leak");
    // 忘记 delete[] leak;
}

void double_free_example()
{
    char *ptr = new char[50];
    delete[] ptr;
    // delete[] ptr;  // 重复释放 - ASan 会捕获
}

void use_after_free()
{
    int *array = new int[10];
    delete[] array;
    array[0] = 42; // 释放后使用 - ASan 会捕获
}

int main()
{

    std::cout << "Memory Check Demo\n";

    // 取消注释来测试不同的内存错误
    // memory_leak_example();
    // double_free_example();
    use_after_free();

    // 正确的内存管理
    int *proper = new int[100];
    // ... 使用内存
    delete[] proper;
    // delete[] proper;

    // NOTE: windows 就是一点用都没有
    std::cout << "Program completed\n";
    return 0;
}