#include <intrins.h>
#include "ps2.h"
#include "delay.h"

void psx_init(void) {
	PS2_ATT = 1;
	PS2_CMD = 1;
	PS2_CLK = 1;
	//PS2_DAT = 0;
	//PS2_ACK = 1;
	return;
}

unsigned char psx_transfer(unsigned char dat) {
	
	unsigned char rd_data ,wt_data, i;
	wt_data = dat;
	rd_data = 0;
	for(i = 0;i < 8;i++){
		PS2_CMD = (bit) (wt_data & (0x01 << i));
		PS2_CLK = 1;
		PS2_CLK = 0;
		delay(10);
		PS2_CLK = 1;
		if(PS2_DAT) {
			rd_data |= 0x01<<i;
		}
		delay(10);
	}
	return rd_data;
}
	

void psx_write_read(unsigned char *get_buf) {
	PS2_ATT = 0;
	
	get_buf[0] = psx_transfer(START_CMD);
	get_buf[1] = psx_transfer(ASK_DAT_CMD);
	get_buf[2] = psx_transfer(get_buf[0]);
	get_buf[3] = psx_transfer(get_buf[0]);
	get_buf[4] = psx_transfer(get_buf[0]);
	get_buf[5] = psx_transfer(get_buf[0]);
	get_buf[6] = psx_transfer(get_buf[0]);
	get_buf[7] = psx_transfer(get_buf[0]);
	get_buf[8] = psx_transfer(get_buf[0]);
	
	PS2_ATT = 1;
	return;
}
