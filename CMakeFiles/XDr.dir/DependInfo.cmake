
# Consider dependencies only in project.
set(CMAKE_DEPENDS_IN_PROJECT_ONLY OFF)

# The set of languages for which implicit dependencies are needed:
set(CMAKE_DEPENDS_LANGUAGES
  "ASM"
  )
# The set of files for implicit dependencies of each language:
set(CMAKE_DEPENDS_CHECK_ASM
  "/data/Github/X-motor-Drive/board/xdr_p_o1.2/startup_stm32f405xx.s" "/data/Github/X-motor-Drive/CMakeFiles/XDr.dir/board/xdr_p_o1.2/startup_stm32f405xx.s.o"
  )
set(CMAKE_ASM_COMPILER_ID "GNU")

# Preprocessor definitions for this target.
set(CMAKE_TARGET_DEFINITIONS_ASM
  "BOARD_XDR_P_O1_2"
  "DEBUG"
  "FW_TYPE_APP"
  "STM32F405xx"
  "USE_HAL_DRIVER"
  "__DEBUG__"
  )

# The include file search paths:
set(CMAKE_ASM_TARGET_INCLUDE_PATH
  "app"
  "board/xdr_p_o1.2"
  "board/xdr_p_o1.2/bsp"
  "board/xdr_p_o1.2/usb"
  "protocol"
  "board/xdr_p_o1.2/Drivers/CMSIS/Include"
  "board/xdr_p_o1.2/Drivers/CMSIS/Device/ST/STM32F4xx/Include"
  "board/xdr_p_o1.2/Drivers/STM32F4xx_HAL_Driver/Inc"
  "board/xdr_p_o1.2/Core/Inc"
  "board/xdr_p_o1.2/Middlewares/ST/ARM/DSP/Inc"
  "app/control"
  "app/communication"
  "app/drivers"
  "app/services"
  "app/utils"
  "board/xdr_p_o1.2/Middlewares/ST/STM32_USB_Device_Library/Core/Inc"
  "board/xdr_p_o1.2/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc"
  "board/xdr_p_o1.2/USB_DEVICE/App"
  "board/xdr_p_o1.2/USB_DEVICE/Target"
  "board/xdr_p_o1.2/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy"
  )

# The set of dependency files which are needed:
set(CMAKE_DEPENDS_DEPENDENCY_FILES
  "/data/Github/X-motor-Drive/app/app_main.c" "CMakeFiles/XDr.dir/app/app_main.c.o" "gcc" "CMakeFiles/XDr.dir/app/app_main.c.o.d"
  "/data/Github/X-motor-Drive/app/communication/DataMonitoring.c" "CMakeFiles/XDr.dir/app/communication/DataMonitoring.c.o" "gcc" "CMakeFiles/XDr.dir/app/communication/DataMonitoring.c.o.d"
  "/data/Github/X-motor-Drive/app/communication/can_port.c" "CMakeFiles/XDr.dir/app/communication/can_port.c.o" "gcc" "CMakeFiles/XDr.dir/app/communication/can_port.c.o.d"
  "/data/Github/X-motor-Drive/app/communication/port_mapping.c" "CMakeFiles/XDr.dir/app/communication/port_mapping.c.o" "gcc" "CMakeFiles/XDr.dir/app/communication/port_mapping.c.o.d"
  "/data/Github/X-motor-Drive/app/communication/uart_port.c" "CMakeFiles/XDr.dir/app/communication/uart_port.c.o" "gcc" "CMakeFiles/XDr.dir/app/communication/uart_port.c.o.d"
  "/data/Github/X-motor-Drive/app/communication/usb_port.c" "CMakeFiles/XDr.dir/app/communication/usb_port.c.o" "gcc" "CMakeFiles/XDr.dir/app/communication/usb_port.c.o.d"
  "/data/Github/X-motor-Drive/app/control/foc_core.c" "CMakeFiles/XDr.dir/app/control/foc_core.c.o" "gcc" "CMakeFiles/XDr.dir/app/control/foc_core.c.o.d"
  "/data/Github/X-motor-Drive/app/control/foc_main.c" "CMakeFiles/XDr.dir/app/control/foc_main.c.o" "gcc" "CMakeFiles/XDr.dir/app/control/foc_main.c.o.d"
  "/data/Github/X-motor-Drive/app/control/hfi.c" "CMakeFiles/XDr.dir/app/control/hfi.c.o" "gcc" "CMakeFiles/XDr.dir/app/control/hfi.c.o.d"
  "/data/Github/X-motor-Drive/app/control/loop_control.c" "CMakeFiles/XDr.dir/app/control/loop_control.c.o" "gcc" "CMakeFiles/XDr.dir/app/control/loop_control.c.o.d"
  "/data/Github/X-motor-Drive/app/control/mit.c" "CMakeFiles/XDr.dir/app/control/mit.c.o" "gcc" "CMakeFiles/XDr.dir/app/control/mit.c.o.d"
  "/data/Github/X-motor-Drive/app/control/smo.c" "CMakeFiles/XDr.dir/app/control/smo.c.o" "gcc" "CMakeFiles/XDr.dir/app/control/smo.c.o.d"
  "/data/Github/X-motor-Drive/app/control/svpwm.c" "CMakeFiles/XDr.dir/app/control/svpwm.c.o" "gcc" "CMakeFiles/XDr.dir/app/control/svpwm.c.o.d"
  "/data/Github/X-motor-Drive/app/control/tune.c" "CMakeFiles/XDr.dir/app/control/tune.c.o" "gcc" "CMakeFiles/XDr.dir/app/control/tune.c.o.d"
  "/data/Github/X-motor-Drive/app/drivers/encoder.c" "CMakeFiles/XDr.dir/app/drivers/encoder.c.o" "gcc" "CMakeFiles/XDr.dir/app/drivers/encoder.c.o.d"
  "/data/Github/X-motor-Drive/app/drivers/flashDr.c" "CMakeFiles/XDr.dir/app/drivers/flashDr.c.o" "gcc" "CMakeFiles/XDr.dir/app/drivers/flashDr.c.o.d"
  "/data/Github/X-motor-Drive/app/drivers/led.c" "CMakeFiles/XDr.dir/app/drivers/led.c.o" "gcc" "CMakeFiles/XDr.dir/app/drivers/led.c.o.d"
  "/data/Github/X-motor-Drive/app/services/log.c" "CMakeFiles/XDr.dir/app/services/log.c.o" "gcc" "CMakeFiles/XDr.dir/app/services/log.c.o.d"
  "/data/Github/X-motor-Drive/app/services/parameter_manager.c" "CMakeFiles/XDr.dir/app/services/parameter_manager.c.o" "gcc" "CMakeFiles/XDr.dir/app/services/parameter_manager.c.o.d"
  "/data/Github/X-motor-Drive/app/services/protection_manager.c" "CMakeFiles/XDr.dir/app/services/protection_manager.c.o" "gcc" "CMakeFiles/XDr.dir/app/services/protection_manager.c.o.d"
  "/data/Github/X-motor-Drive/app/services/status_feedback.c" "CMakeFiles/XDr.dir/app/services/status_feedback.c.o" "gcc" "CMakeFiles/XDr.dir/app/services/status_feedback.c.o.d"
  "/data/Github/X-motor-Drive/app/utils/filter.c" "CMakeFiles/XDr.dir/app/utils/filter.c.o" "gcc" "CMakeFiles/XDr.dir/app/utils/filter.c.o.d"
  "/data/Github/X-motor-Drive/app/utils/math_fast.c" "CMakeFiles/XDr.dir/app/utils/math_fast.c.o" "gcc" "CMakeFiles/XDr.dir/app/utils/math_fast.c.o.d"
  "/data/Github/X-motor-Drive/app/utils/queue.c" "CMakeFiles/XDr.dir/app/utils/queue.c.o" "gcc" "CMakeFiles/XDr.dir/app/utils/queue.c.o.d"
  "/data/Github/X-motor-Drive/app/utils/trajectory.c" "CMakeFiles/XDr.dir/app/utils/trajectory.c.o" "gcc" "CMakeFiles/XDr.dir/app/utils/trajectory.c.o.d"
  "/data/Github/X-motor-Drive/board/xdr_p_o1.2/Core/Src/main.c" "CMakeFiles/XDr.dir/board/xdr_p_o1.2/Core/Src/main.c.o" "gcc" "CMakeFiles/XDr.dir/board/xdr_p_o1.2/Core/Src/main.c.o.d"
  "/data/Github/X-motor-Drive/board/xdr_p_o1.2/Core/Src/stm32f4xx_it.c" "CMakeFiles/XDr.dir/board/xdr_p_o1.2/Core/Src/stm32f4xx_it.c.o" "gcc" "CMakeFiles/XDr.dir/board/xdr_p_o1.2/Core/Src/stm32f4xx_it.c.o.d"
  "/data/Github/X-motor-Drive/board/xdr_p_o1.2/Core/Src/syscalls.c" "CMakeFiles/XDr.dir/board/xdr_p_o1.2/Core/Src/syscalls.c.o" "gcc" "CMakeFiles/XDr.dir/board/xdr_p_o1.2/Core/Src/syscalls.c.o.d"
  )

# Targets to which this target links which contain Fortran sources.
set(CMAKE_Fortran_TARGET_LINKED_INFO_FILES
  )

# Targets to which this target links which contain Fortran sources.
set(CMAKE_Fortran_TARGET_FORWARD_LINKED_INFO_FILES
  )

# Fortran module output directory.
set(CMAKE_Fortran_TARGET_MODULE_DIR "")
