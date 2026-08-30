# XDr-P O1.2 板级 CMakeLists.txt (自包含)
# HAL 库 + CubeMX 外设初始化 + USB 中间件 + BSP + 启动文件 + 链接脚本

# ── HAL 库 ──
file(GLOB_RECURSE HAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/*.c
)
add_library(stm32_hal STATIC ${HAL_SOURCES})
target_include_directories(stm32_hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
)
target_compile_definitions(stm32_hal PUBLIC
    USE_HAL_DRIVER
    STM32F405xx
    $<$<CONFIG:Debug>:DEBUG>
)

# ── CubeMX 外设初始化 ──
file(GLOB CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/*.c
)
add_library(stm32_core STATIC ${CORE_SOURCES})
target_include_directories(stm32_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/Target
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
)
target_link_libraries(stm32_core PUBLIC stm32_hal)

# ── USB Device 库 ──
file(GLOB USB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/Target/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Core/Src/*.c
)
# usbd_desc.c 由板级提供 (usb/usbd_desc.c)，排除 CubeMX 默认的
list(FILTER USB_SOURCES EXCLUDE REGEX "usbd_desc\\.c$")
add_library(stm32_usb STATIC ${USB_SOURCES})
target_include_directories(stm32_usb PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/Target
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Core/Inc
)
target_link_libraries(stm32_usb PUBLIC stm32_hal)

# ── BSP ──
file(GLOB BSP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/bsp/*.c)

# 板级 USB 描述符（含 VID/PID，覆盖 CubeMX 默认的）
set(BOARD_USB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/usb/usbd_desc.c)

# 板级启动文件
set(BOARD_STARTUP ${CMAKE_CURRENT_SOURCE_DIR}/startup_stm32f405xx.s)

add_library(board_bsp STATIC ${BSP_SOURCES} ${BOARD_USB_SOURCES} ${BOARD_STARTUP})
target_include_directories(board_bsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}          # board_config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp      # BSP 头文件
    ${CMAKE_CURRENT_SOURCE_DIR}/usb      # USB 描述符
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/Target
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/ARM/DSP/Inc
    ${CMAKE_SOURCE_DIR}/protocol
)
target_link_libraries(board_bsp stm32_hal)
