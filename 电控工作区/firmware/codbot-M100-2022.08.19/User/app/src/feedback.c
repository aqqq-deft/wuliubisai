/*
*********************************************************************************************************
*
*	模块名称 : 数据反馈处理
*	文件名称 : feedback.c
*	版    本 : V1.0
*	说    明 :
*
*   Copyright (C), 2019-2030, 武汉酷点机器人科技有限公司
*   淘宝店铺地址：https://shop559826635.taobao.com/
*********************************************************************************************************
*/
#include "includes.h"



long  iaAccelData[3] = {0};
short saGyroData[3] = {0};

float QuatW = 0, QuatX = 0, QuatY = 0, QuatZ = 0;
short AX = 0, AY = 0, AZ = 0;
short gx = 0, gy = 0, gz = 0;

extern uint16_t g_feedback_SteerAngle;
extern uint8_t g_feedback_SteerDirection;


/*
*********************************************************************************************************
*	函 数 名: Quat_Cal()
*	功能说明: 根据欧拉角计算四元数
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void Quat_Cal()
{
    float _inter_data = 0;
    float _roll_rad = roll * 3.1415926535898 / 180.0;
    float _pitch_rad = pitch * 3.1415926535898 / 180.0;
    float _yaw_rad = yaw * 3.1415926535898 / 180.0;


    float sr = sin(_roll_rad * 0.5);
    float cr = cos(_roll_rad * 0.5);
    float sp = sin(_pitch_rad * 0.5);
    float cp = cos(_pitch_rad * 0.5);
    float sy = sin(_yaw_rad * 0.5);
    float cy = cos(_yaw_rad * 0.5);

    float w = cr * cp * cy + sr * sp * sy;
    float x = sr * cp * cy - cr * sp * sy;
    float y = cr * sp * cy + sr * cp * sy;
    float z = cr * cp * sy - sr * sp * cy;

    //四元数归一化
    QuatW = w * 1.0 / sqrt(w * w + x * x + y * y + z * z);
    QuatX = x * 1.0 / sqrt(w * w + x * x + y * y + z * z);
    QuatY = y * 1.0 / sqrt(w * w + x * x + y * y + z * z);
    QuatZ = z * 1.0 / sqrt(w * w + x * x + y * y + z * z);

//	printf("  %f  %f  %f  %f \r\n",QuatW,QuatX,QuatY,QuatZ);

}
/*
*********************************************************************************************************
*	函 数 名: MotorSpeed_Feedback
*	功能说明: 电机转速反馈（报头:0xFD 报文长度:0x09 功能位:0x03）
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void MotorSpeedInfoUpdate()
{
    uint8_t _MotorSpeed_data[9] = {0} ;
    uint8_t _MotorSpeed_buf[12] = {0} ;


    _MotorSpeed_data[0] = 0x03;
    _MotorSpeed_data[1] = (g_WheelSpeed[0] >> 8) & 0xFF;
    _MotorSpeed_data[2] = g_WheelSpeed[0] & 0xFF;
    _MotorSpeed_data[3] = (g_WheelSpeed[1] >> 8) & 0xFF;
    _MotorSpeed_data[4] = g_WheelSpeed[1] & 0xFF;
    _MotorSpeed_data[5] = (g_WheelSpeed[2] >> 8) & 0xFF;
    _MotorSpeed_data[6] = g_WheelSpeed[2] & 0xFF;
    _MotorSpeed_data[7] = (g_WheelSpeed[3] >> 8) & 0xFF;
    _MotorSpeed_data[8] = g_WheelSpeed[3] & 0xFF;

    _MotorSpeed_buf[0]  = 0xFD;
    _MotorSpeed_buf[1]  = 0x09;
    _MotorSpeed_buf[2]  = _MotorSpeed_data[0];
    _MotorSpeed_buf[3]  = _MotorSpeed_data[1];
    _MotorSpeed_buf[4]  = _MotorSpeed_data[2];
    _MotorSpeed_buf[5]  = _MotorSpeed_data[3];
    _MotorSpeed_buf[6]  = _MotorSpeed_data[4];
    _MotorSpeed_buf[7]  = _MotorSpeed_data[5];
    _MotorSpeed_buf[8]  = _MotorSpeed_data[6];
    _MotorSpeed_buf[9]  = _MotorSpeed_data[7];
    _MotorSpeed_buf[10] = _MotorSpeed_data[8];
    _MotorSpeed_buf[11] = XorCheck(_MotorSpeed_data, 9);


    comSendBuf(COM1, _MotorSpeed_buf, 12);
}
/*
*********************************************************************************************************
*	函 数 名: CarSpeed_Feedback
*	功能说明: 小车速度及转向角度反馈（报头:0xFD 报文长度:0x0A 功能位:0x02）
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void CarSpeedInfoUpdate()
{

    uint8_t _ucCarSpeedbuf[13] = {0};

    uint16_t _usCarSpeedX = 0;
    uint16_t _usCarSpeedY = 0;

    uint16_t _usAngularVelocity = 0;
		
		
	  int16_t _usMotorA_Speed_M = 0;
    int16_t _usMotorB_Speed_M = 0;
    int16_t _usMotorC_Speed_M = 0;
    int16_t _usMotorD_Speed_M = 0;
		
		int16_t _usMotionSpeed_M = 0;
		int16_t _ucAngularSpeed_M = 0;
	
		int16_t _usMotionSpeed_Y_M = 0;
//给编码器反馈的值赋值		
		if(g_EncoderValue.MotorA_EncoderValue.dir == 0)
		{
		   _usMotorA_Speed_M=g_WheelSpeed[0];
		
		}
		else
		{		
				   _usMotorA_Speed_M=-g_WheelSpeed[0];
		}
				if(g_EncoderValue.MotorB_EncoderValue.dir == 0)
		{
		   _usMotorB_Speed_M=g_WheelSpeed[1];
		
		}
		else
		{		
				   _usMotorB_Speed_M=-g_WheelSpeed[1];
		}
				if(g_EncoderValue.MotorC_EncoderValue.dir == 1)
		{
		   _usMotorC_Speed_M=g_WheelSpeed[2];
		
		}
		else
		{		
				   _usMotorC_Speed_M=-g_WheelSpeed[2];
		}
						if(g_EncoderValue.MotorD_EncoderValue.dir == 1)
		{
		   _usMotorD_Speed_M=g_WheelSpeed[3];
		
		}
		else
		{		
				   _usMotorD_Speed_M=-g_WheelSpeed[3];
		}
		//调研麦轮速度模型，根绝电机的速度反推整车的速度和角速度
		 _usMotionSpeed_M = 0.25*(_usMotorA_Speed_M+_usMotorB_Speed_M+_usMotorC_Speed_M+_usMotorD_Speed_M);
		 _ucAngularSpeed_M = 0.25*100*(_usMotorC_Speed_M+_usMotorD_Speed_M-_usMotorA_Speed_M-_usMotorB_Speed_M)/ (110+92.5);	
		_usMotionSpeed_Y_M = 0.5*(_usMotorA_Speed_M-_usMotorB_Speed_M);
			//整车计算的速度和角速度赋给要反馈的数组	
		if(_usMotionSpeed_M>=0)
		{
		_ucCarSpeedbuf[3] = 0;
		_usCarSpeedX=	_usMotionSpeed_M;
		
		}
		else
		{
		_ucCarSpeedbuf[3] = 1;
		_usCarSpeedX=	-_usMotionSpeed_M;				
		}
		
		
				if(_usMotionSpeed_Y_M>=0)
		{
		_ucCarSpeedbuf[6] = 1;
		_usCarSpeedY=	_usMotionSpeed_Y_M;
		
		}
		else
		{
		_ucCarSpeedbuf[6] = 0;
		_usCarSpeedY=	-_usMotionSpeed_Y_M;				
		}

				if(_ucAngularSpeed_M>=0)
		{
		_ucCarSpeedbuf[9] = 0;
		_usAngularVelocity=	_ucAngularSpeed_M;
		
		}
		else
		{
		_ucCarSpeedbuf[9] = 1;
		_usAngularVelocity=	-_ucAngularSpeed_M;				
		}		
				
    _ucCarSpeedbuf[0] = 0xFD;
    _ucCarSpeedbuf[1] = 0x0A;
    _ucCarSpeedbuf[2] = 02;
    _ucCarSpeedbuf[4] = (_usCarSpeedX >> 8) & 0xFF;
    _ucCarSpeedbuf[5] = _usCarSpeedX & 0xFF;
  
    _ucCarSpeedbuf[7] = (_usCarSpeedY >> 8) & 0xFF;
    _ucCarSpeedbuf[8] = _usCarSpeedY & 0xFF;


    _ucCarSpeedbuf[10] = (_usAngularVelocity >> 8) & 0xFF;
    _ucCarSpeedbuf[11] = _usAngularVelocity & 0xFF;

    _ucCarSpeedbuf[12] = xorCheckCalc(_ucCarSpeedbuf, 2, 10);


    comSendBuf(COM1, _ucCarSpeedbuf, 13);
}
/*
*********************************************************************************************************
*	函 数 名: IMU_Data_Feedback
*	功能说明: IMU数据反馈（报头:0xFD 报文长度:0x0A 功能位:0x05,精度0.01°）
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/

void ImuInfoUpdate()
{

    uint8_t _ucImuData[10] = {0};
    uint8_t _ucImuBuf[13] = {0};
    float _pitch = pitch;
    float _roll = roll;
    float _yaw = yaw;

    _ucImuData[0] = 0x05;

    if(_pitch >= 0)
    {
        _ucImuData[1] = 0x00;
    }
    else
    {
        _ucImuData[1] = 0x01;
        _pitch = -_pitch;
    }

    _ucImuData[2] = (((int16_t)(_pitch * 100)) >> 8) & 0xFF;
    _ucImuData[3] = ((int16_t)(_pitch * 100)) & 0xFF;

    if(_roll >= 0)
    {
        _ucImuData[4] = 0x00;
    }
    else
    {
        _ucImuData[4] = 0x01;
        _roll = -_roll;
    }

    _ucImuData[5] = (((int16_t)(_roll * 100)) >> 8) & 0xFF;
    _ucImuData[6] = ((int16_t)(_roll * 100)) & 0xFF;

    if(_yaw >= 0)
    {
        _ucImuData[7] = 0x00;
    }
    else
    {
        _ucImuData[7] = 0x01;
        _yaw = -_yaw;
    }

    _ucImuData[8] = (((int16_t)(_yaw * 100)) >> 8) & 0xFF;
    _ucImuData[9] = ((int16_t)(_yaw * 100)) & 0xFF;



    _ucImuBuf[0]  = 0xFD;
    _ucImuBuf[1]  = 0x0a;
    _ucImuBuf[2]  = _ucImuData[0];
    _ucImuBuf[3]  = _ucImuData[1];
    _ucImuBuf[4]  = _ucImuData[2];
    _ucImuBuf[5]  = _ucImuData[3];
    _ucImuBuf[6]  = _ucImuData[4];
    _ucImuBuf[7]  = _ucImuData[5];
    _ucImuBuf[8]  = _ucImuData[6];
    _ucImuBuf[9]  = _ucImuData[7];
    _ucImuBuf[10] = _ucImuData[8];
    _ucImuBuf[11] = _ucImuData[9];

    _ucImuBuf[12] = XorCheck(_ucImuData, 10);
    comSendBuf(COM1, _ucImuBuf, 13);
}

/*
*********************************************************************************************************
*	函 数 名: BatteryVoltage_Feedback
*	功能说明: 电池电压反馈（报头:0xFD 报文长度:0x02 功能位:0x04,精度0.1V）
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void BatteryVoltageInfoUpdate()
{
    uint16_t _usAdcNum;
    float _tempVoltage;
    float _voltage;

    uint8_t _ucVoltageBuf[5] = {0};

    _usAdcNum = getAdcAverageValue(ADC_CHANNEL_15, 20); //获取通道1的转换值，20次取平均
    _tempVoltage = (float)_usAdcNum * (3.3 / 4096);
    _voltage = _tempVoltage * 12 * 11.6 / (3.3 * 9.8); //校正获取的电压值

    _ucVoltageBuf[0] = 0xFD;
    _ucVoltageBuf[1] = 0x02;
    _ucVoltageBuf[2] = 0x04;
    _ucVoltageBuf[3] = (uint8_t)(_voltage * 10);
    _ucVoltageBuf[4] = xorCheckCalc(_ucVoltageBuf, 2, 2);

    comSendBuf(COM1, _ucVoltageBuf, 5);
//		printf("adcx: %d temp: %f temp1: %f \r\n", _adc_num,_temp_voltage,_voltage);

}
/*
*********************************************************************************************************
*	函 数 名: IMU_RawData_Feedback
*	功能说明: IMU原始数据反馈（报头:FD ）
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/

void IMURawDataUpdate()
{
    uint8_t _ucImuRawDatabuf[34] = {0};
    short _sAccelX = 0, _sAccelY = 0, _sAccelZ = 0; //加速度
    short _sGyroX  = 0, _sGyroY  = 0, _sGyroZ = 0; //陀螺仪值
    short _sQuatW  = 0, _sQuatX  = 0, _sQuatY = 0, _sQuatZ = 0;

    Quat_Cal();//四元素计算

    _sGyroX = (saGyroData[0] * 1000 * 3.1415926535898 / 180.0) / 16.4; //X轴角速度，单位为(1/1000)rad/s
    _sGyroY = ((saGyroData[1] * 1000 * 3.1415926535898 / 180.0) / 16.4); //y轴角速度，单位为(1/1000)rad/s
    _sGyroZ = ((saGyroData[2] * 1000 * 3.1415926535898 / 180.0) / 16.4); //z轴角速度，单位为(1/1000)rad/s



    _sAccelX = (short)((iaAccelData[0] * 10000) / 16384); //X轴加速度，单位为(1/10000)g/s
    _sAccelY = (short)((iaAccelData[1] * 10000) / 16384); //y轴加速度，单位为(1/10000)g/s
    _sAccelZ = (short)((iaAccelData[2] * 10000) / 16384); //z轴加速度，单位为(1/10000)g/s




    _sQuatW = (short)(QuatW * 10000); //四元素数据*10000
    _sQuatX = (short)(QuatX * 10000);
    _sQuatY = (short)(QuatY * 10000);
    _sQuatZ = (short)(QuatZ * 10000);


    _ucImuRawDatabuf[0] = 0xFD;
    _ucImuRawDatabuf[1] = 0x1F;
    _ucImuRawDatabuf[2] = 0x06;

    if(_sGyroX >= 0)
    {
        _ucImuRawDatabuf[3] = 0x00;
    }
    else
    {
        _ucImuRawDatabuf[3] = 0x01;
        _sGyroX = -_sGyroX;
    }

    _ucImuRawDatabuf[4] = (_sGyroX >> 8) & 0xFF;
    _ucImuRawDatabuf[5] = (_sGyroX) & 0xFF;

    if(_sGyroY >= 0)
    {
        _ucImuRawDatabuf[6] = 0x00;
    }
    else
    {
        _ucImuRawDatabuf[6] = 0x01;
        _sGyroY = -_sGyroY;
    }

    _ucImuRawDatabuf[7] = (_sGyroY >> 8) & 0xFF;
    _ucImuRawDatabuf[8] = (_sGyroY) & 0xFF;

    if(_sGyroZ >= 0)
    {
        _ucImuRawDatabuf[9] = 0x00;
    }
    else
    {
        _ucImuRawDatabuf[9] = 0x01;
        _sGyroZ = -_sGyroZ;
    }

    _ucImuRawDatabuf[10] = (_sGyroZ >> 8) & 0xFF;
    _ucImuRawDatabuf[11] = (_sGyroZ) & 0xFF;

    if(_sAccelX >= 0)
    {
        _ucImuRawDatabuf[12] = 0x00;
    }
    else
    {
        _ucImuRawDatabuf[12] = 0x01;
        _sAccelX = -_sAccelX;
    }
    _ucImuRawDatabuf[13] = (_sAccelX >> 8) & 0xFF;
    _ucImuRawDatabuf[14] = (_sAccelX) & 0xFF;

    if(_sAccelY >= 0)
    {
        _ucImuRawDatabuf[15] = 0x00;
    }
    else
    {
        _ucImuRawDatabuf[15] = 0x01;
        _sAccelY = -_sAccelY;
    }

    _ucImuRawDatabuf[16] = (_sAccelY >> 8) & 0xFF;
    _ucImuRawDatabuf[17] = (_sAccelY) & 0xFF;

    if(_sAccelZ >= 0)
    {
        _ucImuRawDatabuf[18] = 0x00;
    }
    else
    {
        _ucImuRawDatabuf[18] = 0x01;
        _sAccelZ = -_sAccelZ;
    }

    _ucImuRawDatabuf[19] = (_sAccelZ >> 8) & 0xFF;
    _ucImuRawDatabuf[20] = (_sAccelZ) & 0xFF;

    if(_sQuatW >= 0)
    {
        _ucImuRawDatabuf[21] = 0x00;
    }
    else
    {
        _ucImuRawDatabuf[21] = 0x01;
        _sQuatW = -_sQuatW;
    }

    _ucImuRawDatabuf[22] = (_sQuatW >> 8) & 0xFF;
    _ucImuRawDatabuf[23] = (_sQuatW) & 0xFF;

    if(_sQuatX >= 0)
    {
        _ucImuRawDatabuf[24] = 0x00;
    }
    else
    {
        _ucImuRawDatabuf[24] = 0x01;
        _sQuatX = -_sQuatX;
    }

    _ucImuRawDatabuf[25] = (_sQuatX >> 8) & 0xFF;
    _ucImuRawDatabuf[26] = (_sQuatX) & 0xFF;

    if(_sQuatY >= 0)
    {
        _ucImuRawDatabuf[27] = 0x00;
    }
    else
    {
        _ucImuRawDatabuf[27] = 0x01;
        _sQuatY = -_sQuatY;
    }

    _ucImuRawDatabuf[28] = (_sQuatY >> 8) & 0xFF;
    _ucImuRawDatabuf[29] = (_sQuatY) & 0xFF;

    if(_sQuatZ >= 0)
    {
        _ucImuRawDatabuf[30] = 0x00;
    }
    else
    {
        _ucImuRawDatabuf[30] = 0x01;
        _sQuatZ = -_sQuatZ;
    }
    _ucImuRawDatabuf[31] = (_sQuatZ >> 8) & 0xFF;
    _ucImuRawDatabuf[32] = (_sQuatZ) & 0xFF;


    _ucImuRawDatabuf[33] = xorCheckCalc(_ucImuRawDatabuf, 2, 31);

    comSendBuf(COM1, _ucImuRawDatabuf, 34);

//	printf(" %X  %X %X %X  %X %X %X  %X %X %X  %X %X %X  %X %X %X  %X %X %X  %X %X %X  %X %X %X  %X %X %X  %X %X %X  %X %X %X\r\n", _IMU_RawData_buf[0],_IMU_RawData_buf[1],_IMU_RawData_buf[2], _IMU_RawData_buf[3],_IMU_RawData_buf[4],_IMU_RawData_buf[5], _IMU_RawData_buf[6],_IMU_RawData_buf[7],_IMU_RawData_buf[8], _IMU_RawData_buf[9],_IMU_RawData_buf[10],_IMU_RawData_buf[11], _IMU_RawData_buf[12],_IMU_RawData_buf[13],_IMU_RawData_buf[14], _IMU_RawData_buf[15],_IMU_RawData_buf[16],_IMU_RawData_buf[17], _IMU_RawData_buf[18],_IMU_RawData_buf[19],_IMU_RawData_buf[20], _IMU_RawData_buf[21],_IMU_RawData_buf[22],_IMU_RawData_buf[23], _IMU_RawData_buf[24],_IMU_RawData_buf[25],_IMU_RawData_buf[26], _IMU_RawData_buf[27],_IMU_RawData_buf[28],_IMU_RawData_buf[29], _IMU_RawData_buf[30],_IMU_RawData_buf[31],_IMU_RawData_buf[32],_IMU_RawData_buf[33]);
}
/*
*********************************************************************************************************
*	函 数 名: Robot_Model_Feedback
*	功能说明: 底盘型号 ：0x01 阿克曼车型  0x02四轮差速车 0x03麦克纳母轮车 0x04履带车
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void RobotModelInfoUpdate()
{
    uint8_t _ucaRobotModelBuf[5] = {0};

    _ucaRobotModelBuf[0] = 0xFD;
    _ucaRobotModelBuf[1] = 0x02;
    _ucaRobotModelBuf[2] = 0x07; //功能位0x07
    _ucaRobotModelBuf[3] = 0x03; /* Mecanum four-wheel chassis */
    _ucaRobotModelBuf[4] = xorCheckCalc(_ucaRobotModelBuf, 2, 2);
    comSendBuf(COM1, _ucaRobotModelBuf, 5);
}