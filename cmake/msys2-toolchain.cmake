# cmake/msys2-toolchain.cmake
set(CMAKE_SYSTEM_NAME Windows)

# NOTE 一坨屎。 MINGW 还是不行，clang 跨平台能力更强
# 还是越简单越好，MYS32 也解决不了。别忘了，都是 MINGW-64的源码
# 早该想到的，理论上确实可能性不大。一个成功另一个失败不太可能
# 一个师傅，差不多的

# 编译器路径
set(MSYS2_PREFIX "D:/mysoftware/msys64/ucrt64")
set(CMAKE_C_COMPILER "${MSYS2_PREFIX}/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "${MSYS2_PREFIX}/bin/g++.exe")

# 关键工具链工具
# set(CMAKE_AR "${MSYS2_PREFIX}/bin/ar.exe")
# set(CMAKE_RANLIB "${MSYS2_PREFIX}/bin/ranlib.exe")
# set(CMAKE_LD "${MSYS2_PREFIX}/bin/ld.exe")
# set(CMAKE_NM "${MSYS2_PREFIX}/bin/nm.exe")
# set(CMAKE_OBJCOPY "${MSYS2_PREFIX}/bin/objcopy.exe")
# set(CMAKE_OBJDUMP "${MSYS2_PREFIX}/bin/objdump.exe")
# set(CMAKE_STRIP "${MSYS2_PREFIX}/bin/strip.exe")
# set(CMAKE_RC_COMPILER "${MSYS2_PREFIX}/bin/windres.exe")

# 依赖库和头文件路径
# set(CMAKE_PREFIX_PATH "${MSYS2_PREFIX}")
# set(CMAKE_FIND_ROOT_PATH "${MSYS2_PREFIX}")
# set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
# set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# 强制使用 MSYS2 的运行时库
# 强制包含 MSYS2 的系统头文件和库路径
# 显式添加 MSYS2 的 include 和 lib 路径（非覆盖）
set(CMAKE_CXX_FLAGS_INIT "-isystem ${MSYS2_PREFIX}/include")
set(CMAKE_EXE_LINKER_FLAGS_INIT "-L${MSYS2_PREFIX}/lib")

# include_directories(
# D:/mysoftware/msys64/ucrt64/include
# D:/mysoftware/msys64/ucrt64/bin/../lib/gcc/x86_64-w64-mingw32/14.2.0/../../../../include/c++/14.2.0
# D:/mysoftware/msys64/ucrt64/bin/../lib/gcc/x86_64-w64-mingw32/14.2.0/../../../../include/c++/14.2.0/x86_64-w64-mingw32
# D:/mysoftware/msys64/ucrt64/bin/../lib/gcc/x86_64-w64-mingw32/14.2.0/../../../../include/c++/14.2.0/backward
# D:/mysoftware/msys64/ucrt64/bin/../lib/gcc/x86_64-w64-mingw32/14.2.0/include
# D:/mysoftware/msys64/ucrt64/bin/../lib/gcc/x86_64-w64-mingw32/14.2.0/../../../../include
# D:/mysoftware/msys64/ucrt64/bin/../lib/gcc/x86_64-w64-mingw32/14.2.0/include-fixed
# )