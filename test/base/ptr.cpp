#include <iostream>

int main()
{
    int arr[] = {10, 20, 30, 40, 50};
    void *ptr = arr;

    // 将 void* 转换为 int*
    int *int_ptr = static_cast<int *>(ptr);

    // 动态偏移访问元素
    int offset = 2;
    int value = *(int_ptr + offset);
    std::cout << "Value at offset " << offset << ": " << value << "\n";

    return 0;
}