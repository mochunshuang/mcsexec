# 启用测试
enable_testing()

set(TEST_ROOT_DIR "${CMAKE_SOURCE_DIR}/test")
set(TEST_EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/output/test_program)

include(${CMAKE_SOURCE_DIR}/test/script/auto_add_test_by_dir.cmake) # 注册测试
include(${CMAKE_SOURCE_DIR}/test/script/auto_add_exec.cmake)
auto_add_exec("base")
auto_add_exec("exec")

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
auto_add_test_by_dir("stop_token")

# 定义自定义命令，用于构建所有目标：注意，要在build目录下
# 等价于：E:\0_github_project\mcsexec\mcsexec\build> ctest --parallel 16 -C Debug
# 不需要这个目标 因为和 cmaketool 启动并行测试冲突。依赖 DEPENDS all，没生成完就结束了
# add_custom_target(run_all_tests
# COMMAND ${CMAKE_COMMAND} -E echo "Running All Tests!"
# COMMAND ${CMAKE_CTEST_COMMAND} --parallel ${NUM_CPUS} -C ${CMAKE_BUILD_TYPE} --output-on-failure
# COMMAND ${CMAKE_COMMAND} -E echo "All Tests done!"
# COMMENT "Running all tests"
# DEPENDS all
# )
add_executable(no_add_test ${CMAKE_SOURCE_DIR}/test/no_add_test/thread_local3_7.cpp)