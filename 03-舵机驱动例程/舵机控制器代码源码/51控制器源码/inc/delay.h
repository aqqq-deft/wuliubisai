#ifndef __DELAY_H__
#define __DELAY_H__


#define mdelay(x) delay_ms(x)
#define udelay(x) delay_us(x)

void delay(unsigned int t);
void delay_us(unsigned int t);
void delay_ms(unsigned int t);



#endif