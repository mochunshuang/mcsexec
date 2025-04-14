# 定义函数：auto_add_test_by_dir
# 参数：
# dir_name: 目录名（组名）
function(auto_add_test_by_dir dir_name)
    if(NOT DEFINED TEST_ROOT_DIR)
        message(FATAL_ERROR "TEST_ROOT_DIR is not defined. Please define TEST_ROOT_DIR.")
    endif()

    # 获取指定目录下的所有 .cpp 文件
    file(GLOB test_files "${TEST_ROOT_DIR}/${dir_name}/test_*.cpp")

    # 遍历每个 .cpp 文件
    foreach(test_file ${test_files})
        # 获取文件名（不带路径和扩展名）
        get_filename_component(test_name ${test_file} NAME_WE)

        # 生成可执行程序名：组-cpp文件名
        string(REPLACE "/" "-" path_prefix "${dir_name}")
        set(target_name "${path_prefix}-${test_name}")

        # 添加可执行程序
        add_executable(${target_name} ${test_file})
        set_target_properties(${target_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${TEST_EXECUTABLE_OUTPUT_PATH}/${dir_name}
        )

        # 添加测试
        add_test(NAME "${target_name}" COMMAND $<TARGET_FILE:${target_name}>)

        # 打印信息（可选）
        message(STATUS "[Added test]: ${target_name} from ${test_file}")
    endforeach()
endfunction()