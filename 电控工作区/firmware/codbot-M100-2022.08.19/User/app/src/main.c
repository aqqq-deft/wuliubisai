/*
*********************************************************************************************************
*
*	模块名称 : 主程序
*	文件名称 : main.c
*	版    本 : V1.0
*	说    明 :
*
*   Copyright (C), 2019-2030, 武汉酷点机器人科技有限公司
*   淘宝店铺地址：https://shop559826635.taobao.com/
*********************************************************************************************************
*/
#include "includes.h"
#include "competition_config.h"
/*
**********************************************************************************************************
											宏定义
**********************************************************************************************************
*/

/*
**********************************************************************************************************
											函数声明
**********************************************************************************************************
*/
static uint8_t AppTaskCreate(void);
static void AppTaskControl(void* argument);
static void AppTaskMsgBack(void* argument);
static void AppTaskStart(void* argument);
static void AppTimerHanle(void* argument);

uint8_t ble_control(void);
/*
**********************************************************************************************************
											 变量
**********************************************************************************************************
*/
extern uint16_t g_WheelSpeed[4];

float g_pitch, g_roll, g_yaw; 	      //欧拉角


/*定时任务ID*/
osTimerId_t g_TimerHand_Id;

/* 任务的属性设置 */
const osThreadAttr_t ThreadStart_Attr =
{

    .name = "osRtxStartThread",
    .attr_bits = osThreadDetached,
    .priority = osPriorityNormal3,
    .stack_size = 4096,
};


const osThreadAttr_t ThreadControl_Attr =
{
    .name = "osRtxThreadControl",
    .attr_bits = osThreadDetached,
    .priority = osPriorityNormal2,
    .stack_size = 4096,
};

const osThreadAttr_t ThreadMsgBack_Attr =
{
    .name = "osRtxThreadMsgBack",
    .attr_bits = osThreadDetached,
    .priority = osPriorityNormal1,
    .stack_size = 1024,
};


/* 任务句柄 */
osThreadId_t ThreadIdTaskControl = NULL;
osThreadId_t ThreadIdTaskMsgBack = NULL;
osThreadId_t ThreadIdStart       = NULL;


/*
*********************************************************************************************************
*	函 数 名: main
*	功能说明: 程序主入口
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/

int main(void)
{

    System_Init();

    osKernelInitialize();

    ThreadIdStart = osThreadNew(AppTaskStart, NULL, &ThreadStart_Attr);

    osKernelStart();
    while(1);
}

/*
*********************************************************************************************************
*	函 数 名: AppObjCreate
*	功能说明: 消息队列创建
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void AppObjCreate(void)
{


}


/*
*********************************************************************************************************
*	函 数 名: AppTaskMsgBack
*	功能说明: 数据反馈
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/

static void AppTaskMsgBack(void* argument)
{
    uint8_t _CarSpeedCnt = 0;
    uint8_t _BatteryVoltage_Cnt = 0;
    uint8_t _BucketFeedback_Cnt = 0;

    while(1)
    {

        if(_CarSpeedCnt <= 1)
        {
            _CarSpeedCnt++;
        }
        else
        {
            _CarSpeedCnt = 0;

            CarSpeedInfoUpdate();

            MotorSpeedInfoUpdate();

#if COMPETITION_ENABLE_IMU != 0U
            ImuInfoUpdate();
#endif
        }

        if(_BatteryVoltage_Cnt <= 4) //调整电池电压反馈的频率
        {
            _BatteryVoltage_Cnt++;
        }
        else
        {
            _BatteryVoltage_Cnt = 0;

            BatteryVoltageInfoUpdate();

            RobotModelInfoUpdate();
        }

#if COMPETITION_ENABLE_IMU != 0U
        IMURawDataUpdate();//IMU原始数据反馈
#endif
        if(_BucketFeedback_Cnt < 19U) _BucketFeedback_Cnt++;
        else {
            _BucketFeedback_Cnt = 0U;
            Bucket_SendFeedback();
        }



        osDelay(5);
    }

}

/*
*********************************************************************************************************
*	函 数 名: AppTaskControl
*	功能说明: 运动控制
*	形    参: 无
*	返 回 值: 无
*   优 先 级: osPriorityNormal (数值越小优先级越低，这个跟uCOS相反)
*********************************************************************************************************
*/


static void AppTaskControl(void* argument)
{

    uint8_t _disLinkCnt = 0;
    uint8_t ucKeyCode ;
    LinkStatus_t _linkStatus = {0};

    /* Never inherit a non-zero motor command after reset. */
    auto_ControlCarStop();
    while(1)
    {

        /*通过蓝牙控制小车*/
        if((COMPETITION_ENABLE_BLE_CONTROL != 0U) && (ble_control() == 1))
        {
            /*如果是蓝牙控制时，清除相关标志*/
            _linkStatus.autoStatus = 0;
            _linkStatus.disLinkStatus = 0;
            _disLinkCnt = 0;
        }
        else     /*基于串口控制小车*/
        {

            if(g_uartState.uartRecvTcState == 1)/*当接受到控制信息，置位自动驾驶状态*/
            {
                g_uartState.uartRecvTcState  = 0;
                
                
                _linkStatus.autoStatus = 1;
                _linkStatus.disLinkStatus = 0;
                _disLinkCnt = 0;
            }
            else
            {

                if(_disLinkCnt < COMPETITION_COMMAND_TIMEOUT_CYCLES) /*累计没有接收到数据的次数*/
                {
                    _disLinkCnt ++;
                }
                else
                {

                    _linkStatus.disLinkStatus = 1;
                    _linkStatus.autoStatus    = 0;
                }
            }

            if(_linkStatus.autoStatus == 1)
            {

                /*只对数据部分做校验*/
                uint8_t _xorCheck = XorCheck(g_uartFrame.buf, g_uartFrame.len);
//
//				/*检查校验*/
                if(_xorCheck == g_uartFrame.xorcheck)
                {
                    /*执行自动驾驶控制命令*/
//                  printf("auto----\r\n");
                    _linkStatus.autoStatus    = 0;
                    auto_ControlCarCmdHandle();
                }

            }
            else if(_linkStatus.disLinkStatus == 1)
            {
                /*断连控制*/
//				printf("---disLink %d\r\n", _disLinkCnt);
                g_uartFrame.buf[8] = 0;
                g_uartFrame.buf[9] = 0;
                g_uartFrame.buf[10] = 0;
                auto_ControlCarStop();
                Bucket_EmergencyStop();

            }
        }


        Bucket_Update();
        osDelay(COMPETITION_CONTROL_PERIOD_MS);
    }
}

/*
*********************************************************************************************************
*	函 数 名: AppTaskStart
*	功能说明: 启动任务，这里用作BSP驱动包处理。
*	形    参: 无
*	返 回 值: 无
*   优 先 级: osPriorityNormal4
*********************************************************************************************************
*/


void AppTaskStart(void* argument)
{
    const uint16_t usFrequency = 100; /* 延迟周期 */

    uint32_t tick;

    bsp_Init();

    Bucket_Init();

    /*jlink打印*/
    /* 配置通道0，上行配置*/
    SEGGER_RTT_ConfigUpBuffer(0, "RTTUP", NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_SKIP);

    /* 配置通道0，下行配置*/
    SEGGER_RTT_ConfigDownBuffer(0, "RTTDOWN", NULL, 0, SEGGER_RTT_MODE_NO_BLOCK_SKIP);
    /* 创建任务 */
    if(AppTaskCreate() == 0U)
    {
        auto_ControlCarStop();
        while(1)
        {
            bsp_LedToggle(1);
            osDelay(50U);
        }
    }
    AppObjCreate();



    /* 获取当前时间 */
    tick = osKernelGetTickCount();

    while(1)
    {

        /* 相对延迟 */
        tick += usFrequency;
        osDelayUntil(tick);
    }
}

/*
*********************************************************************************************************
*	函 数 名: AppTimerHanle
*	功能说明: 软件定时器的回调函数，周期性执行喂狗及发送状态。
*	形    参: 无
*	返 回 值: 无
*   优 先 级: osPriorityNormal4
*********************************************************************************************************
*/

void AppTimerHanle(void* argument)
{

    bsp_LedToggle(1);//指示灯闪烁

}


/*********************************************************************************************************
*	函 数 名: AppTaskCreate
*	功能说明: 任务创建。
*	形    参: 无
*	返 回 值: 无
*   优 先 级: osPriorityNormal4
*********************************************************************************************************
*/
static uint8_t AppTaskCreate(void)
{


    ThreadIdTaskMsgBack = osThreadNew(AppTaskMsgBack, NULL, &ThreadMsgBack_Attr);
    ThreadIdTaskControl = osThreadNew(AppTaskControl, NULL, &ThreadControl_Attr);
    g_TimerHand_Id      = osTimerNew(AppTimerHanle, osTimerPeriodic, NULL, NULL);

    if((ThreadIdTaskControl == NULL) ||
       (ThreadIdTaskMsgBack == NULL) ||
       (g_TimerHand_Id == NULL) ||
       (osTimerStart(g_TimerHand_Id, 100U) != osOK))
    {
        if(g_TimerHand_Id != NULL) { osTimerDelete(g_TimerHand_Id); g_TimerHand_Id = NULL; }
        if(ThreadIdTaskMsgBack != NULL) { osThreadTerminate(ThreadIdTaskMsgBack); ThreadIdTaskMsgBack = NULL; }
        if(ThreadIdTaskControl != NULL) { osThreadTerminate(ThreadIdTaskControl); ThreadIdTaskControl = NULL; }
        return 0U;
    }
    return 1U;

}


/*蓝牙控制指令处理数据处理*/

uint8_t ble_control(void)
{
    static uint8_t  _byte = 0;
    static uint16_t _bleMoveSpeed = 0;
    static uint8_t  _bleMoveDir = 0;
    static uint8_t _bleMoveDir_Y=0;
    static uint8_t  _blePivotSteerDir = 0;
    static uint16_t  _blePivotSteerSpeed = 0;
    static uint16_t  _blePivotMoveSpeed = 350;
	    static uint16_t  _bleMoveSpeed_Y = 0;

    if(comGetChar(COM5, &_byte) != 0)
    {


        switch(_byte)
        {
        case'0':/*停车*/
        case'5':/*停车*/
            _bleMoveSpeed = 0;
            _bleMoveDir = 0;
            _blePivotSteerDir = 0;
            _blePivotSteerSpeed = 0;
            _bleMoveSpeed_Y=0;
						_bleMoveDir_Y=0;
            break;
        case'3':/*左原地转弯*/
            _blePivotSteerDir = 0;
            _blePivotSteerSpeed = 200;


            break;
        case'4':/*右原地转弯*/
            _blePivotSteerDir = 1;
            _blePivotSteerSpeed = 200;

            break;
        case'6':/*前进*/
            _bleMoveSpeed = _blePivotMoveSpeed;
            _bleMoveDir = 0;


            break;
        case'7':/*后退*/
            _bleMoveSpeed = _blePivotMoveSpeed;
            _bleMoveDir = 1;

            break;

        case'a':/*速度增加*/
            _blePivotMoveSpeed = _blePivotMoveSpeed + 50;
            if(_blePivotMoveSpeed >= 800)
            {
                _blePivotMoveSpeed = 800;
            }
            break;
        case'b':/*速度减少*/
           if(_blePivotMoveSpeed > 0)
           {
              _blePivotMoveSpeed = _blePivotMoveSpeed - 50;
           }

            if(_blePivotMoveSpeed <= 0)
            {
                _blePivotMoveSpeed = 0;
            }
            break;
        case'8':/*左平移*/
            _bleMoveDir_Y = 0;
            _bleMoveSpeed_Y = 300;
            break;
        case'9':/*右平移*/

            _bleMoveDir_Y = 1;
            _bleMoveSpeed_Y = 300;
            break;
        case'1':
            case'2':
                    break;
        default:
            auto_ControlCarStop();
        }
//        printf("_bleMoveSpeed  = %d \r\n",_bleMoveSpeed);
        MotorControl(_blePivotSteerDir, _blePivotSteerSpeed, _bleMoveDir, _bleMoveSpeed, _bleMoveDir_Y, _bleMoveSpeed_Y);
        return 1;
    }
    else
    {
        return 0;
    }

}


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
