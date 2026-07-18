#include "stc15.h"
#include "spi.h"
#define FOSC MAIN_Fosc

/************对SPCTL寄存器的设置位宏定义*************/
#define SSIG    0x80  //SS引脚忽略
#define SPEN    0x40  //SPI使能位
#define DORD    0x20  //SPI数据发送LSB最先发送
#define MSTR    0x10  //主从模式选择
#define CPOL    0x08  //SPICLK空闲时为高电平
#define CPHA    0x04  //数据在SPICLK的前时钟沿驱动,并在后时钟沿采样
#define SP_CLK0 0x00  //SPI时钟频率为CPU_CLK/4
#define SP_CLK1 0x01  //SPI时钟频率为CPU_CLK/16
#define SP_CLK2 0x02  //SPI时钟频率为CPU_CLK/64
#define SP_CLK3 0x03  //SPI时钟频率为CPU_CLK/128
/************对SPSTAT寄存器的设置位宏定义************/
#define SPIF    0x80  //传输完成标志
#define WCOL    0x40  //SPI写冲突标志		


/****************************************
*函数名称:Spi0MasterInit
*输    入:无
*输    出:无
*功    能:SPI0初始化为主机模式
******************************************/
void SpiMasterInit(void)
{
#if 0
		 /* SPI MISO P1.4输入*/
		P1M1|=  1<<4 ;
		P1M0&=~(1<<4);
		 
		 /* SPI MOSI,SCK,CS P1.2,P1.3,P1.5推挽输出*/
		P1M1&=~((1<<2)|(1<<3)|(1<<5));
		P1M0|=  (1<<2)|(1<<3)|(1<<5);
		 
    SPI_CS(1);
		SPI_MOSI(1);
		SPI_SCK(1);
#else	
		P1M1&=~(1<<2);
		P1M0|= (1<<2);
    SPDAT  = 0;         //清空数据寄存器
    SPSTAT = SPIF|WCOL; //清空SPI状态寄存器
		
#if FOSC==33000000UL		
	  SPCTL  = SPEN|MSTR|SP_CLK1|SSIG; //SPI设置为主机模式
#else
	  SPCTL  = SPEN|MSTR|SP_CLK0|SSIG; //SPI设置为主机模式
#endif		
    
#endif		 
}

/****************************************
*函数名称:SpiWriteRead
*输    入:ucData 要发送的数据
*输    出:返回字节数据
*功    能:SPI读写数据
******************************************/
#if 0
UINT8 SpiWriteRead(UINT8 d)
{
    UINT8 i;
	
	  SPI_SCK(0);
	
    for(i=0; i<8; i++)
    {
			  /*   发送字节高位 */
		    if(d & 0x80)
				{
					 SPI_MOSI(1);
				}
				else
				{
					 SPI_MOSI(0);
				}
				
				/*   接收字节高位 */
				d<<=1;
				
				SPI_SCK(1);
			
				if(SPI_MISO())
				{
					 d|=0x01;
				}
					
				SPI_SCK(0);

		
		}	
		
		return d;
}
#else




UINT8 SpiWriteRead(UINT8 d)
{
    SPDAT = d;                  //触发SPI发送数据
    while (!(SPSTAT & SPIF));   //等待发送完成
    SPSTAT = SPIF | WCOL;       //清除SPI状态位
    return SPDAT;               //返回SPI数据
}
#endif

/*
void SpiSetSpeedLow(void)
{
    SPCTL  &=~SP_CLK3; 
	  SPCTL |=SP_CLK3;
}

void SpiSetSpeedHigh(void)
{
    SPCTL  &=~SP_CLK3; 
#if FOSC==33000000UL		
	  SPCTL  |=SP_CLK1;
#else
	  SPCTL  |=SP_CLK0;
#endif
}
*/



