#include "delay.h"


#define FOSC 22118400L          //ÏµÍ³ÆµÂÊ


void delay(unsigned int t) {
	
	while(t--);
	
}

void delay_us(unsigned int t) {
	unsigned char i;
	while(t--) {
		i = 3;
		while(i--) delay(1);
	}
}

void delay_ms(unsigned int t) {
	while(t--) {
	    delay_us(t);
	}
}