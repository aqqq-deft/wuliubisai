#ifndef __W25Q64_H__
#define __W25Q64_H__

/*

W25Q64是华邦公司推出的大容量SPI FLASH产品，其容量为64Mb。
该25Q系列的器件在灵活性和性能方面远远超过普通的串行闪存器件。
W25Q64将8M字节的容量分为128个块，每个块大小为64K字节，每个块又分为16个扇区，每个扇区4K个字节。
W25Q64的最小擦除单位为一个扇区，也就是每次必须擦除4K个字节。所以，这需要给W25Q64开辟一个至少4K的缓存区，这样必须要求芯片有4K以上的SRAM才能有很好的操作。 
W25Q64的擦写周期多达10W次，可将数据保存达20年之久，支持2.7~3.6V的电压，支持标准的SPI，还支持双输出/四输出的SPI，最大SPI时钟可达80Mhz。

*/

#include "stc15.h"

#define UINT32 unsigned long
#define UINT16 unsigned int
#define UINT8  unsigned char
#define INT8  char
//W25X系列/Q系列芯片列表	   
//W25Q80 ID  0XEF13
//W25Q16 ID  0XEF14
//W25Q32 ID  0XEF15
//W25Q32 ID  0XEF16	
#define W25Q80 	0XEF13 	
#define W25Q16 	0XEF14
#define W25Q32 	0XEF15
#define W25Q64 	0XEF16

sbit P4_0 = P4^0;	   
#define	SPI_FLASH_CS(x)			P4_0=(x)

#define W25Q64_SECTOR_SIZE	4096		//4K
#define W25Q64_SECTOR_NUM	2048		//8*1024/4 = 2048

#define FLASH_ASC16_ADDRESS                 0
#define FLASH_HZK16_ADDRESS                 0x1000

#define FLASH_SYSTEM_CONFIG_ADDRESS         0x43000


#define FLASH_BITMAP1_SIZE_ADDRESS	        0x50000
#define FLASH_BITMAP2_SIZE_ADDRESS	        FLASH_BITMAP1_SIZE_ADDRESS+0x28000
#define FLASH_BITMAP3_SIZE_ADDRESS	        FLASH_BITMAP2_SIZE_ADDRESS+0x28000
#define FLASH_BITMAP4_SIZE_ADDRESS	        FLASH_BITMAP3_SIZE_ADDRESS+0x28000
#define FLASH_BITMAP5_SIZE_ADDRESS	        FLASH_BITMAP4_SIZE_ADDRESS+0x28000	
#define FLASH_BITMAP6_SIZE_ADDRESS	        FLASH_BITMAP5_SIZE_ADDRESS+0x28000

#define FLASH_BITMAPMAIN_SIZE_ADDRESS       FLASH_BITMAP6_SIZE_ADDRESS+0x28000
#define FLASH_BITMAPDS1302_SIZE_ADDRESS     FLASH_BITMAPMAIN_SIZE_ADDRESS+0x28000
#define FLASH_BITMAPDS18B20_SIZE_ADDRESS    FLASH_BITMAPDS1302_SIZE_ADDRESS+0x28000
#define FLASH_BITMAPBLUETOOTH_SIZE_ADDRESS  FLASH_BITMAPDS18B20_SIZE_ADDRESS+0x28000


////////////////////////////////////////////////////////////////////////////
 
//指令表
#define W25X_WriteEnable		0x06 
#define W25X_WriteDisable		0x04 
#define W25X_ReadStatusReg		0x05 
#define W25X_WriteStatusReg		0x01 
#define W25X_ReadData			0x03 
#define W25X_FastReadData		0x0B 
#define W25X_FastReadDual		0x3B 
#define W25X_PageProgram		0x02 
#define W25X_BlockErase			0xD8 
#define W25X_SectorErase		0x20 
#define W25X_ChipErase			0xC7 
#define W25X_PowerDown			0xB9 
#define W25X_ReleasePowerDown	0xAB 
#define W25X_DeviceID			0xAB 
#define W25X_ManufactDeviceID	0x90 
#define W25X_JedecDeviceID		0x9F 

void   SpiFlashInit(void);
UINT16 SpiFlashReadID(void);  	    //读取FLASH ID
UINT8	SpiFlashReadSR(void);        //读取状态寄存器 
//void SpiFlashWriteSR(UINT8 sr);  	//写状态寄存器
void SpiFlashWriteEnable(void);  //写使能 
//void SpiFlashWriteDisable(void);	//写保护
void SpiFlashWriteNoCheck(UINT8* pBuffer,UINT32 WriteAddr,UINT16 NumByteToWrite);
void SpiFlashRead(UINT8* pBuffer,UINT32 ReadAddr,UINT16 NumByteToRead);   //读取flash
void SpiFlashWrite(UINT8* pBuffer,UINT32 WriteAddr,UINT16 NumByteToWrite);//写入flash
//void SpiFlashEraseChip(void);    	  //整片擦除
void SpiFlashEraseSector(UINT32 Dst_Addr);//扇区擦除
void SpiFlashWaitBusy(void);           //等待空闲
//void SpiFlashPowerDown(void);           //进入掉电模式
//void SpiFlashWakeUp(void);			  //唤醒
void SpiFlashWritePage(UINT8* pBuffer,UINT32 WriteAddr,UINT16 NumByteToWrite);


#define w25x_init() SpiFlashInit()
#define w25x_readId()	SpiFlashReadID()
#define w25x_read(buf, addr, len) SpiFlashRead(buf, addr, len)
#define w25x_write(buf, addr, len) SpiFlashWriteNoCheck(buf, addr, len)
#define w25x_erase_sector(addr) SpiFlashEraseSector(addr)

#endif