#include "uart.h"
#include "stc15.h"
#include "global.h"
#include "timer.h"
#include <string.h>

#define FOSC 22118400L          //系统频率
#define BAUD 115200L            //串口波特率

#define S2RI  0x01    // 宏定义 串口2寄存器
#define S2TI  0x02   // 宏定义 串口2寄存器

#define ISPPROGRAM() ((void(code*)())0xF000)()

u8 busy2 = 0;
u32 uart_timeout = 0;

u32 get_uart_timeout(void) {
	return uart_timeout;
}

void uart1_init(void) {
	SCON |= 0x50;       //串口1方式1,接收充许    
	T2L = (65536 - (FOSC/4/BAUD));
	T2H = (65536 - (FOSC/4/BAUD))>>8;
	AUXR |= 0x15;       //串口1使用独立波特率发生器，独立波特率发生器1T 
	PCON = 0;//0x7F;    //
	EA = 1;   
	ES = 1;             //	
}

void uart2_init(void) {
	S2CON = 0x50;         //
	T2L = (65536 - (FOSC/4/BAUD));    //
	T2H = (65536 - (FOSC/4/BAUD))>>8; //
	IE2 = 0x01;
	EA = 1; 
	P_SW2 |= 0x01;	//TX2 4.7 RX2 4.6	
}

void uart1_open(void) {
	ES = 1;
}

void uart1_close(void) {
	ES = 0;
}

void uart2_open(void) {
	//ES2 = 1;
	IE2 = 0x01; 
}

void uart2_close(void) {
	//ES2 = 0;
	IE2 = 0x00; 
}


/*----------------------------

----------------------------*/
void uart1_send_byte(u8 dat) {
    SBUF = dat;   
    while(TI == 0);   
    TI = 0; 
}

void uart2_send_byte(u8 dat) {
    S2BUF = dat;   
	while(!(S2CON & S2TI));
	S2CON &= ~S2TI; 
}

/*----------------------------

----------------------------*/
void uart1_send_str(char *s) {
    while (*s) {                  	//
        uart1_send_byte(*s++);         //
    }
}

void uart2_send_str(char *s) {
    while (*s) {                  		//
        uart2_send_byte(*s++);         //
    }
}

void zx_uart_send_str(char *s) {
	uart1_get_ok = 1;
    while (*s) {                  		//
        uart2_send_byte(*s++);         //
    }
	uart1_get_ok = 0;
}


/*----------------------------

数据格式:

命令		$xxx!
单个舵机	#0P1000T1000!
多个舵机	{#0P1000T1000!#1P1000T1000!}
存储命令	<#0P1000T1000!#1P1000T1000!>

-----------------------------*/
void Uart() interrupt 4 using 1 {
	static u16 buf_index = 0;
	static u8 sbuf_bak;
	
    if (RI) {
		if(uart1_get_ok)return;
		sbuf_bak = SBUF;
		RI = 0;                 //清除RI位
		if(sbuf_bak == '<') {
			uart1_mode = 4;
			buf_index = 0;
			uart_timeout = get_systick_ms();
		} else if(uart1_mode == 0) {
			if(sbuf_bak == '$') {
				uart1_mode = 1;
			} else if(sbuf_bak == '#') {
				uart1_mode = 2;
			} else if(sbuf_bak == '{') {
				uart1_mode = 3;
			} else if(sbuf_bak == '<') {
				uart1_mode = 4;
			} 
			buf_index = 0;
		}
		
		uart_receive_buf[buf_index++] = sbuf_bak;
		
		if(uart1_mode == 4) {
			
			if(sbuf_bak == '>') {
				//uart1_close();
				uart_receive_buf[buf_index] = '\0';
				uart1_get_ok = 1;
				buf_index = 0;
			}
		} else if((uart1_mode == 1) && (sbuf_bak == '!')){
			uart_receive_buf[buf_index] = '\0';
			uart1_get_ok = 1;
			buf_index = 0;
		} else if((uart1_mode == 2) && (sbuf_bak == '!')){
			uart_receive_buf[buf_index] = '\0';
			uart1_get_ok = 1;
			buf_index = 0;
		} else if((uart1_mode == 3) && (sbuf_bak == '}')){
			uart_receive_buf[buf_index] = '\0';
			uart1_get_ok = 1;
			buf_index = 0;
		}   

		if(buf_index >= UART_BUF_SIZE)buf_index = 0;
    }
	
//    if (TI) {
//        TI = 0;                 //清除TI位
//    }
}

void UART2_Int(void) interrupt  8 using 1 // 串口2中断服务程序
{
	static u16 buf_index = 0;
	static u8 sbuf_bak;
	if(S2CON&S2RI)		  		// 判断是不是接收数据引起的中断
	{   
		sbuf_bak = S2BUF;
		S2CON &= ~S2RI;
		
		if(uart1_get_ok)return;
		
		
		if(sbuf_bak == '<') {
			uart1_mode = 4;
			buf_index = 0;
			uart_timeout = get_systick_ms();
		} else if(uart1_mode == 0) {
			if(sbuf_bak == '$') {
				uart1_mode = 1;
			} else if(sbuf_bak == '#') {
				uart1_mode = 2;
			} else if(sbuf_bak == '{') {
				uart1_mode = 3;
			} else if(sbuf_bak == '<') {
				uart1_mode = 4;
			} 
			buf_index = 0;
		}
		
		uart_receive_buf[buf_index++] = sbuf_bak;
		
		if(uart1_mode == 4) {
			if(sbuf_bak == '>') {
				//uart1_close();
				uart_receive_buf[buf_index] = '\0';
				uart1_get_ok = 1;
				buf_index = 0;
			}
		} else if((uart1_mode == 1) && (sbuf_bak == '!')){
			uart_receive_buf[buf_index] = '\0';
			uart1_get_ok = 1;
			buf_index = 0;
		} else if((uart1_mode == 2) && (sbuf_bak == '!')){
			uart_receive_buf[buf_index] = '\0';
			uart1_get_ok = 1;
			buf_index = 0;
		} else if((uart1_mode == 3) && (sbuf_bak == '}')){
			uart_receive_buf[buf_index] = '\0';
			uart1_get_ok = 1;
			buf_index = 0;
		}   

		if(buf_index >= UART_BUF_SIZE)buf_index = 0;
		//uart2_send_byte(sbuf_bak);
	}
	
//	if (S2CON&S2TI)// 接收到发送命令
//	{
//		busy2 = 0;
//	}
}


