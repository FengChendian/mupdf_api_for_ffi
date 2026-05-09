cmake_minimum_required(VERSION 3.16)
project(mupdf C)

set(CMAKE_C_STANDARD 99)

# 包含 mupdf_test 头文件目录
include_directories(${CMAKE_SOURCE_DIR}/include)

# 定义宏以使用 mupdf_test DLL 导入函数
add_compile_definitions(FZ_DLL_CLIENT)



# DLL 导出库
add_library(mupdf SHARED
    mupdf_api.c
    mupdf_api.h
)
target_compile_definitions(mupdf PRIVATE MUPDF_API_EXPORTS)
# target_link_libraries(mupdf PRIVATE ${MUPDF_LIB})



# 添加可执行文件 - 链接到 mupdf DLL 以测试导出的函数
add_executable(mupdf_test main.c)
target_link_libraries(mupdf_test PRIVATE mupdf)

# 查找 DLL 目录下的库文件
file(GLOB MUPDF_LIB "${CMAKE_SOURCE_DIR}/lib/*.lib")
# file(GLOB mupdf "${CMAKE_SOURCE_DIR}/dll/*.dll")

# if(NOT mupdf)
#     message(FATAL_ERROR "未找到 DLL 文件，请将 MuPDFLib.dll 放入 dll/ 目录")
# endif()
if(MSVC)
    set(RELEASE_CONFIGS Release RelWithDebInfo MinSizeRel)
    add_compile_options("$<$<CONFIG:${RELEASE_CONFIGS}>:/GL>")
    add_link_options("$<$<CONFIG:${RELEASE_CONFIGS}>:/LTCG>")
endif()

if(MUPDF_LIB)
    # 使用导入库 (.lib/.a) 链接
    target_link_libraries(mupdf PRIVATE ${MUPDF_LIB})
    target_link_libraries(mupdf_test PRIVATE ${MUPDF_LIB})
    message(STATUS "找到导入库: ${MUPDF_LIB}")
else()
    target_link_libraries(mupdf PRIVATE ${mupdf})
    target_link_libraries(mupdf_test PRIVATE ${mupdf})
    message(STATUS "MinGW/Clang  detected，直接链接 DLL: ${mupdf}")
# else()
#     message(FATAL_ERROR
#         "MSVC 编译器需要 .lib 导入库才能链接 DLL。\n"
#         "请从 DLL 提供方获取对应的 .lib 文件并放入 dll/ 目录，\n"
#         "或在 Visual Studio Developer Command Prompt 中运行: \n"
#         "  dumpbin /exports dll/MuPDFLib.dll > exports.txt\n"
#         "然后手动创建 dll/MuPDFLib.def 并运行: \n"
#         "  lib /def:dll/MuPDFLib.def /out:dll/MuPDFLib.lib /machine:x64\n"
#         "完成后重新运行 CMake。")
endif()

# 构建后将可执行文件复制到 test 目录
add_custom_command(TARGET mupdf_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    ${CMAKE_SOURCE_DIR}/test
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:mupdf_test>
    ${CMAKE_SOURCE_DIR}/test
    COMMENT "Copying executable to test directory"
)

# 构建 mupdf 后自动复制 DLL 和 LIB 到 test/dll 目录
add_custom_command(TARGET mupdf POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/test/dll
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:mupdf>
    ${CMAKE_SOURCE_DIR}/test/
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_LINKER_FILE:mupdf>
    ${CMAKE_SOURCE_DIR}/test/dll/
    COMMENT "Copying mupdf.dll and .lib to test/dll directory"
)
# 构建后将 mupdf_api.h 复制到 test/include 目录
add_custom_command(TARGET mupdf POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/test/include
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_SOURCE_DIR}/mupdf_api.h
    ${CMAKE_SOURCE_DIR}/test/include/
    COMMENT "Copying mupdf_api.h to test/include directory"
)

# Windows: 构建后将 DLL 复制到可执行文件输出目录，确保运行时能找到
# if(WIN32)
#     add_custom_command(TARGET mupdf POST_BUILD
#         COMMAND ${CMAKE_COMMAND} -E copy_if_different
#         ${mupdf}
#         $<TARGET_FILE_DIR:mupdf>
#         COMMENT "Copying DLL to output directory"
#     )
# endif()
