#include <chrono>
#include <exception>
#include <iostream>
#include <thread>
#include "task_v2/__task.hpp"

int main()
{
    namespace ex = ::mcs::execution;

    std::cout << "main done\n";
    return 0;
}