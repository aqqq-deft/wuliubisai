#include "includes.h"

static uint16_t s_currentPulse = BUCKET_TRANSPORT_PULSE_US;
static uint16_t s_targetPulse = BUCKET_TRANSPORT_PULSE_US;
static BucketState_t s_state = BUCKET_STATE_TRANSPORT_READY;
static BucketState_t s_targetState = BUCKET_STATE_TRANSPORT_READY;
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
    s_targetPulse = BUCKET_TRANSPORT_PULSE_US;
    (void)movingState;
    (void)readyState;
    s_targetState = BUCKET_STATE_STOPPED;
    s_state = BUCKET_STATE_STOPPED;
    return;
#else
    s_targetPulse = Bucket_ClampPulse(pulse);
#endif
    s_targetState = readyState;
    s_state = (s_currentPulse == s_targetPulse) ? readyState : movingState;
}

void Bucket_Init(void)
{
    s_currentPulse = Bucket_ClampPulse(BUCKET_TRANSPORT_PULSE_US);
    s_targetPulse = s_currentPulse;
    s_state = BUCKET_STATE_TRANSPORT_READY;
    s_targetState = BUCKET_STATE_TRANSPORT_READY;
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
        case BUCKET_COMMAND_LOAD_START:
            Bucket_SetTarget(BUCKET_DIG_PULSE_US, BUCKET_STATE_MOVING_TO_DIG, BUCKET_STATE_DIG_READY);
            break;
        case BUCKET_COMMAND_LOAD_STOP:
        case BUCKET_COMMAND_UNLOAD_STOP:
            Bucket_SetTarget(BUCKET_TRANSPORT_PULSE_US, BUCKET_STATE_MOVING_TRANSPORT, BUCKET_STATE_TRANSPORT_READY);
            break;
        case BUCKET_COMMAND_UNLOAD_START:
            Bucket_SetTarget(BUCKET_DUMP_PULSE_US, BUCKET_STATE_MOVING_TO_DUMP, BUCKET_STATE_DUMP_READY);
            break;
        case BUCKET_COMMAND_RESET:
            s_emergencyLatched = 0U;
            Bucket_SetTarget(BUCKET_TRANSPORT_PULSE_US, BUCKET_STATE_MOVING_TRANSPORT, BUCKET_STATE_TRANSPORT_READY);
#if BUCKET_ENABLE_MOTION == 0U
            s_targetState = BUCKET_STATE_TRANSPORT_READY;
            s_state = BUCKET_STATE_TRANSPORT_READY;
#endif
            break;
        case BUCKET_COMMAND_EMERGENCY_STOP:
            Bucket_EmergencyStop();
            break;
        default:
            break;
    }
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
