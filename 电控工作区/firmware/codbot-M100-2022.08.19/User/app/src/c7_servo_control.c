#include "includes.h"

static uint16_t s_currentCommandPulse = C7_SERVO_PRESET_PULSE_US;
static uint16_t s_targetCommandPulse = C7_SERVO_PRESET_PULSE_US;
static C7ServoState_t s_state = C7_SERVO_STATE_DISABLED;
static uint8_t s_enabled = 0U;
static uint8_t s_emergencyLatched = 0U;
static uint8_t s_updateDivider = 0U;

static uint16_t C7Servo_ClampPulse(uint16_t pulse)
{
    if(pulse < C7_SERVO_MIN_PULSE_US) return C7_SERVO_MIN_PULSE_US;
    if(pulse > C7_SERVO_MAX_PULSE_US) return C7_SERVO_MAX_PULSE_US;
    return pulse;
}

static void C7Servo_SetMoveTarget(uint16_t pulse)
{
    if((s_enabled == 0U) || (s_emergencyLatched != 0U)) return;

    s_targetCommandPulse = C7Servo_ClampPulse(pulse);
    s_state = (s_currentCommandPulse == s_targetCommandPulse) ?
              C7_SERVO_STATE_HOLDING : C7_SERVO_STATE_MOVING;
}

void C7Servo_Init(void)
{
    s_currentCommandPulse = C7Servo_ClampPulse(C7_SERVO_PRESET_PULSE_US);
    s_targetCommandPulse = s_currentCommandPulse;
    s_state = C7_SERVO_STATE_DISABLED;
    s_enabled = 0U;
    s_emergencyLatched = 0U;
    s_updateDivider = 0U;

    /* Intentionally do not call servoSetPluse(): C7 stays at a 0 us compare. */
}

void C7Servo_Update(void)
{
    uint16_t remaining;

    s_updateDivider++;
    if(s_updateDivider < C7_SERVO_UPDATE_CYCLES) return;
    s_updateDivider = 0U;

    if((s_enabled == 0U) || (s_emergencyLatched != 0U) ||
       (s_currentCommandPulse == s_targetCommandPulse)) return;

    if(s_currentCommandPulse < s_targetCommandPulse) {
        remaining = s_targetCommandPulse - s_currentCommandPulse;
        s_currentCommandPulse += (remaining > C7_SERVO_STEP_US) ?
                                 C7_SERVO_STEP_US : remaining;
    } else {
        remaining = s_currentCommandPulse - s_targetCommandPulse;
        s_currentCommandPulse -= (remaining > C7_SERVO_STEP_US) ?
                                 C7_SERVO_STEP_US : remaining;
    }

    servoSetPluse(C7_SERVO_NUMBER, s_currentCommandPulse);
    if(s_currentCommandPulse == s_targetCommandPulse) {
        s_state = C7_SERVO_STATE_HOLDING;
    }
}

void C7Servo_HandleCommand(uint8_t action, uint16_t pulse)
{
    switch(action) {
        case C7_SERVO_ACTION_HOLD:
            if((s_enabled != 0U) && (s_emergencyLatched == 0U)) {
                s_targetCommandPulse = s_currentCommandPulse;
                s_state = C7_SERVO_STATE_HOLDING;
            }
            break;

        case C7_SERVO_ACTION_PRESET_ENABLE:
            s_currentCommandPulse = C7Servo_ClampPulse(pulse);
            s_targetCommandPulse = s_currentCommandPulse;
            s_enabled = 1U;
            s_emergencyLatched = 0U;
            s_state = C7_SERVO_STATE_HOLDING;
            servoSetPluse(C7_SERVO_NUMBER, s_currentCommandPulse);
            break;

        case C7_SERVO_ACTION_MOVE_TARGET:
            C7Servo_SetMoveTarget(pulse);
            break;

        case C7_SERVO_ACTION_POSITION_0:
#if C7_SERVO_FIXED_POSITIONS_READY != 0U
            C7Servo_SetMoveTarget(C7_SERVO_POSITION_0_PULSE_US);
#endif
            break;

        case C7_SERVO_ACTION_POSITION_1:
#if C7_SERVO_FIXED_POSITIONS_READY != 0U
            C7Servo_SetMoveTarget(C7_SERVO_POSITION_1_PULSE_US);
#endif
            break;

        case C7_SERVO_ACTION_POSITION_2:
#if C7_SERVO_FIXED_POSITIONS_READY != 0U
            C7Servo_SetMoveTarget(C7_SERVO_POSITION_2_PULSE_US);
#endif
            break;

        case C7_SERVO_ACTION_POSITION_3:
#if C7_SERVO_FIXED_POSITIONS_READY != 0U
            C7Servo_SetMoveTarget(C7_SERVO_POSITION_3_PULSE_US);
#endif
            break;

        case C7_SERVO_ACTION_EMERGENCY_STOP:
            C7Servo_EmergencyStop();
            break;

        case C7_SERVO_ACTION_QUERY_STATUS:

        default:
            break;
    }

    /* Reply from the control task; no periodic C7 send runs at boot. */
    C7Servo_SendFeedback();
}

void C7Servo_EmergencyStop(void)
{
    s_targetCommandPulse = s_currentCommandPulse;
    if(s_enabled == 0U) {
        s_emergencyLatched = 0U;
        s_state = C7_SERVO_STATE_DISABLED;
        return;
    }
    s_emergencyLatched = 1U;
    s_state = C7_SERVO_STATE_EMERGENCY_STOP;
}

void C7Servo_SendFeedback(void)
{
    uint8_t frame[9];

    frame[0] = 0xFDU;
    frame[1] = 0x06U;
    frame[2] = C7_SERVO_UART_FEEDBACK_CODE;
    frame[3] = (uint8_t)s_state;
    frame[4] = (uint8_t)(s_currentCommandPulse >> 8);
    frame[5] = (uint8_t)(s_currentCommandPulse & 0xFFU);
    frame[6] = (uint8_t)(s_targetCommandPulse >> 8);
    frame[7] = (uint8_t)(s_targetCommandPulse & 0xFFU);
    frame[8] = frame[2] ^ frame[3] ^ frame[4] ^ frame[5] ^
               frame[6] ^ frame[7];
    comSendBuf(COM1, frame, sizeof(frame));
}

C7ServoState_t C7Servo_GetState(void) { return s_state; }
uint16_t C7Servo_GetCurrentCommandPulse(void) { return s_currentCommandPulse; }
uint16_t C7Servo_GetTargetCommandPulse(void) { return s_targetCommandPulse; }
