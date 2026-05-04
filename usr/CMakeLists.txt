cmake_minimum_required(VERSION 3.20)
project(XDr_App)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(__DEBUG__)
endif()

# ── 参数 ──
if(NOT BSP)
    message(FATAL_ERROR "请设置 -DBSP=series_P/O_V1.2")
endif()
set(FIRMWARE_TYPE "app" CACHE STRING "固件类型: app 或 bl")
set(BSP_PATH ${CMAKE_SOURCE_DIR}/../bsp/${BSP})

# 输出目录（将所有产物统一放到 build 下）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})

# ── 引入 BSP（自动带入 CubeMX、HAL、USB、DSP，以及链接脚本） ──
add_subdirectory(${BSP_PATH} ${CMAKE_BINARY_DIR}/bsp)


# ── 入口文件 ──
if(FIRMWARE_TYPE STREQUAL "bl")
    set(APP_ENTRY ${CMAKE_SOURCE_DIR}/bootload/bl_main.c)
else()
    set(APP_ENTRY ${CMAKE_SOURCE_DIR}/app_main.c)
endif()

# ── FOC 源文件（仅 App 模式） ──
set(FOC_SOURCES "")
if(FIRMWARE_TYPE STREQUAL "app")
    file(GLOB_RECURSE FOC_SOURCES "foc/*.c")
    list(APPEND FOC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/syscalls.c)
endif()


# ── 可执行文件 ──
add_executable(firmware ${APP_ENTRY} ${FOC_SOURCES})

# ── 包含路径（USR 自己的头文件） ──
target_include_directories(firmware PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/foc/control
    ${CMAKE_CURRENT_SOURCE_DIR}/foc/services
    ${CMAKE_CURRENT_SOURCE_DIR}/foc/drivers
    ${CMAKE_CURRENT_SOURCE_DIR}/foc/communication
    ${CMAKE_CURRENT_SOURCE_DIR}/foc/utils
)

# ── 链接 BSP（BSP 已通过 PUBLIC 传递链接脚本、HAL 库、头文件等） ──
target_link_libraries(firmware PRIVATE bsp)