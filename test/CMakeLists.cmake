# 启用测试
enable_testing()

set(TEST_ROOT_DIR "${CMAKE_SOURCE_DIR}/test")
set(TEST_EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/output/test_program)

# ut.hpp 不使用 MODULE 语法
add_definitions(-DBOOST_UT_DISABLE_MODULE)
include(${CMAKE_SOURCE_DIR}/test/script/auto_add_test_by_dir.cmake) # 注册测试
include(${CMAKE_SOURCE_DIR}/test/script/auto_add_exec.cmake)
auto_add_exec("base")

# add_executable(test_hello ${CMAKE_SOURCE_DIR}/test/hello/test_hello.cpp)
# add_test(NAME test_hello COMMAND $<TARGET_FILE:test_hello>)
# add_executable(test_ut ${CMAKE_SOURCE_DIR}/test/hello/test_ut.cpp)
# add_test(NAME test_ut COMMAND $<TARGET_FILE:test_ut>)
auto_add_test_by_dir("hello")
auto_add_test_by_dir("recv")
auto_add_test_by_dir("adaptors")
auto_add_test_by_dir("factories")
auto_add_test_by_dir("cmplsigs")
auto_add_test_by_dir("consumers")
auto_add_test_by_dir("awaitables")
auto_add_test_by_dir("concept")
auto_add_test_by_dir("queries")
auto_add_test_by_dir("task")