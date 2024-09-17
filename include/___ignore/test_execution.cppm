module;
#include <iostream>

export module mcs.execution;

template <class Fn, class... Args>
using call_result_t = decltype(std::declval<Fn>()(std::declval<Args>()...));

export call_result_t<decltype([]() {})> world()
{
    std::cout << "hello world\n";
}