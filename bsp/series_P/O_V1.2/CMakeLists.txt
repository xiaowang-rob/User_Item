cmake_minimum_required(VERSION 3.20)
project(bsp_P_O_V1_2 LANGUAGES C ASM)

if(NOT DEFINED FIRMWARE_TYPE)
    set(FIRMWARE_TYPE "app")
endif()

add_subdirectory(cubemx)

# ── 收集 CubeMX 源文件 ──
file(GLOB_RECURSE CUBEMX_PERIPH_SRCS 
    ${CMAKE_CURRENT_SOURCE_DIR}/cubemx/Core/Src/*.c 
    ${CMAKE_CURRENT_SOURCE_DIR}/cubemx/USB_DEVICE/App/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/cubemx/USB_DEVICE/Target/*.c
)
list(FILTER CUBEMX_PERIPH_SRCS EXCLUDE REGEX "/main\\.c$")

if(FIRMWARE_TYPE STREQUAL "bl")
    list(FILTER CUBEMX_PERIPH_SRCS INCLUDE REGEX
        "(gpio\\.c|tim\\.c|system_stm32f4xx\\.c|stm32f4xx_it\\.c|stm32f4xx_hal_msp\\.c|usb)"
    )
    message(STATUS "Bootloader 模式，仅编译基础外设源文件")
else()
    message(STATUS "App 模式，编译全部外设源文件")
endif()

set(BSP_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_adc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_led.c
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp_flash.c
    ${CUBEMX_PERIPH_SRCS}
)

add_library(bsp STATIC ${BSP_SOURCES})

target_include_directories(bsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(bsp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cubemx/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/cubemx/Drivers/STM32F4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/cubemx/Drivers/CMSIS/Include
)

# 启动文件 OBJECT 库（保持不变）
set(STARTUP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cubemx/startup_stm32f405xx.s")
if(NOT EXISTS ${STARTUP_FILE})
    message(FATAL_ERROR "未找到启动文件: ${STARTUP_FILE}")
endif()
add_library(bsp_startup OBJECT ${STARTUP_FILE})
target_include_directories(bsp_startup PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/cubemx/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/cubemx/Drivers/CMSIS/Include
)
target_link_libraries(bsp PUBLIC bsp_startup)

target_link_libraries(bsp PUBLIC
    stm32cubemx
    STM32_Drivers
    USB_Device_Library
    dsp_math
    m
)

add_library(dsp_math STATIC IMPORTED)
set_target_properties(dsp_math PROPERTIES
    IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/cubemx/Middlewares/ST/ARM/DSP/Lib/libarm_cortexM4lf_math.a
)

if(FIRMWARE_TYPE STREQUAL "bl")
    target_link_options(bsp PUBLIC -T ${CMAKE_CURRENT_SOURCE_DIR}/linker_bl.ld)
else()
    target_link_options(bsp PUBLIC -T ${CMAKE_CURRENT_SOURCE_DIR}/linker_app.ld)
endif()

target_compile_options(bsp PRIVATE -Wall -Werror)