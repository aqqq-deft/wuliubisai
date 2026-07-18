#ifndef __SPI_H__
#define __SPI_H__

#define UINT16 unsigned int
#define UINT8  unsigned char
#define INT8  char
#define SPI_CS(x)    P4^0=(x)
#define SPI_MOSI(x)  P1^3=(x)
#define SPI_MISO()   P1^4
#define SPI_SCK(x)   P1^5=(x)

void  SpiMasterInit(void);
UINT8 SpiWriteRead(UINT8 d);

//void SpiSetSpeedLow(void);
//void SpiSetSpeedHigh(void);


#endif

