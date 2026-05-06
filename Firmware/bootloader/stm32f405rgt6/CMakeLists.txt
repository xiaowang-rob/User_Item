cmake_minimum_required(VERSION 3.22)

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler flexibility
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME XDr)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable C support for ASM and C languages
enable_language(C ASM)

# ========================================
# 1. 将顶层目标改为可执行文件（与子目录保持一致）
# ========================================
add_executable(${CMAKE_PROJECT_NAME})

# ========================================
# 2. 添加 STM32CubeMX 子目录（包含所有 HAL/CMSIS/startup）
# ========================================
add_subdirectory(cmake/stm32cubemx)

# ========================================
# 3. BSP 静态库
# ========================================
file(GLOB BSP_SOURCES
    ${CMAKE_SOURCE_DIR}/bsp/*.c
)
add_library(bsp STATIC ${BSP_SOURCES})
target_include_directories(bsp PUBLIC ${CMAKE_SOURCE_DIR}/bsp)

# ========================================
# 4. 应用层源文件（usr 下各子目录 + 根文件）
# ========================================
file(GLOB_RECURSE USR_SOURCES
    ${CMAKE_SOURCE_DIR}/usr/*.c
)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${USR_SOURCES})

# 添加头文件搜索路径（应用层各子目录）
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
   
)

# ========================================
# 5. 链接 BSP 库
# ========================================
target_link_libraries(${CMAKE_PROJECT_NAME} bsp)
target_link_libraries(bsp stm32cubemx)

# ========================================
# 6. 其他你原本的配置保留
# ========================================
# Add project symbols (macros) if needed
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# add linked libraries already handled in subdirectory (stm32cubemx)
# No extra link libraries needed unless you have something else