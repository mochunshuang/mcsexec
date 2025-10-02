#include <iostream>

#include "config_debug.hpp"

int main()
{
    std::cout << "Build type: " << CMAKE_BUILD_TYPE << std::endl;
#ifdef MCS_ENABLE_LOG
    std::cout << "CMAKE_BUILD_TYPE == RelWithDebInfo: " << CMAKE_BUILD_TYPE << std::endl;
#endif
    std::cout << "main done\n";
    return 0;
}