#!/usr/bin/env python3
"""批量重命名 usr/ 下的函数为 snake_case 风格"""
import re, os, sys

# 旧名 → 新名 映射
RENAMES = {
    # foc_main
    "fFocInit": "foc_init",
    "fFocStateUpdate": "foc_state_update",
    "BSP_CurrentSampleISR": "bsp_current_sample_isr",
    "BSP_FOC_ITCallback": "bsp_foc_it_callback",
    # encoder
    "fEncoder_Init": "encoder_init",
    "fEncoderMainLoopTask": "encoder_main_loop_task",
    "fGetEncoderAngle_ABS": "encoder_get_angle_abs",
    "fGetEncoderAngle_INC": "encoder_get_angle_inc",
    "fGetEncoderRPM": "encoder_get_rpm",
    "fGetEncoderNumTurns": "encoder_get_num_turns",
    "fSetEncoderAngleZero": "encoder_set_angle_zero",
    "fMotorParamTuneInit": "motor_param_tune_init",
    "fMotorParamTuneReset": "motor_param_tune_reset",
    "fMotorParamTuneForceSave": "motor_param_tune_force_save",
    "fMotorParamTuneGetProgress": "motor_param_tune_get_progress",
    # can
    "fCAN_PortInit": "can_port_init",
    "fCAN_SendData": "can_send_data",
    "fCAN_RxDataCallback": "can_rx_data_callback",
    "fCAN_QueueData_deal": "can_queue_data_deal",
    "fCAN_SetConfig": "can_set_config",
    # communication
    "fCommunicateInit": "comm_init",
    "fCommunicateMainLoop": "comm_main_loop",
    "fHostComputer_send": "comm_host_send",
    "fStreamDataGet": "stream_data_get",
    "fStreamDataPrepare": "stream_data_prepare",
    "fStatusFeedbackMainLoop": "status_feedback_main_loop",
    "fSystemFaultFeedback": "system_fault_feedback",
    "fVOFA_FloatDataSend": "vofa_float_data_send",
    # usb
    "fUSB_Init": "usb_init",
    "fUSB_SendData": "usb_send_data",
    "fUSB_SendFrame": "usb_send_frame",
    "fUSB_RxFrameCallback": "usb_rx_frame_callback",
    # uart
    "fUartPortInit": "uart_port_init",
    "fUartPortSendData": "uart_port_send_data",
    "fUartPortSendFrame": "uart_port_send_frame",
    "fUartReviceByte": "uart_receive_byte",
    "fUartRxFrameCallback": "uart_rx_frame_callback",
    # flash
    "fFLASH_Init": "flash_init",
    "fFLASH_ReadData": "flash_read_data",
    "fFLASH_WriteWord": "flash_write_word",
    "fFLASH_EraseSector": "flash_erase_sector",
    "fEraseOneSector": "flash_erase_one_sector",
    "fEraseFLASHChip": "flash_erase_chip",
    # protection
    "fProManagerInit": "pro_manager_init",
    "fProManagerClearFlag": "pro_manager_clear_flag",
    "fProManagerMainLoop": "pro_manager_main_loop",
    "fProSetLimitPosition": "pro_set_limit_position",
    # log
    "fLogInit": "log_init",
    "fLogDataSave": "log_data_save",
    "fLogDataWrite": "log_data_write",
    "fLogErase": "log_erase",
    "fLogReadFlash": "log_read_flash",
    # parameter
    "fParamInit": "param_init",
    "fParamSave": "param_save",
    "fParamGet": "param_get",
    "fParamSet": "param_set",
    "fParamErase": "param_erase",
    # svpwm
    "fSvpwmInit": "svpwm_init",
    "fSvpwmRun": "svpwm_run",
    "fSvpwmSetVbus": "svpwm_set_vbus",
    "fSvpwmGetSector": "svpwm_get_sector",
    "fSamplePointCalibration": "svpwm_sample_point_calibration",
    # loop control
    "fLoopControlInit": "loop_control_init",
    "fLoopReset": "loop_control_reset",
    "fCurrentLoopUpdate": "loop_current_update",
    "fSpeedLoopUpdate": "loop_speed_update",
    "fPositionRelLoopUpdate": "loop_position_update",
    "fMagLoopUpdate": "loop_mag_update",
    "fWeakMagLoopUpdate": "loop_weak_mag_update",
    # frequency division
    "fFrequencyDivisionInit": "freq_div_init",
    "fFrequencyDivisionUpdate": "freq_div_update",
    # mit
    "fMIT_Init": "mit_init",
    "fMIT_LoopUpdate": "mit_loop_update",
    "fMIT_ConfigDynamic": "mit_config_dynamic",
    "fMIT_ConfigStatic": "mit_config_static",
    "fMIT_ConfigTFF": "mit_config_tff",
    # hfi
    "fHfiInit": "hfi_init",
    "fHfiStep": "hfi_step",
    "fHfiDetectInitialPosition": "hfi_detect_initial_position",
    "fHfiResetInitialPosition": "hfi_reset_initial_position",
    "fHfiGetStatus": "hfi_get_status",
    "fHfiGetOmegaElec": "hfi_get_omega_elec",
    "fHfiGetThetaElec": "hfi_get_theta_elec",
    # smo
    "fSmoInit": "smo_init",
    "fSmoReset": "smo_reset",
    "fSmoMainLoop": "smo_main_loop",
    "fSmoSetConfig": "smo_set_config",
    # trajectory
    "fTraj_Init": "traj_init",
    "fTraj_Reset": "traj_reset",
    "fTraj_SetTarget": "traj_set_target",
    "fTraj_SetRate": "traj_set_rate",
    # misc
    "fCalculateControlParams": "calculate_control_params",
    "fLED_Control": "led_control",
    # filters
    "fFirstOrderLagInit": "filter_first_order_lag_init",
    "fFirstOrderLagFilter": "filter_first_order_lag",
    "fKalmanInit": "filter_kalman_init",
    "fKalmanFilter": "filter_kalman",
    "fMedianFilterInit": "filter_median_init",
    "fMedianFilter": "filter_median",
    "fMovingAverageInit": "filter_moving_avg_init",
    "fWeightedMovingAverageInit": "filter_weighted_moving_avg_init",
    "fWeightedMovingAverageFilter": "filter_weighted_moving_avg",
    "fAmplitudeLimitingInit": "filter_amplitude_limiting_init",
    "fAmplitudeLimitingFilter": "filter_amplitude_limiting",
    "fPulseInterferenceInit": "filter_pulse_init",
    "fPulseInterferenceFilter": "filter_pulse",
    "fButterworthFilter_Init": "filter_butterworth_init",
    "fButterworthFilter_Reset": "filter_butterworth_reset",
    # queue
    "fStaticQueueIsEmpty": "queue_is_empty",
    "fStaticQueueIsFull": "queue_is_full",
    "fStaticQueueCount": "queue_count",
    "fStaticQueueRemaining": "queue_remaining",
    "fStaticQueueClear": "queue_clear",
    # auto calibration
    "fAutoCalibrationUpdate": "auto_calibration_update",
    # bsp callbacks (keep BSP_ prefix, just fix style)
    "BSP_CanRxCallback": "bsp_can_rx_callback",
    "BSP_Encoder_SPI_ErrorCallback": "bsp_encoder_spi_error_callback",
    "BSP_Encoder_SPI_TxRxCpltCallback": "bsp_encoder_spi_txrx_cplt_callback",
    "BSP_UART_RxCallback": "bsp_uart_rx_callback",
    # USB recv weak
    "BSP_USB_RecvByte": "bsp_usb_recv_byte",
}

def rename_in_file(filepath, renames):
    with open(filepath, 'r') as f:
        content = f.read()
    
    original = content
    for old, new in renames.items():
        # 使用 word boundary 替换，避免误改子串
        content = re.sub(r'\b' + re.escape(old) + r'\b', new, content)
    
    if content != original:
        with open(filepath, 'w') as f:
            f.write(content)
        return True
    return False

def main():
    base_dir = os.path.join(os.path.dirname(__file__), '..', 'Firmware', 'series_P', 'O_V1.2')
    base_dir = os.path.abspath(base_dir)
    
    # 扫描 usr/ + bsp/ + Core/Src (HAL回调)
    dirs_to_scan = [
        os.path.join(base_dir, 'usr'),
        os.path.join(base_dir, 'bsp'),
        os.path.join(base_dir, 'Core', 'Src'),
        os.path.join(base_dir, 'Core', 'Inc'),
    ]
    
    changed = 0
    for scan_dir in dirs_to_scan:
        if not os.path.isdir(scan_dir):
            continue
        for root, dirs, files in os.walk(scan_dir):
            for fname in files:
                if fname.endswith(('.c', '.h')):
                    fpath = os.path.join(root, fname)
                    if rename_in_file(fpath, RENAMES):
                        print(f"  ✅ {os.path.relpath(fpath, base_dir)}")
                        changed += 1
    
    print(f"\n共修改 {changed} 个文件")

if __name__ == "__main__":
    main()
