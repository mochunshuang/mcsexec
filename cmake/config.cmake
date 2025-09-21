# 根据构建类型定义不同的宏
if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(MCS_ENABLE_LOG ON)
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(MCS_ENABLE_LOG ON)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(MCS_ENABLE_LOG)
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(MCS_ENABLE_LOG)
endif()

configure_file(${CMAKE_SOURCE_DIR}/config/config_debug.hpp.in ${CMAKE_SOURCE_DIR}/config/config_debug.hpp NO_SOURCE_PERMISSIONS @ONLY)
include_directories(${CMAKE_SOURCE_DIR}/config)
