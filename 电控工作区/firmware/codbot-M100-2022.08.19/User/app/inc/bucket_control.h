#ifndef __BUCKET_CONTROL_H__
#define __BUCKET_CONTROL_H__

#include <stdint.h>

#define BUCKET_UART_COMMAND_CODE 0x07U
#define BUCKET_UART_FEEDBACK_CODE 0x08U

typedef enum {
    BUCKET_COMMAND_STOP = 0U,
    BUCKET_COMMAND_LOAD_START = 1U,
    BUCKET_COMMAND_LOAD_STOP = 2U,
    BUCKET_COMMAND_UNLOAD_START = 3U,
    BUCKET_COMMAND_UNLOAD_STOP = 4U,
    BUCKET_COMMAND_RESET = 5U,
    BUCKET_COMMAND_EMERGENCY_STOP = 255U
} BucketCommand_t;

typedef enum {
    BUCKET_STATE_STOPPED = 0U,
    BUCKET_STATE_MOVING_TO_DIG = 1U,
    BUCKET_STATE_DIG_READY = 2U,
    BUCKET_STATE_MOVING_TRANSPORT = 3U,
    BUCKET_STATE_TRANSPORT_READY = 4U,
    BUCKET_STATE_MOVING_TO_DUMP = 5U,
    BUCKET_STATE_DUMP_READY = 6U,
    BUCKET_STATE_EMERGENCY_STOP = 255U
} BucketState_t;

void Bucket_Init(void);
void Bucket_Update(void);
void Bucket_HandleCommand(uint8_t command);
void Bucket_EmergencyStop(void);
void Bucket_SendFeedback(void);
BucketState_t Bucket_GetState(void);
uint16_t Bucket_GetCurrentPulse(void);

#endif
