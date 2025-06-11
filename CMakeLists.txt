cmake_minimum_required(VERSION 3.16)
project(SharedMemoryLogTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 针对不同编译器添加 UTF-8 选项
if(MSVC)
    add_compile_options(/utf-8)
else()
    add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 创建DLL
add_library(MainLogDll SHARED
    LogDll.cpp
    LogDll.h
)

# 设置DLL导出宏
target_compile_definitions(MainLogDll PRIVATE MAINLOGDLL_EXPORTS)

# 链接Windows库
target_link_libraries(MainLogDll PRIVATE)

# 创建日志读取程序（主程序）
add_executable(LogReader
    LogReader.cpp
)

# 创建日志写入程序（仿真程序）
add_executable(LogWriter
    LogWriter.cpp
)

# 链接DLL
target_link_libraries(LogReader PRIVATE MainLogDll)
target_link_libraries(LogWriter PRIVATE MainLogDll)

# 设置调试信息
if(MSVC)
    target_compile_options(MainLogDll PRIVATE /W4)
    target_compile_options(LogReader PRIVATE /W4)
    target_compile_options(LogWriter PRIVATE /W4)
endif()

# 复制DLL到可执行文件目录
add_custom_command(TARGET LogReader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:MainLogDll>
    $<TARGET_FILE_DIR:LogReader>
)

add_custom_command(TARGET LogWriter POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:MainLogDll>
    $<TARGET_FILE_DIR:LogWriter>
)
