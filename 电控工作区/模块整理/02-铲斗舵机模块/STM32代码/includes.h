#ifndef __INCLUDE_H__
#define __INCLUDE_H__


#include "bsp.h"

#include "cmsis_os2.h" 
#include "motor.h"
#include "steercontrol.h"
#include "pid.h"
#include "autocontrol.h"
#include "feedback.h"
#include "SEGGER_RTT.h"
#include "tool.h"
typedef struct{

#include "competition_config.h"
#include "bucket_control.h"
	uint8_t autoStatus;
	uint8_t remoteStatus;
	uint8_t disLinkStatus;
}LinkStatus_t;
#endif /*__INCLUDE_H__*/

