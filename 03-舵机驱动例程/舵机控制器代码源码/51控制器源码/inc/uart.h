#ifndef __UART_H__
#define __UART_H__

#include "stc15.h"


//typedef unsigned char BYTE;
//typedef unsigned int WORD;


//#define NONE_PARITY     0       //无校验
//#define ODD_PARITY      1       //奇校验
//#define EVEN_PARITY     2       //偶校验
//#define MARK_PARITY     3       //标记校验
//#define SPACE_PARITY    4       //空白校验

//#define PARITYBIT EVEN_PARITY   //定义校验位

void uart1_init(void);
void uart1_send_byte(u8 dat);
void uart1_send_str(char *s);
void uart1_close(void);
void uart1_open(void);

void uart2_init(void);
void uart2_send_byte(u8 dat);
void uart2_send_str(char *s);
void uart2_close(void);
void uart2_open(void);

void zx_uart_send_str(char *s);

u32 get_uart_timeout(void);


#endif