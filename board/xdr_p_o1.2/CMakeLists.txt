# XDr-P 板级 CMakeLists.txt
# BSP 实现 + 板级配置 + USB描述符 + 启动文件 + 链接脚本

file(GLOB BSP_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/bsp/*.c)

# 板级 USB 描述符（含 VID/PID）
set(BOARD_USB_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/usb/usbd_desc.c)

# 板级启动文件
set(BOARD_STARTUP ${CMAKE_CURRENT_SOURCE_DIR}/startup_stm32f405xx.s)

add_library(board_bsp STATIC ${BSP_SOURCES} ${BOARD_USB_SOURCES} ${BOARD_STARTUP})
target_include_directories(board_bsp PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}          # board_config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/bsp      # BSP 头文件
    ${CMAKE_CURRENT_SOURCE_DIR}/usb      # USB 描述符
    ${PLATFORM_DIR}/core/Inc             # CubeMX 外设头文件
    ${PLATFORM_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${PLATFORM_DIR}/Drivers/CMSIS/Include
    ${PLATFORM_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${PLATFORM_DIR}/Middlewares/ST/STM32_USB_Device_Library/Core/Inc
    ${PLATFORM_DIR}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
    ${PLATFORM_DIR}/USB_DEVICE/App
    ${PLATFORM_DIR}/USB_DEVICE/Target
    ${CMAKE_SOURCE_DIR}/protocol
)
target_link_libraries(board_bsp platform)
