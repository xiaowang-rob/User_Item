# STM32F4xx 平台层 CMakeLists.txt
# 包含 HAL 库 + CubeMX 生成的外设初始化

# HAL 库源文件
file(GLOB_RECURSE HAL_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Src/*.c
)
add_library(stm32_hal STATIC ${HAL_SOURCES})
target_include_directories(stm32_hal PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Inc
)
target_compile_definitions(stm32_hal PUBLIC
    USE_HAL_DRIVER
    STM32F405xx
    $<$<CONFIG:Debug>:DEBUG>
)

# CubeMX 外设初始化
file(GLOB CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Src/*.c
)
add_library(stm32_core STATIC ${CORE_SOURCES})
target_include_directories(stm32_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/Target
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc)
target_link_libraries(stm32_core PUBLIC stm32_hal)

# USB Device 库
file(GLOB USB_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/Target/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Core/Src/*.c
)
# 排除 usbd_desc.c（板级提供）
list(FILTER USB_SOURCES EXCLUDE REGEX "usbd_desc\\.c$")
add_library(stm32_usb STATIC ${USB_SOURCES})
target_include_directories(stm32_usb PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App
    ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/Target
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Middlewares/ST/STM32_USB_Device_Library/Core/Inc
)
target_link_libraries(stm32_usb stm32_hal)

# 聚合库（接口库，不编译源文件）
add_library(platform INTERFACE)
target_link_libraries(platform INTERFACE stm32_hal stm32_core stm32_usb)

# main.c 由固件目标直接编译（需要 FW_TYPE 宏）
set(CORE_EXCLUDE main.c)
list(FILTER CORE_SOURCES EXCLUDE REGEX "main\\.c$")
