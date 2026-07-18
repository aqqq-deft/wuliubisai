#ifndef __C7_SERVO_CONTROL_H__
#define __C7_SERVO_CONTROL_H__

#include <stdint.h>

#define C7_SERVO_UART_COMMAND_CODE  0x0AU
#define C7_SERVO_UART_FEEDBACK_CODE 0x0BU

typedef enum {
    C7_SERVO_ACTION_HOLD = 0U,
    C7_SERVO_ACTION_PRESET_ENABLE = 1U,
    C7_SERVO_ACTION_MOVE_TARGET = 2U,
    C7_SERVO_ACTION_POSITION_0 = 3U,
    C7_SERVO_ACTION_POSITION_1 = 4U,
    C7_SERVO_ACTION_POSITION_2 = 5U,
    C7_SERVO_ACTION_POSITION_3 = 6U,
    C7_SERVO_ACTION_QUERY_STATUS = 7U,
    C7_SERVO_ACTION_EMERGENCY_STOP = 255U
} C7ServoAction_t;

typedef enum {
    C7_SERVO_STATE_DISABLED = 0U,
    C7_SERVO_STATE_HOLDING = 1U,
    C7_SERVO_STATE_MOVING = 2U,
    C7_SERVO_STATE_EMERGENCY_STOP = 255U
} C7ServoState_t;

void C7Servo_Init(void);
void C7Servo_Update(void);
void C7Servo_HandleCommand(uint8_t action, uint16_t pulse);
void C7Servo_EmergencyStop(void);
void C7Servo_SendFeedback(void);
C7ServoState_t C7Servo_GetState(void);
uint16_t C7Servo_GetCurrentCommandPulse(void);
uint16_t C7Servo_GetTargetCommandPulse(void);

#endif
