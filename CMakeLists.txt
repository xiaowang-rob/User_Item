# ============================================================
# X-motor-Drive 顶层 CMakeLists.txt —— 重构中：库层构建入口
#
# 当前只构建新架构底层库（usr_if / usr_abs / usr_drv / usr_app / board_hw
# + 板级 HAL/Core/USB），固件可执行目标待高层（ctl/srv/通讯）迁移后恢复。
#
# 配置（toolchain 需绝对路径）：
#   cmake -S . -B build -G "Unix Makefiles" \
#     -DCMAKE_TOOLCHAIN_FILE=$PWD/board/xdr_p_o1.2/cmake/gcc-arm-none-eabi.cmake \
#     -DCMAKE_MAKE_PROGRAM=/usr/bin/make \
#     -DCMAKE_C_COMPILER=arm-none-eabi-gcc -DCMAKE_ASM_COMPILER=arm-none-eabi-gcc
#   cmake --build build --target usr_app
# ============================================================

cmake_minimum_required(VERSION 3.22)
project(XDr C ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# ── usr/if 接口契约层（纯头库：time_if / spi_if / pwm_dma_if，无 .c） ──
add_library(usr_if INTERFACE)
target_include_directories(usr_if INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR} # #include "usr/if/xxx_if.h"；hw 反向 include 的实现入口
)

# ── usr/abs 业务对象层（纯逻辑：tEncoder / tLed / tRgb / 存储单元） ──
file(GLOB ABS_SOURCES usr/abs/*.c)
add_library(usr_abs STATIC ${ABS_SOURCES})
target_link_libraries(usr_abs PUBLIC usr_if)

# ── 板级层（xdr_p_o1.2：stm32_hal / stm32_core / stm32_usb / board_hw） ──
add_subdirectory(board/xdr_p_o1.2)

# ── usr/drv 芯片协议驱动层 ──
file(GLOB DRV_SOURCES usr/drv/*.c)
add_library(usr_drv STATIC ${DRV_SOURCES})
target_link_libraries(usr_drv PUBLIC usr_if)

# ── 组装层：把板上资源+芯片实现装配成 g_dev（usr/app/dev_board.c） ──
# 注意：usr/app 下的旧高层文件（app_main/can_port/...）尚未迁移，暂不编入；
# 高层迁移完成后按需扩展本目标源列表。
set(APP_SOURCES usr/app/dev_board.c)
add_library(usr_app STATIC ${APP_SOURCES})
target_link_libraries(usr_app PUBLIC usr_abs usr_drv board_hw)
