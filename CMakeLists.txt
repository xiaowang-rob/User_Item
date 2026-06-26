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

# ── 平台层（SDK + HAL库） ──
add_subdirectory(${PLATFORM_DIR})

# ── 板级层（BSP 实现） ──
add_subdirectory(${BOARD_DIR})

# ── 固件目标 ──
if(FW_TYPE STREQUAL "bl")
    set(FW_DIR ${CMAKE_SOURCE_DIR}/bl)
    set(FW_NAME "XDr-BL")
    # BL 不需要 ARM DSP 库
    unset(ARM_DSP_LIB)
    set(ARM_DSP_LIB "")
    # BL 使用独立的链接脚本（板级）
    set(FW_LD_FILE ${CMAKE_SOURCE_DIR}/board/${BOARD}/STM32F405XX_BL.ld)
else()
    set(FW_DIR ${CMAKE_SOURCE_DIR}/app)
    set(FW_NAME "XDr")
    set(FW_LD_FILE ${CMAKE_SOURCE_DIR}/board/${BOARD}/STM32F405XX_FLASH.ld)
endif()

file(GLOB_RECURSE FW_SOURCES ${FW_DIR}/*.c)
set(MAIN_C ${PLATFORM_DIR}/core/Src/main.c)
list(APPEND FW_SOURCES ${MAIN_C})
# 强制把 stm32f4xx_it.c 直接编译进目标（不经过静态库），
# 因为启动文件提供了 SysTick_Handler 的 weak alias，
# 链接器不会从 libstm32_core.a 中拉取强定义
set(CORE_IT_C ${PLATFORM_DIR}/core/Src/stm32f4xx_it.c)
if(EXISTS ${CORE_IT_C})
  list(APPEND FW_SOURCES ${CORE_IT_C})
endif()
add_executable(${FW_NAME} ${FW_SOURCES})

# 启动文件（板级）
set(STARTUP_FILE ${BOARD_DIR}/startup_stm32f405xx.s)
if(EXISTS ${STARTUP_FILE})
    target_sources(${FW_NAME} PRIVATE ${STARTUP_FILE})
endif()

# 链接脚本（由工具链文件处理）
# set(LD_FILE ${CMAKE_SOURCE_DIR}/platform/${PLATFORM}/STM32F405XX_FLASH.ld)
# if(EXISTS ${LD_FILE})
#     target_link_options(${FW_NAME} PRIVATE -T${LD_FILE})
#     message(STATUS "链接脚本: ${LD_FILE}")
# endif()

# 头文件搜索路径
target_include_directories(${FW_NAME} PRIVATE
    ${FW_DIR}
    ${BOARD_DIR}
    ${BOARD_DIR}/bsp
    ${BOARD_DIR}/usb
    ${CMAKE_SOURCE_DIR}/protocol
    ${PLATFORM_DIR}/Drivers/CMSIS/Include
    ${PLATFORM_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${PLATFORM_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${PLATFORM_DIR}/core/Inc
    ${PLATFORM_DIR}/Middlewares/ST/ARM/DSP/Inc
)

# APP 模式下添加 app 子目录头文件搜索路径
if(FW_TYPE STREQUAL "app")
    target_include_directories(${FW_NAME} PRIVATE
        ${FW_DIR}/control
        ${FW_DIR}/communication
        ${FW_DIR}/drivers
        ${FW_DIR}/services
        ${FW_DIR}/utils
    )
endif()

# 链接库（处理循环依赖 + ARM DSP库 + libm）
set(ARM_DSP_LIB ${CMAKE_SOURCE_DIR}/platform/${PLATFORM}/Middlewares/ST/ARM/DSP/Lib/libarm_cortexM4lf_math.a)
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

# 编译定义：BOARD 宏中替换 . 为 _（避免预处理器报错）
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
