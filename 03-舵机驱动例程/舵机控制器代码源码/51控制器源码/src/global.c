#include "global.h"

duoji_doing_t duoji_doing[DJ_NUM];
u8 uart_receive_buf[UART_BUF_SIZE+4], uart1_get_ok, uart1_mode, uart_receive_buf_index;
u8 cmd_return[100];
//u32 uart_get_timeout;




void global_init(void) {

	u8 i;
	for(i=0;i<DJ_NUM;i++) {
		duoji_doing[i].aim = duoji_doing[i].cur = 1500;
		duoji_doing[i].inc = 0;		
	}
	
	uart1_get_ok = 0;
	uart1_mode = 0;
	uart_receive_buf_index = 0;
	//uart_get_timeout = 0;

}

u16 str_contain_str(u8 *str, u8 *str2) {
	u8 *str_temp, *str_temp2;
	str_temp = str;
	str_temp2 = str2;
	while(*str_temp) {
		if(*str_temp == *str_temp2) {
			while(*str_temp2) {
				if(*str_temp++ != *str_temp2++) {
					str_temp = str_temp - (str_temp2-str2) + 1;
					str_temp2 = str2;
					break;
				}	
			}
			if(!*str_temp2) {
				return (str_temp-str);
			}
			
		} else {
			str_temp++;
		}
	}
	return 0;
}