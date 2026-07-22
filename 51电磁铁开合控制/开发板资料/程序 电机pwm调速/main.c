/**********************BST-V51实验开发板例程************************
*  平台：BST-V51+ Keil U3 + STC89C52RD
*  公司：深圳市亚博智能科技有限公司    
*  晶振:11.0592MHZ
*  说明：2个红色独立按键 分别控制加速、减速
*		数码管显示 01-18速度等级，数字越大，速度越大
******************************************************************/
#include <reg52.h>
#include <string.h>

#define DataPort P0 //定义数据端口 程序中遇到DataPort 则用P0 替换
sbit LATCH1=P2^6;//定义锁存使能端口 段锁存
sbit LATCH2=P2^7;//                 位锁存

unsigned char code dofly_DuanMa[10]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};// 显示段码值0~9
unsigned char code dofly_WeiMa[]={0xfe,0xfd,0xfb,0xf7,0xef,0xdf,0xbf,0x7f};//分别对应相应的数码管点亮,即位码
unsigned char TempData[8],start=0; //存储显示值的全局变量

sbit D1 = P1^3;
sbit K1 = P3^4;
sbit K2 = P3^5;

unsigned char Speed=1;
bit StopFlag;
void Display(unsigned char FirstBit,unsigned char Num);
void Init_Timer0(void);
unsigned char KeyScan(void);

/*协议定义*/
bit startBit = 0;  				//串口接收开始标志位
bit newLineReceived = 0; 		//串口一帧协议包接收完成
unsigned char inputString[50];  //接收数据协议
//串口初始化
void ComInit(void)
{
   
   	SCON = 0x50; 	// SCON: 模式1, 8-bit UART, 使能接收
	TMOD |= 0x20;
	TH1=0xfd; 		//波特率9600 初值
	TL1=0xfd;
	TR1= 1;
	EA = 1;	    	//开总中断
	ES= 1; 			//打开串口中断


}
/*------------------------------------------------
 uS延时函数，含有输入参数 unsigned char t，无返回值
 unsigned char 是定义无符号字符变量，其值的范围是
 0~255 这里使用晶振12M，精确延时请使用汇编,大致延时
 长度如下 T=tx2+5 uS 
------------------------------------------------*/
void DelayUs2x(unsigned char t)
{   
 while(--t);
}
/*------------------------------------------------
 mS延时函数，含有输入参数 unsigned char t，无返回值
 unsigned char 是定义无符号字符变量，其值的范围是
 0~255 这里使用晶振12M，精确延时请使用汇编
------------------------------------------------*/
void DelayMs(unsigned char t)
{
     
 while(t--)
 {
     //大致延时1mS
     DelayUs2x(245);
	 DelayUs2x(245);
 }
}

void WaitKeyFree(void){

	while(1){
		while(K1==0);
		while(K2==0);

		DelayMs(10);

		while(K1==0);
		while(K2==0);
		break;
	}


	
}
void Protocol(void)
{
	while (newLineReceived)  //协议数据接收完毕一包
	{
			//判断是否是51的协议
		if(inputString[1] != '5' || inputString[2] != '1')
		{
		 	newLineReceived = 0;  
	   		memset(inputString, 0x00, sizeof(inputString)); 
			break; 
		}
		//判断是否是风扇的协议数据
	 	if(inputString[4] != 'F' && inputString[5] != 'A' && inputString[6] != 'N')
		{
			newLineReceived = 0;  
	   		memset(inputString, 0x00, sizeof(inputString)); 
			break;
		}

		if(inputString[7] == '2') //加速
		{
			if(Speed<19)
				Speed++;
			
		}
		else if(inputString[7] == '3') //减速
		{
			if(Speed>1)
				Speed--;
			
		}
		else if(inputString[7] == '0')	 //停止
		{
			TR0 = 0;
			D1 = 0;	
		}
		else if(inputString[7] == '1')	//顺转
		{
			TR0 = 1;
		}
	
	
		newLineReceived = 0;  
   		memset(inputString, 0x00, sizeof(inputString)); 
	}
}
/*------------------------------------------------
                    主函数
------------------------------------------------*/
main()
{
	Init_Timer0();
	ComInit();
	while(1)
	{ 
		if(K1==0)//第一个按键,速度等级增加
		{
			if(Speed<19)
			Speed++;
			WaitKeyFree();
		}	
		else if(K2==0)//第二个按键，速度等级减小
		{
			if(Speed>1)
			Speed--;
			WaitKeyFree();
		}	
		TempData[0]=dofly_DuanMa[(Speed-1)/10];//分解显示信息，如要显示68，则68/10=6  68%10=8  
		TempData[1]=dofly_DuanMa[(Speed-1)%10];

	}
}

/*------------------------------------------------
 显示函数，用于动态扫描数码管
 输入参数 FirstBit 表示需要显示的第一位，如赋值2表示从第三个数码管开始显示
 如输入0表示从第一个显示。
 Num表示需要显示的位数，如需要显示99两位数值则该值输入2
------------------------------------------------*/
void Display(unsigned char FirstBit,unsigned char Num)
{
      static unsigned char i=0;
	  

	   DataPort=0;   //清空数据，防止有交替重影
       LATCH1=1;     //段锁存
       LATCH1=0;

       DataPort=dofly_WeiMa[i+FirstBit]; //取位码 
       LATCH2=1;     //位锁存
       LATCH2=0;

       DataPort=TempData[i]; //取显示数据，段码
       LATCH1=1;     //段锁存
       LATCH1=0;
       
	   i++;
       if(i==Num)
	      i=0;


}
/*------------------------------------------------
                    定时器初始化子程序
------------------------------------------------*/
void Init_Timer0(void)
{
	TMOD |= 0x01;	  //使用模式1，16位定时器，使用"|"符号可以在使用多个定时器时不受影响		     
	//TH0=0x00;	      //给定初值
	//TL0=0x00;
	EA=1;            //总中断打开
	ET0=1;           //定时器中断打开
	TR0=1;           //定时器开关打开
	PT0=1;           //优先级打开
}
/*------------------------------------------------
                 定时器中断子程序
------------------------------------------------*/
void Timer0_isr(void) interrupt 1 
{
	static unsigned char times;
	TH0=(65536-1000)/256;		  //重新赋值 1ms
	TL0=(65536-1000)%256;
	
	Display(0,8);
	if(times>(Speed-1))//最大值18，所以最小间隔值20-18=2
		D1=0;
	else
		D1=1;
	times++;
	if(times==19)
		times=0;
}

/******************************************************************/
/* 串口中断程序*/
/******************************************************************/
void UART_SER () interrupt 4
{
	unsigned char n; 	//定义临时变量
	static int num = 0;

	if(RI) 		//判断是接收中断产生
	{
		RI = 0; 	//标志位清零
		n = SBUF; //读入缓冲区的值

		//control=n;
	    if(n == '$')
	    {
	      startBit = 1;
		  num = 0;
	    }
	    if(startBit == 1)
	    {
	       inputString[num] = n; 
		   num++;    
	    }  
	    if (n == '#') 
	    {
	       newLineReceived = 1; 
	       startBit = 0;
		   Protocol();
	    }
		
		if(num >= 50)
		{
			num = 0;
			startBit = 0;
			newLineReceived	= 0;
		}
	}

}
