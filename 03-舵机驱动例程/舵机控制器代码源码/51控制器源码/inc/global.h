#ifndef __GLOBAL_H__
#define __GLOBAL_H__

#include "stc15.h"

typedef struct {
	float aim;
	float cur;
	float inc;
	int time;
}duoji_doing_t;

#define DJ_NUM 8


extern duoji_doing_t duoji_doing[DJ_NUM];

#define UART_BUF_SIZE 1000

extern u8 uart_receive_buf[UART_BUF_SIZE+4], uart1_get_ok, uart1_mode, uart_receive_buf_index;
extern u8 cmd_return[100];

void global_init(void);
u16 str_contain_str(u8 *str, u8 *str2);



#endif