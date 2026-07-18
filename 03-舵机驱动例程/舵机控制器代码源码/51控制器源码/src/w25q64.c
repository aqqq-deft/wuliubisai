#include "w25q64.h"
#include "spi.h"


/*
void Delayus(UINT16 i )
{
	unsigned char j = 0;
	for (;i > 0; i--);
}*/


/****************************************
*函数名称:SpiFlashInit
*输    入:无
*输    出:无
*功    能:初始化SPI FLASH的IO口
******************************************/
void SpiFlashInit(void)
{
     /* Flash CS P1.0推挽输出*/
	 P4M1&=~(1<<0);
	 P4M0&=  ~(1<<0);
	 
     SpiMasterInit();
}  
/****************************************
*函数名称:SpiFlashReadSR
*输    入:无
*输    出:忙标记位(1,忙;0,空闲)
*功    能:读取SPI_FLASH的状态寄存器
BIT7  6   5   4   3   2   1   0
SPR   RV  TB BP2 BP1 BP0 WEL BUSY
SPR:默认0,状态寄存器保护位,配合WP使用
TB,BP2,BP1,BP0:FLASH区域写保护设置
WEL:写使能锁定
BUSY:忙标记位(1,忙;0,空闲)
默认:0x00
******************************************/
UINT8 SpiFlashReadSR(void)   
{  
	UINT8 rt=0;   
	SPI_FLASH_CS(0);                      //使能器件   
	SpiWriteRead(W25X_ReadStatusReg);    //发送读取状态寄存器命令    
	rt=SpiWriteRead(0Xff);               //读取一个字节  
	SPI_FLASH_CS(1);                      //取消片选     
	return rt;   
} 
/****************************************
*函数名称:SpiFlashWriteSR
*输    入:设置状态值
*输    出:无
*功    能:写SPI_FLASH的状态寄存器
只有SPR,TB,BP2,BP1,BP0(bit 7,5,4,3,2)可以写
*****************************************
void SpiFlashWriteSR(UINT8 sr)   
{   
	SPI_FLASH_CS(0);                      //使能器件   
	SpiWriteRead(W25X_WriteStatusReg);   //发送写取状态寄存器命令    
	SpiWriteRead(sr);                    //写入一个字节  
	SPI_FLASH_CS(1);                      //取消片选     	      
}   */

/****************************************
*函数名称:SpiFlashWriteEnable
*输    入:无
*输    出:无
*功    能:SPI_FLASH写使能，WEL置位
******************************************/
void SpiFlashWriteEnable(void)   
{
	SPI_FLASH_CS(0);                      //使能器件   
  SpiWriteRead(W25X_WriteEnable);      //发送写使能  
	SPI_FLASH_CS(1);                      //取消片选     	      
} 
/****************************************
*函数名称:SpiFlashWriteDisable
*输    入:无
*输    出:无
*功    能:SPI_FLASH写禁止，将WEL清零  	
*****************************************
void SpiFlashWriteDisable(void)   
{  
	SPI_FLASH_CS(0);                       //使能器件   
  SpiWriteRead(W25X_WriteDisable);      //发送写禁止指令    
	SPI_FLASH_CS(1);                       //取消片选     	      
} 		*/  

/****************************************
*函数名称:SpiFlashReadID
*输    入:无
*输    出:			   
					0XEF13,表示芯片型号为W25Q80  
					0XEF14,表示芯片型号为W25Q16    
					0XEF15,表示芯片型号为W25Q32  
					0XEF16,表示芯片型号为W25Q64 
*功    能:读取芯片ID
******************************************/
UINT16 SpiFlashReadID(void)
{
	UINT16 Temp = 0;	  
	SPI_FLASH_CS(0);				    
	SpiWriteRead(0x90);             //发送读取ID命令	    
	SpiWriteRead(0x00); 	    
	SpiWriteRead(0x00); 	    
	SpiWriteRead(0x00); 	 			   
	Temp|=SpiWriteRead(0xFF)<<8;  
	Temp|=SpiWriteRead(0xFF);	 
	SPI_FLASH_CS(1);				    
	return Temp;
}   		    
/****************************************
*函数名称:SpiFlashRead
*输    入:pBuffer        -数据存储区
          ReadAddr       -开始读取的地址(24bit)
          NumByteToRead  -要读取的字节数(最大65535)
*输    出:无
*功    能:读取SPI FLASH  
******************************************/
void SpiFlashRead(UINT8* pBuffer,UINT32 ReadAddr,UINT16 NumByteToRead)   
{ 
 	UINT16 i;   										    
	SPI_FLASH_CS(0);                         //使能器件   
  SpiWriteRead(W25X_ReadData);            //发送读取命令   
  SpiWriteRead((UINT8)((ReadAddr)>>16));  //发送24bit地址    
  SpiWriteRead((UINT8)((ReadAddr)>>8));   
  SpiWriteRead((UINT8)(ReadAddr&0xFF));   
	
  for(i=0;i<NumByteToRead;i++)
	{ 
      pBuffer[i]=SpiWriteRead(0XFF);      //循环读数  
  }
	
	SPI_FLASH_CS(1);  				    	      
}  
/****************************************
*函数名称:SpiFlashWritePage
*输    入:pBuffer        -数据存储区
          WriteAddr      -开始写入的地址(24bit)
          NumByteToWrite -要写入的字节数(最大256),该数不应该超过该页的剩余字节数
*输    出:无
*功    能:SPI在一页(0~65535)内写入少于256个字节的数据
******************************************/
void SpiFlashWritePage(UINT8* pBuffer,UINT32 WriteAddr,UINT16 NumByteToWrite)
{
 	UINT16 i; 
	
  SpiFlashWriteEnable();                  //SET WEL 
	SPI_FLASH_CS(0);                           //使能器件
	
  SpiWriteRead(W25X_PageProgram);           //发送写页命令   
  SpiWriteRead((UINT8)((WriteAddr)>>16));   //发送24bit地址    
  SpiWriteRead((UINT8)((WriteAddr)>>8));   
  SpiWriteRead((UINT8)WriteAddr);
	
  for(i=0;i<NumByteToWrite;i++)SpiWriteRead(pBuffer[i]);//循环写数  
	
	SPI_FLASH_CS(1);                            //取消片选 
	SpiFlashWaitBusy();					            //等待写入结束
} 
/****************************************
*函数名称:SpiFlashWriteNoCheck
*输    入:pBuffer        -数据存储区
          WriteAddr      -开始写入的地址(24bit)
          NumByteToWrite -要写入的字节数(最大65535)
*输    出:无
*功    能:无检验写SPI FLASH 
必须确保所写的地址范围内的数据全部为0XFF,
否则在非0XFF处写入的数据将失败!
具有自动换页功能 
在指定地址开始写入指定长度的数据,但是要确保地址不越界!
******************************************/
void SpiFlashWriteNoCheck(UINT8* pBuffer,UINT32 WriteAddr,UINT16 NumByteToWrite)   
{ 			 		 
	UINT16 pageremain;	   
	pageremain=256-WriteAddr%256; //单页剩余的字节数		 	    
	if(NumByteToWrite<=pageremain)pageremain=NumByteToWrite;//不大于256个字节
	
	while(1)
	{	   
		SpiFlashWritePage(pBuffer,WriteAddr,pageremain);
		if(NumByteToWrite==pageremain)break;//写入结束了
	 	else //NumByteToWrite>pageremain
		{
			pBuffer+=pageremain;
			WriteAddr+=pageremain;	

			NumByteToWrite-=pageremain;			      //减去已经写入了的字节数
			if(NumByteToWrite>256)pageremain=256; //一次可以写入256个字节
			else pageremain=NumByteToWrite; 	    //不够256个字节了
		}
	}	    
} 

/****************************************
*函数名称:SpiFlashWrite
*输    入:pBuffer        -数据存储区
          WriteAddr      -开始写入的地址(24bit)
          NumByteToWrite -要写入的字节数(最大65535)
*输    出:无
*功    能:写SPI FLASH  
在指定地址开始写入指定长度的数据并带擦除操作!
******************************************/
void SpiFlashWrite(UINT8* pBuffer,UINT32 WriteAddr,UINT16 NumByteToWrite)   
{ 
	UINT32   secpos = 0;
	//secpos=WriteAddr>>9;//扇区地址  
	//SpiFlashEraseSector(secpos);//擦除这个扇区
	SpiFlashWriteNoCheck(pBuffer,WriteAddr,NumByteToWrite);//写入整个扇区  
}
/****************************************
*函数名称:SpiFlashEraseChip
*输    入:无
*输    出:无
*功    能:擦除整个芯片		  
*****************************************
void SpiFlashEraseChip(void)   
{                                   
    SpiFlashWriteEnable();             //SET WEL 
    SpiFlashWaitBusy();   
  	SPI_FLASH_CS(0);                      //使能器件   
    SpiWriteRead(W25X_ChipErase);        //发送片擦除命令  
	  SPI_FLASH_CS(1);                      //取消片选     	      
	  SpiFlashWaitBusy();   				      //等待芯片擦除结束
}   */
/****************************************
*函数名称:SpiFlashEraseSector
*输    入:Dst_Addr  -扇区地址 根据实际容量设置
*输    出:无
*功    能:擦除一个扇区最少150毫秒
******************************************/
void SpiFlashEraseSector(UINT32 Dst_Addr)   
{  
 
 	Dst_Addr <<= 12;
    SpiFlashWriteEnable();                //SET WEL 	 
    SpiFlashWaitBusy();   
  	SPI_FLASH_CS(0);                         //使能器件   
    SpiWriteRead(W25X_SectorErase);         //发送扇区擦除指令 
    SpiWriteRead((UINT8)((Dst_Addr)>>16));  //发送24bit地址    
    SpiWriteRead((UINT8)((Dst_Addr)>>8));   
    SpiWriteRead((UINT8)Dst_Addr);  
	SPI_FLASH_CS(1);                         //取消片选     	      
    SpiFlashWaitBusy();   				         //等待擦除完成
}  
/****************************************
*函数名称:SpiFlashWaitBusy
*输    入:无
*输    出:无
*功    能:等待空闲
******************************************/
void SpiFlashWaitBusy(void)   
{   
	while((SpiFlashReadSR()&0x01)==0x01);  // 等待BUSY位清空
}  
/****************************************
*函数名称:SpiFlashPowerDown
*输    入:无
*输    出:无
*功    能:进入掉电模式
*****************************************
void SpiFlashPowerDown(void)   
{ 
  	SPI_FLASH_CS(0);                         //使能器件   
    SpiWriteRead(W25X_PowerDown);           //发送掉电命令  
	  SPI_FLASH_CS(1);                         //取消片选     	      
    Delayus(3);                              //等待TPD  
} */  
/****************************************
*函数名称:SpiFlashWakeUp
*输    入:无
*输    出:无
*功    能:唤醒
*****************************************
void SpiFlashWakeUp(void)   
{  
  	SPI_FLASH_CS(0);                            //使能器件   
    SpiWriteRead(W25X_ReleasePowerDown);       //发送唤醒指令 
	SPI_FLASH_CS(1);                            //取消片选     	      
    Delayus(3);                                 //等待TRES1
}*/   


























