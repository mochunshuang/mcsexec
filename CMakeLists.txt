cmake_minimum_required(VERSION 3.30)

project(mcsexec_test VERSION 0.1.0.0 LANGUAGES CXX)

# .cmake
include(cmake/host_system_information.cmake)
include(cmake/output.cmake)
include(cmake/third_party.cmake)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF) # 避免扩展 -std=gnu++23
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 添加测试
#
include(${CMAKE_SOURCE_DIR}/test/CMakeLists.cmake)
