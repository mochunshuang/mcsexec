# https://cmake.org/cmake/help/latest/command/cmake_host_system_information.html
# 查询 CPU 核心数
message(STATUS "System Information:")
cmake_host_system_information(RESULT NUM_CPUS QUERY NUMBER_OF_LOGICAL_CORES)
message(STATUS "Number of CPU cores: ${NUM_CPUS}")

message("")
