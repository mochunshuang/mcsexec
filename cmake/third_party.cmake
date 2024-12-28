# 网络下载不现实，网速在哪
set(THIRD_PARTY_DIR ${CMAKE_SOURCE_DIR}/third_party)

# head-only 就是舒服
# add_subdirectory(${THIRD_PARTY_DIR}/ut-2.1.1)
include_directories(${THIRD_PARTY_DIR}/ut-2.1.1/include)