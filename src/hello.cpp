
#include <iostream>

import mcs.execution.functional;
import mcs.execution;

int main()
{

    // 概念可以导出
    callable auto fun = []() {
        return 1;
    };
    fun();
    hello();
    world();
    std::cout << "hello world\n";
    return 0;
}