cmake_minimum_required(VERSION 3.15)
project(mupdf_test C)

set(CMAKE_C_STANDARD 11)

# MuPDF 头文件路径 (dll 目录中包含 mupdf_api.h)
set(MUPDF_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")

# DLL 导入库路径
set(MUPDF_LIB_DIR "${CMAKE_SOURCE_DIR}/import_lib")

# 添加测试可执行文件
add_executable(mupdf_test test.c)

# 包含头文件目录
target_include_directories(mupdf_test PRIVATE ${MUPDF_INCLUDE_DIR})

# 链接 DLL 的导入库 (Windows)
if(WIN32)
    # 使用 LoadLibrary 动态加载 DLL，不需要链接 .lib
    # 如果需要链接导入库，取消下面这行的注释
    target_link_directories(mupdf_test PRIVATE ${MUPDF_LIB_DIR})
    target_link_libraries(mupdf_test mupdf)
endif()

# copy exe to source directory for easy execution
add_custom_command(
    TARGET mupdf_test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:mupdf_test> ${CMAKE_SOURCE_DIR}
)

