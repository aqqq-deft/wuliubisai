#ifndef __IO_H__
#define __IO_H__

#include "stc15.h"

sbit nled = P2^0;
sbit beep = P5^1;

sbit dj0 = P5^0;
sbit dj1 = P3^4;
sbit dj2 = P0^4;
sbit dj3 = P5^3;
sbit dj4 = P0^5;
sbit dj5 = P5^2;

sbit cg0 = P0^2;
sbit cg1 = P0^3;
sbit cg2 = P1^6;
sbit cg3 = P1^2;
sbit cg4 = P1^1;
sbit cg5 = P1^0;

sbit pwm2_io = P3^7;
sbit pwm3_io = P2^1;
sbit pwm4_io = P2^2;
sbit pwm5_io = P2^3;



#define nled_on() {nled = 0;}
#define nled_off() {nled = 1;}
#define nled_switch() {nled = ~nled;}

#define beep_on() {beep = 0;}
#define beep_off() {beep = 1;}
#define beep_switch() {beep = ~beep;}

void io_init(void);
void dj_io_init(void);
void dj_io_set(u8 index, u8 level);


#endif