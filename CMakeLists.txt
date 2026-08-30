# X-motor-Drive 顶层 CMakeLists.txt
cmake_minimum_required(VERSION 3.22)

# 从 CMakePresets 或命令行获取参数
set(BOARD    "xdr_p"      CACHE STRING "Board: xdr_p | xdr_s")
set(PLATFORM "stm32f4xx"  CACHE STRING "Platform: stm32f4xx | stm32g4xx")
set(FW_TYPE  "app"        CACHE STRING "Firmware type: app | bl")

# 项目名
set(CMAKE_PROJECT_NAME XDr)
project(${CMAKE_PROJECT_NAME} C ASM)

# C 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# 启用编译命令导出
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# 路径变量
set(BOARD_DIR    ${CMAKE_SOURCE_DIR}/board/${BOARD})
set(PLATFORM_DIR ${CMAKE_SOURCE_DIR}/platform/${PLATFORM})

# ── 判断板级是否自包含（有 Drivers/ 目录） ──
if(EXISTS ${BOARD_DIR}/Drivers)
    set(BOARD_SELF_CONTAINED TRUE)
    set(CORE_DIR ${BOARD_DIR})
    set(ARM_DSP_DIR ${BOARD_DIR})
else()
    set(BOARD_SELF_CONTAINED FALSE)
    set(CORE_DIR ${PLATFORM_DIR})
    set(ARM_DSP_DIR ${PLATFORM_DIR})
endif()

# ── 平台层（仅非自包含板需要） ──
if(NOT BOARD_SELF_CONTAINED)
    add_subdirectory(${PLATFORM_DIR})
endif()

# ── 板级层 ──
add_subdirectory(${BOARD_DIR})

# ── 固件目标 ──
if(FW_TYPE STREQUAL "bl")
    set(FW_DIR ${CMAKE_SOURCE_DIR}/bl)
    set(FW_NAME "XDr-BL")
    # BL 使用独立的链接脚本（板级）
    set(FW_LD_FILE ${CMAKE_SOURCE_DIR}/board/${BOARD}/STM32F405XX_BL.ld)
else()
    set(FW_DIR ${CMAKE_SOURCE_DIR}/app)
    set(FW_NAME "XDr")
    set(FW_LD_FILE ${CMAKE_SOURCE_DIR}/board/${BOARD}/STM32F405XX_FLASH.ld)
endif()

file(GLOB_RECURSE FW_SOURCES ${FW_DIR}/*.c)
set(MAIN_C ${CORE_DIR}/Core/Src/main.c)
if(NOT EXISTS ${MAIN_C})
    set(MAIN_C ${CORE_DIR}/core/Src/main.c)
endif()
list(APPEND FW_SOURCES ${MAIN_C})

# stm32f4xx_it.c / syscalls.c 直接编译进目标
set(CORE_IT_C ${CORE_DIR}/Core/Src/stm32f4xx_it.c)
if(NOT EXISTS ${CORE_IT_C})
    set(CORE_IT_C ${CORE_DIR}/core/Src/stm32f4xx_it.c)
endif()
if(EXISTS ${CORE_IT_C})
    list(APPEND FW_SOURCES ${CORE_IT_C})
endif()
set(CORE_SC_C ${CORE_DIR}/Core/Src/syscalls.c)
if(NOT EXISTS ${CORE_SC_C})
    set(CORE_SC_C ${CORE_DIR}/core/Src/syscalls.c)
endif()
if(EXISTS ${CORE_SC_C})
    list(APPEND FW_SOURCES ${CORE_SC_C})
endif()

add_executable(${FW_NAME} ${FW_SOURCES})

# 启动文件（板级）
set(STARTUP_FILE ${BOARD_DIR}/startup_stm32f405xx.s)
if(EXISTS ${STARTUP_FILE})
    target_sources(${FW_NAME} PRIVATE ${STARTUP_FILE})
endif()

# ── 头文件搜索路径 ──
target_include_directories(${FW_NAME} PRIVATE
    ${FW_DIR}
    ${BOARD_DIR}
    ${BOARD_DIR}/bsp
    ${BOARD_DIR}/usb
    ${CMAKE_SOURCE_DIR}/protocol
    ${CORE_DIR}/Drivers/CMSIS/Include
    ${CORE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CORE_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${CORE_DIR}/Core/Inc
    ${CORE_DIR}/Middlewares/ST/ARM/DSP/Inc
)

# APP 模式下添加 app 子目录头文件
if(FW_TYPE STREQUAL "app")
    target_include_directories(${FW_NAME} PRIVATE
        ${FW_DIR}/control
        ${FW_DIR}/communication
        ${FW_DIR}/drivers
        ${FW_DIR}/services
        ${FW_DIR}/utils
    )
endif()

# ── 链接库 ──
set(ARM_DSP_LIB ${ARM_DSP_DIR}/Middlewares/ST/ARM/DSP/Lib/libarm_cortexM4lf_math.a)
if(BOARD_SELF_CONTAINED)
    # 自包含板：所有库由板级 CMakeLists 提供
    target_link_libraries(${FW_NAME}
        -Wl,--start-group
        board_bsp
        stm32_hal
        stm32_core
        stm32_usb
        $<$<NOT:$<STREQUAL:${FW_TYPE},bl>>:${ARM_DSP_LIB}>
        $<$<NOT:$<STREQUAL:${FW_TYPE},bl>>:m>
        -Wl,--end-group
    )
else()
    # 传统方式：库由平台层提供
    target_link_libraries(${FW_NAME}
        -Wl,--start-group
        board_bsp
        stm32_hal
        stm32_core
        stm32_usb
        $<$<NOT:$<STREQUAL:${FW_TYPE},bl>>:${ARM_DSP_LIB}>
        $<$<NOT:$<STREQUAL:${FW_TYPE},bl>>:m>
        -Wl,--end-group
    )
endif()

# 编译定义
string(TOUPPER ${BOARD} BOARD_UPPER)
string(REPLACE "." "_" BOARD_MACRO "${BOARD_UPPER}")
string(TOUPPER ${FW_TYPE} FW_TYPE_UPPER)
target_compile_definitions(${FW_NAME} PRIVATE
    BOARD_${BOARD_MACRO}
    FW_TYPE_${FW_TYPE_UPPER}
    $<$<CONFIG:Debug>:__DEBUG__>
)

# 移除错误的 libob 依赖
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)
