#include "includes.h"

static uint16_t s_currentPulse = BUCKET_CALIBRATION_CENTER_PULSE_US;
static uint16_t s_targetPulse = BUCKET_CALIBRATION_CENTER_PULSE_US;
static BucketState_t s_state = BUCKET_STATE_STOPPED;
static BucketState_t s_targetState = BUCKET_STATE_STOPPED;
static uint8_t s_emergencyLatched = 0U;
static uint8_t s_updateDivider = 0U;

static uint16_t Bucket_ClampPulse(uint16_t pulse)
{
    if(pulse < BUCKET_MIN_PULSE_US) return BUCKET_MIN_PULSE_US;
    if(pulse > BUCKET_MAX_PULSE_US) return BUCKET_MAX_PULSE_US;
    return pulse;
}

static void Bucket_SetTarget(uint16_t pulse, BucketState_t movingState, BucketState_t readyState)
{
    if(s_emergencyLatched != 0U) return;
#if BUCKET_ENABLE_MOTION == 0U
    (void)pulse;
    s_targetPulse = s_currentPulse;
    (void)movingState;
    (void)readyState;
    s_targetState = BUCKET_STATE_STOPPED;
    s_state = BUCKET_STATE_STOPPED;
    return;
#else
    s_targetPulse = Bucket_ClampPulse(pulse);
    s_targetState = readyState;
    s_state = (s_currentPulse == s_targetPulse) ? readyState : movingState;
#endif
}

void Bucket_Init(void)
{
#if BUCKET_ENABLE_CALIBRATION != 0U
    s_currentPulse = Bucket_ClampPulse(BUCKET_CALIBRATION_CENTER_PULSE_US);
    s_state = BUCKET_STATE_STOPPED;
    s_targetState = BUCKET_STATE_STOPPED;
#else
    s_currentPulse = Bucket_ClampPulse(BUCKET_CLOSED_PULSE_US);
    s_state = BUCKET_STATE_CLOSED;
    s_targetState = BUCKET_STATE_CLOSED;
#endif
    s_targetPulse = s_currentPulse;
    s_emergencyLatched = 0U;
    s_updateDivider = 0U;
    servoSetPluse(BUCKET_SERVO_NUMBER, s_currentPulse);
}

void Bucket_Update(void)
{
    s_updateDivider++;
    if(s_updateDivider < 2U) return;
    s_updateDivider = 0U;
    if((s_emergencyLatched != 0U) || (s_currentPulse == s_targetPulse)) {
        if(s_emergencyLatched == 0U) s_state = s_targetState;
        return;
    }
    if(s_currentPulse < s_targetPulse) {
        uint16_t remaining = s_targetPulse - s_currentPulse;
        s_currentPulse += (remaining > BUCKET_STEP_US) ? BUCKET_STEP_US : remaining;
    } else {
        uint16_t remaining = s_currentPulse - s_targetPulse;
        s_currentPulse -= (remaining > BUCKET_STEP_US) ? BUCKET_STEP_US : remaining;
    }
    servoSetPluse(BUCKET_SERVO_NUMBER, s_currentPulse);
    if(s_currentPulse == s_targetPulse) s_state = s_targetState;
}

void Bucket_HandleCommand(uint8_t command)
{
    switch(command) {
        case BUCKET_COMMAND_STOP:
            if(s_emergencyLatched == 0U) {
                s_targetPulse = s_currentPulse;
                s_targetState = BUCKET_STATE_STOPPED;
                s_state = BUCKET_STATE_STOPPED;
            }
            break;
        case BUCKET_COMMAND_OPEN:
            Bucket_SetTarget(BUCKET_OPEN_PULSE_US, BUCKET_STATE_MOVING_TO_OPEN, BUCKET_STATE_OPEN);
            break;
        case BUCKET_COMMAND_CLOSE:
            Bucket_SetTarget(BUCKET_CLOSED_PULSE_US, BUCKET_STATE_MOVING_TO_CLOSED, BUCKET_STATE_CLOSED);
            break;
        case BUCKET_COMMAND_RESERVED_3:
        case BUCKET_COMMAND_RESERVED_4:
            break;
        case BUCKET_COMMAND_RESET:
            s_emergencyLatched = 0U;
#if BUCKET_ENABLE_CALIBRATION != 0U
            s_targetPulse = Bucket_ClampPulse(BUCKET_CALIBRATION_CENTER_PULSE_US);
            s_targetState = BUCKET_STATE_STOPPED;
            s_state = (s_currentPulse == s_targetPulse) ? BUCKET_STATE_STOPPED : BUCKET_STATE_MOVING_TO_CLOSED;
#else
            Bucket_SetTarget(BUCKET_CLOSED_PULSE_US, BUCKET_STATE_MOVING_TO_CLOSED, BUCKET_STATE_CLOSED);
#endif
            break;
        case BUCKET_COMMAND_EMERGENCY_STOP:
            Bucket_EmergencyStop();
            break;
        default:
            break;
    }
}

void Bucket_HandleCalibrationCommand(uint8_t command)
{
#if BUCKET_ENABLE_CALIBRATION != 0U
    uint16_t nextPulse;

    if(s_emergencyLatched != 0U) return;

    nextPulse = s_targetPulse;
    switch(command) {
        case BUCKET_CALIBRATION_HOLD:
            s_targetPulse = s_currentPulse;
            s_targetState = BUCKET_STATE_STOPPED;
            s_state = BUCKET_STATE_STOPPED;
            break;
        case BUCKET_CALIBRATION_DECREASE:
            if(nextPulse > (BUCKET_MIN_PULSE_US + BUCKET_STEP_US)) nextPulse -= BUCKET_STEP_US;
            else nextPulse = BUCKET_MIN_PULSE_US;
            s_targetPulse = Bucket_ClampPulse(nextPulse);
            s_targetState = BUCKET_STATE_STOPPED;
            break;
        case BUCKET_CALIBRATION_INCREASE:
            if(nextPulse < (BUCKET_MAX_PULSE_US - BUCKET_STEP_US)) nextPulse += BUCKET_STEP_US;
            else nextPulse = BUCKET_MAX_PULSE_US;
            s_targetPulse = Bucket_ClampPulse(nextPulse);
            s_targetState = BUCKET_STATE_STOPPED;
            break;
        default:
            break;
    }
#else
    (void)command;
#endif
}

void Bucket_EmergencyStop(void)
{
    s_targetPulse = s_currentPulse;
    s_emergencyLatched = 1U;
    s_state = BUCKET_STATE_EMERGENCY_STOP;
    s_targetState = BUCKET_STATE_EMERGENCY_STOP;
}

void Bucket_SendFeedback(void)
{
    uint8_t frame[7];
    frame[0] = 0xFDU;
    frame[1] = 0x04U;
    frame[2] = BUCKET_UART_FEEDBACK_CODE;
    frame[3] = (uint8_t)s_state;
    frame[4] = (uint8_t)(s_currentPulse >> 8);
    frame[5] = (uint8_t)(s_currentPulse & 0xFFU);
    frame[6] = frame[2] ^ frame[3] ^ frame[4] ^ frame[5];
    comSendBuf(COM1, frame, sizeof(frame));
}

BucketState_t Bucket_GetState(void) { return s_state; }
uint16_t Bucket_GetCurrentPulse(void) { return s_currentPulse; }
