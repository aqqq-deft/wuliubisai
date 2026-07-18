#ifndef __COMPETITION_CONFIG_H__
#define __COMPETITION_CONFIG_H__

/* First-power-on safety defaults. Change only after the matching test passes. */
#define COMPETITION_ENABLE_BLE_CONTROL       0U
#define COMPETITION_CONTROL_PERIOD_MS       10U
#define COMPETITION_COMMAND_TIMEOUT_MS      500U
#define COMPETITION_PWM_LIMIT               300U
#define COMPETITION_ENABLE_IMU               0U
#define COMPETITION_LED_DIAGNOSTIC            0U

/*
 * Servo calibration safety defaults.
 *
 * Native open/close motion stays disabled; the PC tool walks the servo safely.
 * Calibration mode only accepts one 10 us step per command and is hard-limited
 * to the current staged test range. Keep calibration enabled until the real
 * claw linkage is installed and its safe OPEN/CLOSED positions are verified.
 */
#define BUCKET_ENABLE_MOTION                 0U
#define BUCKET_ENABLE_CALIBRATION            1U
#define BUCKET_SERVO_NUMBER                  1U
#define BUCKET_MIN_PULSE_US                  550U
#define BUCKET_MAX_PULSE_US                  2450U
#define BUCKET_STEP_US                       10U
#define BUCKET_CALIBRATION_CENTER_PULSE_US   1500U
#define BUCKET_OPEN_PULSE_US                 1500U
#define BUCKET_CLOSED_PULSE_US               1500U

/*
 * C7 / PD-20S safety configuration.
 *
 * C7 starts disabled and does not receive a valid PWM pulse at boot.  The PC
 * tool must first send the PRESET_ENABLE command while the external servo
 * supply is still off.  Fixed positions remain disabled until the assembled
 * mechanism has been explored and the verified pulses are copied here.
 */
#define C7_SERVO_NUMBER                      2U
#define C7_SERVO_MIN_PULSE_US                550U
#define C7_SERVO_MAX_PULSE_US                2450U
#define C7_SERVO_STEP_US                     5U
#define C7_SERVO_UPDATE_PERIOD_MS            20U
#define C7_SERVO_PRESET_PULSE_US             1500U
#define C7_SERVO_FIXED_POSITIONS_READY       0U
#define C7_SERVO_POSITION_0_PULSE_US         1500U
#define C7_SERVO_POSITION_1_PULSE_US         1500U
#define C7_SERVO_POSITION_2_PULSE_US         1500U
#define C7_SERVO_POSITION_3_PULSE_US         1500U

#if (BUCKET_MIN_PULSE_US < 550U) || (BUCKET_MAX_PULSE_US > 2450U)
#error "Bucket servo limits must stay within the staged 550..2450 us test range"
#endif
#if (BUCKET_MIN_PULSE_US >= BUCKET_MAX_PULSE_US)
#error "Bucket servo minimum pulse must be less than maximum pulse"
#endif
#if (BUCKET_CALIBRATION_CENTER_PULSE_US < BUCKET_MIN_PULSE_US) || \
    (BUCKET_CALIBRATION_CENTER_PULSE_US > BUCKET_MAX_PULSE_US) || \
    (BUCKET_OPEN_PULSE_US < BUCKET_MIN_PULSE_US) || (BUCKET_OPEN_PULSE_US > BUCKET_MAX_PULSE_US) || \
    (BUCKET_CLOSED_PULSE_US < BUCKET_MIN_PULSE_US) || (BUCKET_CLOSED_PULSE_US > BUCKET_MAX_PULSE_US)
#error "Every configured servo pulse must stay inside the configured limits"
#endif

#if (C7_SERVO_MIN_PULSE_US < 550U) || (C7_SERVO_MAX_PULSE_US > 2450U)
#error "C7 servo limits must stay within the staged 550..2450 us test range"
#endif
#if (C7_SERVO_MIN_PULSE_US >= C7_SERVO_MAX_PULSE_US)
#error "C7 servo minimum pulse must be less than maximum pulse"
#endif
#if (C7_SERVO_STEP_US == 0U)
#error "C7 servo step must be greater than zero"
#endif
#if (C7_SERVO_UPDATE_PERIOD_MS < COMPETITION_CONTROL_PERIOD_MS) || \
    ((C7_SERVO_UPDATE_PERIOD_MS % COMPETITION_CONTROL_PERIOD_MS) != 0U)
#error "C7 servo update period must be a multiple of the control period"
#endif
#if (C7_SERVO_PRESET_PULSE_US < C7_SERVO_MIN_PULSE_US) || \
    (C7_SERVO_PRESET_PULSE_US > C7_SERVO_MAX_PULSE_US) || \
    (C7_SERVO_POSITION_0_PULSE_US < C7_SERVO_MIN_PULSE_US) || \
    (C7_SERVO_POSITION_0_PULSE_US > C7_SERVO_MAX_PULSE_US) || \
    (C7_SERVO_POSITION_1_PULSE_US < C7_SERVO_MIN_PULSE_US) || \
    (C7_SERVO_POSITION_1_PULSE_US > C7_SERVO_MAX_PULSE_US) || \
    (C7_SERVO_POSITION_2_PULSE_US < C7_SERVO_MIN_PULSE_US) || \
    (C7_SERVO_POSITION_2_PULSE_US > C7_SERVO_MAX_PULSE_US) || \
    (C7_SERVO_POSITION_3_PULSE_US < C7_SERVO_MIN_PULSE_US) || \
    (C7_SERVO_POSITION_3_PULSE_US > C7_SERVO_MAX_PULSE_US)
#error "Every configured C7 servo pulse must stay inside the configured limits"
#endif

#define C7_SERVO_UPDATE_CYCLES \
    (C7_SERVO_UPDATE_PERIOD_MS / COMPETITION_CONTROL_PERIOD_MS)

#define COMPETITION_COMMAND_TIMEOUT_CYCLES  \
    (COMPETITION_COMMAND_TIMEOUT_MS / COMPETITION_CONTROL_PERIOD_MS)

#endif
