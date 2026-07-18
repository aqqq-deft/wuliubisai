#ifndef __PS2_H__
#define __PS2_H__

#include "stc15.h"

#define START_CMD			0x01
#define ASK_DAT_CMD			0x42

//IO口定义
sbit  PS2_DAT=P4^2;	
sbit  PS2_CMD=P4^5;
sbit  PS2_ATT=P2^7;
sbit  PS2_CLK=P2^6;

 /*********************************************************

	函数声明

**********************************************************/	 

void psx_init(void);
void psx_write_read(unsigned char *get_buf);




#endif