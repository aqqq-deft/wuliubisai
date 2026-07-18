#ifndef __BUCKET_CONTROL_H__
#define __BUCKET_CONTROL_H__

#include <stdint.h>

#define BUCKET_UART_COMMAND_CODE 0x07U
#define BUCKET_UART_FEEDBACK_CODE 0x08U
#define BUCKET_UART_CALIBRATION_CODE 0x09U

typedef enum {
    BUCKET_COMMAND_STOP = 0U,
    BUCKET_COMMAND_OPEN = 1U,
    BUCKET_COMMAND_CLOSE = 2U,
    BUCKET_COMMAND_RESERVED_3 = 3U,
    BUCKET_COMMAND_RESERVED_4 = 4U,
    BUCKET_COMMAND_RESET = 5U,
    BUCKET_COMMAND_EMERGENCY_STOP = 255U
} BucketCommand_t;

typedef enum {
    BUCKET_STATE_STOPPED = 0U,
    BUCKET_STATE_MOVING_TO_OPEN = 1U,
    BUCKET_STATE_OPEN = 2U,
    BUCKET_STATE_MOVING_TO_CLOSED = 3U,
    BUCKET_STATE_CLOSED = 4U,
    BUCKET_STATE_EMERGENCY_STOP = 255U
} BucketState_t;

typedef enum {
    BUCKET_CALIBRATION_HOLD = 0U,
    BUCKET_CALIBRATION_DECREASE = 1U,
    BUCKET_CALIBRATION_INCREASE = 2U
} BucketCalibrationCommand_t;

void Bucket_Init(void);
void Bucket_Update(void);
void Bucket_HandleCommand(uint8_t command);
void Bucket_HandleCalibrationCommand(uint8_t command);
void Bucket_EmergencyStop(void);
void Bucket_SendFeedback(void);
BucketState_t Bucket_GetState(void);
uint16_t Bucket_GetCurrentPulse(void);

#endif
