cmake_minimum_required(VERSION 3.24)
project(harborRPC VERSION 1.0.0 LANGUAGES CXX)

# ============================================================
# 全局设置
# ============================================================
set_property(GLOBAL PROPERTY USE_FOLDERS On)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "CMakeTargets")

# C++标准设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 字符集unicode
add_definitions(-DUNICODE -D_UNICODE)

# MSVC: force UTF-8 source and execution character set
if(MSVC)
    add_compile_options(/utf-8)
    add_compile_options(/EHsc)  # 启用C++异常展开语义
endif()

# Find includes in the build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# ============================================================
# 源文件定义
# ============================================================
set(src_files
    src/harbor.cpp
    src/server.cpp
)

set(inc_files
    include/harbor_rpc.h
    include/harbor_rpc/defines.h
    include/harbor_rpc/harbor.h
    include/harbor_rpc/server.h
    include/harbor_rpc/client.h
    include/harbor_rpc/util.h
)

set(test_files
    test/test.cpp
    test/example_rpc.cpp
    test/example_rpc.h
)

source_group("include" FILES ${inc_files})
source_group("implement" FILES ${src_files})
source_group("test" FILES ${test_files})

# ============================================================
# 库目标
# ============================================================
add_library(${PROJECT_NAME}
    ${src_files}    
    ${inc_files}
)

target_include_directories(${PROJECT_NAME}
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# 设置库属性
set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER "${inc_files}"
)

# ============================================================
# 测试目标
# ============================================================
add_executable(harbor_rpc_test
    ${test_files}
)

target_link_libraries(harbor_rpc_test
    PRIVATE harborRPC
)

set_target_properties(harbor_rpc_test PROPERTIES
    LINK_FLAGS "/SUBSYSTEM:CONSOLE"
)

# 启用测试
enable_testing()

add_test(NAME harbor_rpc_test
    COMMAND harbor_rpc_test
)

# ============================================================
# 安装配置
# ============================================================
include(GNUInstallDirs)

install(TARGETS harborRPC
    EXPORT harborRPC-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/harbor_rpc
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# 导出配置
install(EXPORT harborRPC-targets
    FILE harborRPC-config.cmake
    NAMESPACE harborRPC::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/harborRPC
)

# ============================================================
# 打印配置信息
# ============================================================
message(STATUS "")
message(STATUS "======== harborRPC 配置信息 ========")
message(STATUS "  版本:        ${PROJECT_VERSION}")
message(STATUS "  C++ 标准:    ${CMAKE_CXX_STANDARD}")
message(STATUS "  安装路径:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "=====================================")
message(STATUS "")
