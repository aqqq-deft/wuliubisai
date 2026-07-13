#ifndef __AUTOCONTROL_H__
#define __AUTOCONTROL_H__

#include "bsp.h"

void auto_ControlCarCmdHandle();
void auto_ControlCarStop();

void MotorControl(uint8_t _ucAngularDirection,uint8_t _ucAngularSpeed,uint8_t _ucMotionDirection,uint16_t _usMotionSpeed,uint8_t _ucMotionDirection_Y,uint16_t _usMotionSpeed_Y);
#endif /*__AUTOCONTROL_H__*/