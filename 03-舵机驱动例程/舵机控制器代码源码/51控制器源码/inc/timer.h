#ifndef __TIMER_H__
#define __TIMER_H__

#include "stc15.h"

#define timer1_open() {TR1 = 1;ET1 = 1;}
#define timer1_close() {TR1 = 0;ET1 = 0;}

u32 get_systick_ms(void);

void timer0_init(void);
void timer1_init(void);

//void timer0_reset(int t_us);
void timer1_reset(int t_us);
void duoji_inc_handle(u8 index);

#endif



