#ifndef __COMPETITION_CONFIG_H__
#define __COMPETITION_CONFIG_H__

/* First-power-on safety defaults. Change only after the matching test passes. */
#define COMPETITION_ENABLE_BLE_CONTROL       0U
#define COMPETITION_CONTROL_PERIOD_MS       10U
#define COMPETITION_COMMAND_TIMEOUT_MS      500U
#define COMPETITION_PWM_LIMIT               300U
#define COMPETITION_ENABLE_IMU               0U
#define COMPETITION_LED_DIAGNOSTIC            0U

/* Keep disabled until all positions are calibrated with the linkage detached. */
#define BUCKET_ENABLE_MOTION                 0U
#define BUCKET_SERVO_NUMBER                  1U
#define BUCKET_MIN_PULSE_US                  1000U
#define BUCKET_MAX_PULSE_US                  2000U
#define BUCKET_STEP_US                       10U
#define BUCKET_DIG_PULSE_US                  1500U
#define BUCKET_TRANSPORT_PULSE_US            1500U
#define BUCKET_DUMP_PULSE_US                 1500U

#if (BUCKET_MIN_PULSE_US < 1000U) || (BUCKET_MAX_PULSE_US > 2000U)
#error "Bucket servo limits must stay within the initial 1000..2000 us safety range"
#endif
#if (BUCKET_MIN_PULSE_US >= BUCKET_MAX_PULSE_US)
#error "Bucket servo minimum pulse must be less than maximum pulse"
#endif
#if (BUCKET_DIG_PULSE_US < BUCKET_MIN_PULSE_US) || (BUCKET_DIG_PULSE_US > BUCKET_MAX_PULSE_US) || \
    (BUCKET_TRANSPORT_PULSE_US < BUCKET_MIN_PULSE_US) || (BUCKET_TRANSPORT_PULSE_US > BUCKET_MAX_PULSE_US) || \
    (BUCKET_DUMP_PULSE_US < BUCKET_MIN_PULSE_US) || (BUCKET_DUMP_PULSE_US > BUCKET_MAX_PULSE_US)
#error "Every calibrated bucket position must stay inside the configured limits"
#endif


#define COMPETITION_COMMAND_TIMEOUT_CYCLES  \
    (COMPETITION_COMMAND_TIMEOUT_MS / COMPETITION_CONTROL_PERIOD_MS)

#endif
