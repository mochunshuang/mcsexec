# https://cmake.org/cmake/help/latest/command/cmake_host_system_information.html
# 查询 CPU 核心数
message(STATUS "System Information:")
cmake_host_system_information(RESULT NUM_CPUS QUERY NUMBER_OF_LOGICAL_CORES)
message(STATUS "Number of CPU cores: ${NUM_CPUS}")

message(STATUS "Compiler ID: ${CMAKE_CXX_COMPILER_ID}")

if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
    message(STATUS "Clang detected, setting Clang-specific settings.")

# clang forward_like 有问题, 盗版的clang 才有的问题。正版文件是：LLVM-19.1.7-win64.exe
# add_definitions(-fno-builtin-std-forward_like)

# 在这里设置针对 Clang 的特定配置
elseif(${CMAKE_CXX_COMPILER_ID} MATCHES "MSVC")
    message(STATUS "MSVC detected, setting MSVC-specific settings.")

# 在这里设置针对 MSVC 的特定配置
else()
    message(STATUS "Unknown compiler detected.")

    # 对于未知编译器，可以设置默认配置或给出警告
endif()

message("")
