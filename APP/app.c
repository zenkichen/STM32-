/****************************************************************************
* Copyright (C), 2010 奋斗嵌入式工作室 ourstm.5d6d.com
*
* 本例程在 奋斗版STM32开发板V2,2.1,V3,MINI上调试通过           
* QQ: 9191274, 旺旺：sun68, Email: sun68@163.com 
* 淘宝店铺：ourstm.taobao.com  
*
* 文件名: app.c
* 内容简述:
*       本例程操作系统采用ucos2.86a版本， 建立了5个任务
			任务名											 优先级
			APP_TASK_START_PRIO                               2	        主任务	  		
            Task_Com1_PRIO                                    4			COM1通信任务
            Task_Led1_PRIO                                    7			LED1 闪烁任务
            Task_Led2_PRIO                                    8			LED2 闪烁任务
            Task_Led3_PRIO                                    9			LED3 闪烁任务
		 当然还包含了系统任务：
		    OS_TaskIdle                  空闲任务-----------------优先级最低
			OS_TaskStat                  统计运行时间的任务-------优先级次低
*
* 文件历史:
* 版本号  日期       作者    说明
* v0.1    2010-11-12 sun68  创建该文件
*
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/
#define GLOBALS

#include "stdarg.h"
//#include <stdlib.h>
//#include <stdio.h>
//#include <time.h>

#include "includes.h"
#include "globals.h"
#include "../LCD_Driver/LCD_Dis.h"
#include "../LCD_Driver/LCD_Config.h"
#include "SPI_Flash.h"
#include "Fifo4Serial.h"
#include "icon.h"

#define  N_MESSAGES   32
void *MsgGrp[N_MESSAGES];    //定义消息指针数组
void *MsgMainGrp[N_MESSAGES];    //定义消息指针数组

// 显示屏编号定义
#define SCR_MAIN 0x01
// 屏幕宽
#define SCR_WIDTH 480
//屏幕高
#define SCR_HEIGHT 320
#define MENU_COLUMN_1 0+24
#define MENU_COLUMN_2 SCR_WIDTH/2+24

// 包头定义
#define PACK_HEAD1 0x59
#define PACK_HEAD2 0x47
#define PACK_VER  0x01

#define COLOR_FONT      0xFFFF             //文字颜色
#define COLOR_SELECTED  0xF800         //菜单选中颜色
#define COLOR_BG    0x8E04//0x64BD                  //背景颜色
#define HIGH_LIGHT_HOUR 6// 亮度大的时间点
#define LOW_LIGHT_HOUR 18//低亮度开始时间点
#define HIGH_LIGHT_LEVEL 9//最大亮度
#define LOW_LIGHT_LEVEL 1//最小亮度
#define ADMIN_PWD_LEN 6//管理员密码长度

//无操作计次，单位s
#define CODE_NO_OPERATE 5//暗码
#define PAGE_NO_OPERATE 10//页面
#define BARCODE_NO_OPERATE 20// 条形码
#define DRAW_SPAN_S 5//绘制停顿
#define DRAW_SPAN_B 20//绘制停顿

#define DOOR_ALL 0xFF// 操作所有柜门的柜门值

#define ANY_WAY 0x0A//
#define DRAW_ALL 0xFF//画全部
/////////////
#define DIS_BKLIGHT 0x00
#define DIS_TIME       (u8)0x01
#define DIS_BGIMG       0x02
#define DIS_MAIN        0x03//主界面
#define DIS_ADMIN_PWD        0x04//管理员主界面
#define DIS_CARD      0x05
#define DIS_GPRS_STATUS      0x31
#define DIS_GPRS_SIG      0x06
//////////////
#define KEY_1 0x31;
#define KEY_2 0x32;
#define KEY_3 0x33;
#define KEY_4 0x34;
#define KEY_5 0x35;
#define KEY_6 0x36;
#define KEY_7 0x37;
#define KEY_8 0x38;
#define KEY_9 0x39;
#define KEY_0 0x30;
#define KEY_ENTER     0x0F//key 回车
#define KEY_CANCEL     0x0C//key 取消
#define KEY_GET      0x0D//key 取
#define KEY_PUT      0x0E//key 存
#define KEY_UP      0x0A//key 上
#define KEY_DOWN     0x0B//key 下


/////////功能模式
#define MOD_MAIN                        0x01//主界面功能
#define MOD_ADMIN_PWD_IN       0x02//管理员输入密码功能
#define MOD_ADMIN_MENU           0x03
#define MOD_SET_IP                      0x04
#define MOD_SET_PORT                0x05
#define MOD_SET_GUIZI_ID               0x06
#define MOD_OPEN_DOOR       0x07


#define GPRS_ST_NO_MODEL  0x01
#define GPRS_ST_NO_SIMCARD 0x02
#define GPRS_ST_NO_NET 0x03
#define GPRS_ST_CONNECTED  0x04
#define GPRS_CMCC 0x05
#define GPRS_UNION 0x06
#define GPRS_ST_CON_FAIL 0x07

#define GUIZI_NUM 24//箱门数

#define setbit(x,y) x|=(1<<y) //将X的第Y位置1
#define clrbit(x,y) x&=~(1<<y) //将X的第Y位清0


//柜门信息
typedef struct
{
    char door_id[3];//箱门编号
    unsigned char status;//箱门状态；0:空 1:满 2:锁
    unsigned char doorstatus;//箱门关闭状态；1:关 0:开
    char userCard[9];//用户卡号
    char userPwd[7];//用户密码
}guiziInfo;

//柜子信息
typedef struct
{
    char isUsed;
    unsigned int year,month,day,hour,minute,second;//时间信息
    char guizi_id[9];//柜子ID
    char superAdminPwd[7];//超级管理员密码
    char adminPwd[7];//管理员密码
    char servIP[16];//服务器IP
    int port;//服务器端口
    int door_num;//箱门数量
    char compName[21];//公司名称
    unsigned int compNameLen;//公司名称长度
    guiziInfo guizi[GUIZI_NUM];//箱门信息
}sdata;

//联合体
typedef union
{
    char dataBuff[sizeof(sdata)];    
    sdata sysData;
}store_data;

store_data sysInfo;


//FLASH中存放数据定义
unsigned char adminEnterCode[]="888888";//进入管理界面的暗码
unsigned char time_data[]="2014-06-30 06:51:00";// 系统时间
unsigned char test_text[]="this is a test";
unsigned char str_test_gprs[]="模块检查";
unsigned char str_no_card[]="无上网卡";
unsigned char str_con_ing[]="正在连接";
unsigned char str_CMCC[]="中国移动";
unsigned char str_unicom[]="中国联通";
unsigned char str_apn_cmcc[]="CMNET";
unsigned char str_apn_unicom[]="3GNET";
unsigned char *strGprsStatus=str_test_gprs;
unsigned char gprs_send_go[]={0x1A,'\0'};

u16 mode=MOD_MAIN;
u16 screen_num=SCR_MAIN;
u16 gprs_status=GPRS_ST_NO_MODEL;
u8 gprs_strength=0;// 信号强度
u8 gprs_strength_dis=1;// 信号强度显示
u8 gprs_cops=0;
u8 barcode_on=0;// 条形码模块打开状态
u8 barcodeShutCount=0;// 条形码关闭计次
u8 noOperateCount=0;//无操作计次
u8 noOperateCountPage=0;//二级页面无操作计次
u8 scrBackLight=HIGH_LIGHT_LEVEL;// 屏幕亮度
int timeCount=0;
u8 dooroperate=0;// 是否操作柜子的开关
u16 dooroperatenum=1;// 被操作的柜门号

OS_EVENT* Screen_OSQ;// 屏幕更新消息队列
OS_EVENT* main_OSQ;
OS_EVENT* iccard_rev_MBOX;
OS_EVENT* barcode_rev_MBOX;
OS_EVENT* gprs_rev_MBOX;
OS_EVENT* keyEventMBOX;
// 显示屏资源独占互斥信号灯
OS_EVENT *screenMutexSem;

void USART_OUT(USART_TypeDef* USARTx, uint8_t *Data,...);
char *itoa(int value, char *string, int radix);
extern void fun_para(void);

/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/




/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/
void draw_gprs_sig(u8 type);
void draw_gprs_status();
void draw_adminMenu();
void draw_guizi_door(u8 doorNum);
void moveMenu(u8 last,u8 pos);
static  void App_TaskCreate(void);

static  void App_TaskStart(void* p_arg);
static void Task_savedata(void* p_arg);
static void Task_guizi(void* p_arg);
static void Task_keyboard(void* p_arg);
static void Task_icread(void* p_arg);
static void Task_gprs(void* p_arg);
//static void Task_Screen(void* p_arg);
static void Task_barcode(void* p_arg);
void jump_modMain();
void jump_modAdminMenu();
void jump_modAdminPwdIn();
void jump_modSetGuiziID();
void jump_modOpenDoor();
void jump_modResetGPRS();
void jump_modResetBar();
void jump_modResetIC();
void delay_us(u16 time);
void delay_ms(u16 time);
u16 ic_command(u16 i,unsigned char *command,unsigned char *recv_data);
void cal_signal(u8 *recv_data);
u8 chk_cops(u8 *recv_data);
void opendoorAll();
void opendoor(u8 doornum);
void device_init(void);
void adjust_screen_light();
void sysInit(void);
u8 gprs_send(u8 *sendData,u8 *recv_data,u16 dataLen);
u8 sim900a_send_cmd(u8 *cmd,u8 *ack,unsigned char *recv_data,u16 timeout,u16 cmd_len);
u16 packMake(u8 *packData,u8 *cmd);
u8 packDecode(unsigned char *packData);

/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  OS_STK App_TaskStartStk[APP_TASK_START_STK_SIZE];

static  OS_STK Task_mainStk[Task_main_STK_SIZE];
static  OS_STK Task_calendarStk[Task_calendar_STK_SIZE];
//static  OS_STK Task_screenStk[Task_Screen_STK_SIZE];
static  OS_STK Task_savedataStk[Task_savedata_STK_SIZE];
static  OS_STK Task_guiziStk[Task_guizi_STK_SIZE];
static  OS_STK Task_keyboardStk[Task_keyboard_STK_SIZE];
static  OS_STK Task_icreadStk[Task_icread_STK_SIZE];
static  OS_STK Task_gprsStk[Task_gprs_STK_SIZE];
static  OS_STK Task_barcodeStk[Task_barcode_STK_SIZE];


/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Argument : none.
*
* Return   : none.
*********************************************************************************************************
*/

int main(void)
{
   CPU_INT08U os_err;
   //禁止CPU中断
   CPU_IntDis();
   //UCOS 初始化
   OSInit();                                                   /* Initialize "uC/OS-II, The Real-Time Kernel".         */
   //硬件平台初始化

   // 串口收发队列初始化
   QueueInit(&USART1Recieve);
   QueueInit(&USART2Recieve);
   QueueInit(&USART3Recieve);
   QueueInit(&USART1Send);
   QueueInit(&USART2Send);
   QueueInit(&USART3Send);

   BSP_Init();                                                 /* Initialize BSP functions.  */
   LCD_Init();

   device_init();

   delay_ms(1000);
    
    SPEAKER_ON();// 喇叭
    delay_ms(500);
    SPEAKER_OFF();

   //建立主任务， 优先级最高  建立这个任务另外一个用途是为了以后使用统计任务
   os_err = OSTaskCreate((void (*) (void *)) App_TaskStart,	  		  		//指向任务代码的指针
                          (void *) 0,								  		//任务开始执行时，传递给任务的参数的指针
               (OS_STK *) &App_TaskStartStk[APP_TASK_START_STK_SIZE - 1],	//分配给任务的堆栈的栈顶指针   从顶向下递减
               (INT8U) APP_TASK_START_PRIO);								//分配给任务的优先级
   
   //ucos的节拍计数器清0    节拍计数器是0-4294967295    对于节拍频率100hz时， 每隔497天就重新计数 

   OSTimeSet(0);

   //TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

   OSStart();                                                  /* Start multitasking (i.e. give control to uC/OS-II).  */
                                                 /* Start multitasking (i.e. give control to uC/OS-II).  */
 
   return (0);
}




/*
*********************************************************************************************************
*                                          App_TaskStart()
*
* Description : The startup task.  The uC/OS-II ticker should only be initialize once multitasking starts.
*
* Argument : p_arg       Argument passed to 'App_TaskStart()' by 'OSTaskCreate()'.
*
* Return   : none.
*
* Caller   : This is a task.
*
* Note     : none.
*********************************************************************************************************
*/

static  void App_TaskStart(void* p_arg)
{
   (void) p_arg;

   //初始化ucos时钟节拍
   OS_CPU_SysTickInit();                                       /* Initialize the SysTick.       */

//使能ucos 的统计任务
#if (OS_TASK_STAT_EN > 0)
   //----统计任务初始化函数  
   OSStatInit();                                               /* Determine CPU capacity.                              */
#endif

   sysInit();
   //建立其他的任务
   App_TaskCreate();
   
   while (1)
   {
        OSTimeDlyHMSM(0, 0, 1, 0);
   }
}

static void Task_main(void* p_arg)
{
    INT8U err;
    u8 enterStrPos=0;//暗码位置
    u8 menuPos=0,menuPosLast=0;//菜单位置
    unsigned char adminEnter[10];//暗码
    unsigned char doorNum[3];//柜门
    unsigned char guiziNum[9];//柜号
    u8 doorNumOpen=0;
    u8 i;
    u8 msg[2];
    u8 *msgrev;

/*
    QueueIn(&USART3Send, 'A');
    QueueIn(&USART3Send, 'T');
    QueueIn(&USART3Send, 0x0D);
    QueueIn(&USART3Send, 0x0A);
    USART_ITConfig(USART3, USART_IT_TXE, ENABLE);						//使能发送缓冲空中断  
*/

    //////计算屏幕亮度
    if((sysInfo.sysData.hour>HIGH_LIGHT_HOUR)&&(sysInfo.sysData.hour<LOW_LIGHT_HOUR))
      scrBackLight=HIGH_LIGHT_LEVEL;
    else
      scrBackLight=LOW_LIGHT_LEVEL;
    /////////////
    SetBG_Color(COLOR_BG);
    SetFG_Color(COLOR_FONT);
    ClrScreen();
    draw_gprs_status();
    draw_gprs_sig(ANY_WAY);
    SetBackLight(scrBackLight);
    jump_modMain();

    //PutString_cn((SCR_WIDTH-50*6), 0, "测试", LCD_HZK16_FONT);
    while(1)
    {
        switch(mode)
        {
            case MOD_MAIN:
            {
                //等待按键
                msgrev=OSQPend(main_OSQ,0,&err); 
                if(msgrev[0]==MAIN_KEY)//按键
                {
                    msgrev[0]=0x00;
                    switch(msgrev[1])
                    {
                        //"取"键盘被按
                        case KEY_GET:
                            
                            break;
                        case KEY_PUT:
                            barcodeShutCount=0;
                            barcode_on=1;
                            SMM_SW_ON();
                            break;
                        default:
                            //按键间隔时间是否超时
                            if(noOperateCount>=CODE_NO_OPERATE)
                            {
                                enterStrPos=0;
                            }
                            noOperateCount=0;
                            if(enterStrPos>strlen((char *)adminEnterCode))
                                enterStrPos=0;
                            adminEnter[enterStrPos]=msgrev[1];
                            enterStrPos++;
                            if(enterStrPos==strlen((char *)adminEnterCode))
                            {
                                //暗码正确，进入输入管理员密码页面
                                if(strstr(adminEnter,adminEnterCode)!=NULL){
                                    enterStrPos=0;
                                    jump_modAdminPwdIn();
                                    draw_gprs_status();
                                    draw_gprs_sig(ANY_WAY);
                                }
                            }
                            break;
                    }
                }
                break;
            }
            case MOD_ADMIN_PWD_IN:
                //等待按键
                msgrev=OSQPend(main_OSQ,100,&err); 
                if(msgrev[0]==MAIN_KEY)//页面超时
                {
                    msgrev[0]=0x00;
                    noOperateCountPage=0;//清无操作计次
                    if(enterStrPos>=ADMIN_PWD_LEN){
                        enterStrPos=0;
                        PutString(176,150,"      ", LCD_ASC16_FONT);
                    }
                    adminEnter[enterStrPos]=msgrev[1];
                    PutChar(176+enterStrPos*16, 150, '*', LCD_ASC16_FONT);
                    enterStrPos++;
                    if(enterStrPos==ADMIN_PWD_LEN)
                    {
                        if(strstr(adminEnter,sysInfo.sysData.superAdminPwd)!=NULL){
                            enterStrPos=0;
                            jump_modAdminMenu();
                            menuPos=1;menuPosLast=1;
                            moveMenu(menuPosLast, menuPos);
                            draw_gprs_status();
                            draw_gprs_sig(ANY_WAY);
                        }
						;
                    }
                }
                break;
            case MOD_ADMIN_MENU:
                //等待按键
                msgrev=OSQPend(main_OSQ,0,&err); 
                if(msgrev[0]==MAIN_KEY)//页面超时
                {
                    msgrev[0]=0x00;
                    noOperateCountPage=0;//清无操作计次
                    switch(msgrev[1])
                    {
                        case KEY_UP:
                            menuPos--;
                            if(menuPos==0)
                            {
                                menuPos=12;
                            }
                            moveMenu(menuPosLast, menuPos);
                            menuPosLast=menuPos;
                            break;
                        case KEY_DOWN:
                            menuPos++;
                            if(menuPos==13)
                            {
                                menuPos=1;
                            }
                            moveMenu(menuPosLast, menuPos);
                            menuPosLast=menuPos;
                            break;
                        case KEY_ENTER:
                            if(menuPos==4)
                            {
                                //opendoor(DOOR_ALL);
                                opendoorAll();
                            }
                            else if(menuPos==5)
                            {
                                enterStrPos=0;
                                jump_modOpenDoor();
                                draw_gprs_status();
                                draw_gprs_sig(ANY_WAY);
                            }
                            else if(menuPos==6)
                            {
                                enterStrPos=0;
                                jump_modSetGuiziID();
                                draw_gprs_status();
                                draw_gprs_sig(ANY_WAY);
                            }
                            else if(menuPos==10)//重置GPRS
                            {
                                jump_modResetGPRS();
                                menuPos=1;menuPosLast=1;
                                moveMenu(menuPosLast, menuPos);
                            }
                            else if(menuPos==11)
                            {
                                jump_modResetBar();
                                menuPos=1;menuPosLast=1;
                                moveMenu(menuPosLast, menuPos);
                            }
                            else if(menuPos==12)
                            {
                                jump_modResetIC();
                                menuPos=1;menuPosLast=1;
                                moveMenu(menuPosLast, menuPos);
                            }
                            break;
                    }
                }
                break;
            case MOD_SET_GUIZI_ID://菜单设定柜子编号
                //等待按键
                msgrev=OSQPend(main_OSQ,100,&err); 
                if(msgrev[0]==MAIN_KEY)//页面超时
                {
                    if(enterStrPos>=8)
                    {
                        if(msgrev[1]==KEY_ENTER)
                        {
                            guiziNum[8]='\0';
                            strcpy(sysInfo.sysData.guizi_id,guiziNum);
                            enterStrPos=0;
                            SST25_W_BLOCK(0, sysInfo.dataBuff, SST25_SECTOR_SIZE);//保存数据
                            gprs_status=GPRS_ST_NO_NET;//重新连接网络
                            strGprsStatus=str_con_ing;
                            jump_modAdminMenu();
                            menuPos=1;menuPosLast=1;
                            moveMenu(menuPosLast, menuPos);
                            draw_gprs_status();
                            draw_gprs_sig(ANY_WAY);
                       }
                        else if(msgrev[1]==KEY_CANCEL)//按取消键
                        {
                            enterStrPos=0;
                            jump_modAdminMenu();
                            menuPos=1;menuPosLast=1;
                            moveMenu(menuPosLast, menuPos);
                            draw_gprs_status();
                            draw_gprs_sig(ANY_WAY);
                        }
                    }
                    else if(msgrev[1]==KEY_CANCEL)//按取消键
                    {
                        enterStrPos=0;
                        jump_modAdminMenu();
                        menuPos=1;menuPosLast=1;
                        moveMenu(menuPosLast, menuPos);
                        draw_gprs_status();
                        draw_gprs_sig(ANY_WAY);
                    }
                    else if((msgrev[1]>=0x30)&&(msgrev[1]<=0x39))//>=KEY_0   <=KEY_9
                    {
                        msgrev[0]=0x00;
                        noOperateCountPage=0;//清无操作计次
                        guiziNum[enterStrPos]=msgrev[1];
                        SetFG_Color(LCD_BLUE);
                        PutChar(176+enterStrPos*16, 180, guiziNum[enterStrPos], LCD_ASC16_FONT);
                        SetFG_Color(COLOR_FONT);
                        enterStrPos++;
                        if(enterStrPos>=8)
                        {
                            SetFG_Color(LCD_BLUE);
                            PutString_cn(128, 220, "请按确认或取消", LCD_HZK32_FONT);
                            SetFG_Color(COLOR_FONT);
                        }
                    }
                }
                break;
            case MOD_OPEN_DOOR://打开指令柜门
            {
                //等待按键
                msgrev=OSQPend(main_OSQ,100,&err); 
                if(msgrev[0]==MAIN_KEY)//页面超时
                {
                    if(msgrev[1]==KEY_CANCEL)//按取消键
                    {
                        enterStrPos=0;
                        jump_modAdminMenu();
                        menuPos=1;menuPosLast=1;
                        moveMenu(menuPosLast, menuPos);
                        draw_gprs_status();
                        draw_gprs_sig(ANY_WAY);
                    }
                    else if((msgrev[1]>=0x30)&&(msgrev[1]<=0x39))//>=KEY_0   <=KEY_9
                    {
                        msgrev[0]=0x00;
                        noOperateCountPage=0;//清无操作计次
                        doorNum[enterStrPos]=msgrev[1];
                        SetFG_Color(LCD_BLUE);
                        PutChar(224+enterStrPos*16, 180, doorNum[enterStrPos], LCD_ASC16_FONT);
                        SetFG_Color(COLOR_FONT);
                        enterStrPos++;
                        if(enterStrPos>=2)
                        {
                            SetFG_Color(LCD_BLUE);
                            PutString_cn(128, 220, "请按确认或取消", LCD_HZK32_FONT);
                            SetFG_Color(COLOR_FONT);
                        }
                    }
                    else if(enterStrPos>=2)
                    {
                        if(msgrev[1]==KEY_ENTER)
                        {
                            doorNumOpen=((doorNum[0]-0x30)*10)+(doorNum[1]-0x30);
                            if((doorNumOpen>sysInfo.sysData.door_num)||(doorNumOpen<=0))
                            {
                                PutString_cn(120, 260, "柜门不存在，请按取消", LCD_HZK24_FONT);
                            }
                            else
                            {
                                opendoor(doorNumOpen);
                                /******开完门以后返回主菜单*******/
                                enterStrPos=0;
                                jump_modAdminMenu();
                                menuPos=1;menuPosLast=1;
                                moveMenu(menuPosLast, menuPos);
                                draw_gprs_status();
                                draw_gprs_sig(ANY_WAY);
                                /*******************************************/
                            }
                        }
                        else if(msgrev[1]==KEY_CANCEL)//按取消键
                        {
                            enterStrPos=0;
                            jump_modAdminMenu();
                            menuPos=1;menuPosLast=1;
                            moveMenu(menuPosLast, menuPos);
                            draw_gprs_status();
                            draw_gprs_sig(ANY_WAY);
                        }
                    }
                }
            }
            default:
                break;
        }
        //OSTimeDlyHMSM(0, 0, 0, 500);
    }
}
static void Task_calendar(void* p_arg)
{
    INT8U err;
   u8 msg[2]={DIS_TIME, 0x00};
   while (1)
   {
        sysInfo.sysData.second++;
        //时间管理维护
        if  (sysInfo.sysData.second > 59)
        {
            sysInfo.sysData.second = 0;
            sysInfo.sysData.minute++;
        }
        if(sysInfo.sysData.minute > 59)
        {
            sysInfo.sysData.minute = 0;
            sysInfo.sysData.hour++;
        }
        if(sysInfo.sysData.hour > 23)
        {
            sysInfo.sysData.hour = 0;
            sysInfo.sysData.day++;
        }
        //日
        if((sysInfo.sysData.month == 1) | (sysInfo.sysData.month == 3) | (sysInfo.sysData.month == 5) | (sysInfo.sysData.month == 7) | (sysInfo.sysData.month == 8) | (sysInfo.sysData.month == 10) | (sysInfo.sysData.month == 12)) //大月
        {
            if(sysInfo.sysData.day > 31)
            {
                sysInfo.sysData.day = 0;
                sysInfo.sysData.month++;
            }
        }
        else if(sysInfo.sysData.month == 2) //二月
        {
            if(sysInfo.sysData.day > 28)
            {
                sysInfo.sysData.day = 0;
                sysInfo.sysData.month++;
            }
        }
        else//小月
        {
            if(sysInfo.sysData.day > 30)
            {
                sysInfo.sysData.day = 0;
                sysInfo.sysData.month++;
            }
        }
        //月
        if(sysInfo.sysData.month > 12)
        {
            sysInfo.sysData.month = 0;
            sysInfo.sysData.year++;
        }
        OSTimeDlyHMSM(0, 0, 0, 500);
        sprintf(time_data, "%d-%02d-%02d %02d %02d %02d", sysInfo.sysData.year, sysInfo.sysData.month, sysInfo.sysData.day, sysInfo.sysData.hour, sysInfo.sysData.minute, sysInfo.sysData.second);
        /***display**/
        OSMutexPend(screenMutexSem,0,&err);
        PutString(16, 0, time_data, LCD_ASC8_FONT);
        OSMutexPost(screenMutexSem);
        /*****display end****/
        OSTimeDlyHMSM(0, 0, 0, 500);
        sprintf(time_data, "%d-%02d-%02d %02d:%02d:%02d", sysInfo.sysData.year, sysInfo.sysData.month, sysInfo.sysData.day, sysInfo.sysData.hour, sysInfo.sysData.minute, sysInfo.sysData.second);
        /***display***********/
        OSMutexPend(screenMutexSem,0,&err);
        PutString(16, 0, time_data, LCD_ASC8_FONT);
        OSMutexPost(screenMutexSem);
        /*****display end****/

        //判断时间决定是否切换屏幕亮度
        if(((sysInfo.sysData.hour==HIGH_LIGHT_HOUR)||(sysInfo.sysData.hour==LOW_LIGHT_HOUR))&&(sysInfo.sysData.minute==0)&&(sysInfo.sysData.second==0))
        {
            adjust_screen_light();
        }

        //维护无操作计次
        if(mode==MOD_MAIN)
        {
            noOperateCount++;
            if(barcode_on)
            {
                barcodeShutCount++;
                if(barcodeShutCount>=BARCODE_NO_OPERATE)//条形码无动作
                {
                    SMM_SW_OFF();
                    barcode_on=0;
                }
            }
        }
        else
        {
            noOperateCountPage++;
            if(noOperateCountPage>=PAGE_NO_OPERATE)//二级页面倒计时到
            {
                jump_modMain();
                draw_gprs_status();
                draw_gprs_sig(ANY_WAY);
            }
        }
   }
}


/*
// 显示屏任务UI任务
static void Task_Screen(void* p_arg)
{
   INT8U err;
   unsigned char str[]="this is a test";
   unsigned char * msg; 
   unsigned char * str_gprs; 
  int i=0;
  int test=0;
  (void)p_arg;  

  while(1) 
  { 
        msg=OSQPend(Screen_OSQ,0,&err); 
        switch(msg[0])
        {
            case DIS_MAIN:
                //////计算屏幕亮度
                if((sysInfo.sysData.hour>HIGH_LIGHT_HOUR)&&(sysInfo.sysData.hour<LOW_LIGHT_HOUR))
                  scrBackLight=0x09;
                else
                  scrBackLight=0x01;
                /////////////
                SetBG_Color(COLOR_BG);
                SetFG_Color(COLOR_FONT);
                SetBackLight(scrBackLight);
                ClrScreen();
                PutBitmap(0,16,BG);
                break;
            case DIS_ADMIN_PWD:
                ClrScreen();
                break;
            case DIS_BKLIGHT:
                SetBackLight(scrBackLight);
                break;
            case DIS_TIME:
                for(i=0;i<strlen((char *)time_data);i++)// 显示时间
    		    PutChar(16+i*8,0,time_data[i],LCD_ASC8_FONT);
                break;
            case DIS_CARD:
                sprintf(str, "card:%02X %02X %02X %02X",msg[1],msg[2],msg[3] ,msg[4]);
                for(i=0;i<strlen((char *)str);i++)
    		    PutChar(0+i*8,80,str[i],LCD_ASC8_FONT);
                break;
            case DIS_GPRS_SIG:
                break;
            case DIS_GPRS_STATUS:
                switch(msg[1])
                {
                    case GPRS_ST_NO_MODEL:
                        str_gprs=str_test_gprs;
                        break;
                    case GPRS_ST_NO_SIMCARD:
                        str_gprs=str_no_card;
                        break;
                    case GPRS_ST_NO_NET:
                        str_gprs=str_con_ing;
                        break;
                    case GPRS_CMCC:
                        str_gprs=str_CMCC;
                        break;
                    case GPRS_UNION:
                        str_gprs=str_unicom;
                        break;
                }
                for(i=0;i<strlen((char *)str_gprs)-1;i+=2)
    		    PutChar_cn((SCR_WIDTH-16*6)+(i/2)*16,0,&str_gprs[i],LCD_HZK16_FONT);
                break;
            case 0x98:// 柜门读取测试
                sprintf(str, "door:%02X  %02X  %02X  ",msg[1],msg[2],msg[3] );
                for(i=0;i<strlen((char *)str);i++)
    		    PutChar(0+i*8,40,str[i],LCD_ASC8_FONT);
                break;
            case 0x99:// Key 测试
                sprintf(str, "Key:%02X ",msg[1]);
                for(i=0;i<strlen((char *)str);i++)
    		    PutChar(0+i*8,50,str[i],LCD_ASC8_FONT);
                break;
            case 0xFF:
                sprintf(str, "GPRS:%02X %02X %02X %02X %02X %02X %02X",msg[1],msg[2],msg[3] ,msg[4],msg[5],msg[6],msg[7]);
                for(i=0;i<strlen((char *)str);i++)
                PutChar(50+i*8,60,str[i],LCD_ASC8_FONT);
                test++;
                break;
            default:
                for(i=0;i<strlen((char *)msg);i++)
    		    PutChar(0+i*8,60,msg[i],LCD_ASC8_FONT);
                break;
        }
  }
}
*/

////////////定时数据保存任务
static void Task_savedata(void* p_arg)      
{
    (void)p_arg; 
    while(1)
    {
        SST25_W_BLOCK(0, sysInfo.dataBuff, SST25_SECTOR_SIZE);
        OSTimeDlyHMSM(0,0,20,0); // 延时20s
    }
}

////////////柜子状态刷新及操作控制任务
static void Task_guizi(void* p_arg)
{
    unsigned char msg[10];
    u8 openstatus_01=0xFF;
    u8 openstatus_02=0xFF;
    u8 openstatus_03=0xFF;
    u8 i=0;
   unsigned char str[]="this is a test";
  (void)p_arg;  
  while(1)
  {
    // 读取柜子状态
    IRcvStr(PCA9555_01W,0x01,&openstatus_01,1);
    IRcvStr(PCA9555_02W,0x01,&openstatus_02,1);
    IRcvStr(PCA9555_03W,0x01,&openstatus_03,1);
    for(i=0;i<8;i++)
    {
        if(sysInfo.sysData.guizi[i].doorstatus!=((openstatus_01&(0x1<<i))>>i))
        {
            sysInfo.sysData.guizi[i].doorstatus=((openstatus_01&(0x1<<i))>>i);//为1表示门开着  !待确定
            if(mode==MOD_MAIN)
                draw_guizi_door(i);
        }
    }
    for(i=0;i<8;i++)
    {
        if(sysInfo.sysData.guizi[8+i].doorstatus!=((openstatus_02&(0x1<<i))>>i))
        {
            sysInfo.sysData.guizi[8+i].doorstatus=((openstatus_02&(0x1<<i))>>i);//为1表示门开着  !待确定
            if(mode==MOD_MAIN)
                draw_guizi_door(8+i);
        }
    }
    for(i=0;i<8;i++)
    {
        if(sysInfo.sysData.guizi[16+i].doorstatus!=((openstatus_03&(0x1<<i))>>i))
        {
            sysInfo.sysData.guizi[16+i].doorstatus=((openstatus_03&(0x1<<i))>>i);//为1表示门开着  !待确定
            if(mode==MOD_MAIN)
                draw_guizi_door(16+i);
        }
    }
    /*
    if(mode==MOD_MAIN)
    {
        // 显示门未关的柜子
        for(i=0;i<sysInfo.sysData.door_num;i++)
        {
            if(sysInfo.sysData.guizi[i].doorstatus==0){
                SetFG_Color(COLOR_SELECTED);
                OSTimeDlyHMSM(0,0,0,5); 
                draw_guizi_door(i);
                SetFG_Color(COLOR_FONT);
            }
        }
    }
    */
    OSTimeDlyHMSM(0,0,0,500); 
  }
    
}

////////////键盘扫描任务
static void Task_keyboard(void* p_arg)
{
    unsigned char msg[2];
    int keyLast=-1;
    int KeyValue=-1;
    (void)p_arg;  
    while(1)
    {
        keyLast=KeyValue;
        KeyValue=keyScan();
        if(KeyValue!=-1)
        {
            if(KeyValue!=keyLast)//判断是否未松开
            {
                msg[0]=MAIN_KEY;
                msg[1]=KeyValue;
                OSQPost(main_OSQ,(void *)&msg);
                //OSMboxPost(keyEventMBOX,(void *)&msg); 
            }
        }
        OSTimeDlyHMSM(0,0,0,100); 
        KB_A1_OFF();
        KB_A2_OFF();
        KB_A3_OFF();
        KB_A4_OFF();
    }
}

/////////////GPRS
static void Task_gprs(void* p_arg)
{
    INT8U err;
    u8 msg[8];
    u8 start=0;// 检测到命令字的位置
    u8 strength=0;
    u8 *strtmp;
    u8 cops=0;
    unsigned char recv_data[128];
    unsigned char sendPack[128];
    u16 sendPackLen=0;
    unsigned char cmd[32];

    int recvPackLen=0,i;


    ////初始化
    strGprsStatus=str_test_gprs;
    draw_gprs_status();
    gprs_strength=0;
    draw_gprs_sig(ANY_WAY);
    OSTimeDlyHMSM(0,0,2,0); 
    OSMboxPend(gprs_rev_MBOX, 1000, &err);
    if(USART3Recieve.bufferCount!=0)
    {
        recvPackLen=USART3Recieve.bufferCount;
        for(i=0;i<recvPackLen;i++)
        {
            QueueOut(&USART3Recieve, &recv_data[i]);
        }
    }
    
    while(1)
    {
        switch(gprs_status)
        {
            case GPRS_ST_NO_MODEL:// 检测模块
                if(sim900a_send_cmd("AT","OK",recv_data,1000,0)==0)
                {
                    ////////////否则重启GPRS模块
                    GPRS_SW_OFF();
                    OSTimeDlyHMSM(0,0,1,0); 
                    GPRS_SW_ON();
                    OSTimeDlyHMSM(0,0,4,0); 
                    //////////////
                    break;
                }
                gprs_status=GPRS_ST_NO_SIMCARD;
                break;
            case GPRS_ST_NO_SIMCARD:// 检测SIM卡
                gprs_strength=0;
                if(sim900a_send_cmd("ATE0","OK",recv_data,1000,0)==0)break;// 关闭回显
                start=sim900a_send_cmd("AT+CPIN?","READY",recv_data,1000,0);// 查询SIM卡
                if(start!=0)
                {
                    gprs_status=GPRS_ST_NO_NET;// 检测网络
                    strGprsStatus=str_con_ing;
                    draw_gprs_status();
                }
                else
                {
                    strGprsStatus=str_no_card;
                    draw_gprs_status();
                }
                OSTimeDlyHMSM(0,0,1,0); 
                break;
            case GPRS_ST_NO_NET:
                gprs_strength=0;
                cal_signal(recv_data);// 计算信号强度
                OSTimeDlyHMSM(0,0,1,0); 
                start=sim900a_send_cmd("AT+CIPSTATUS","OK",recv_data,1000,0);//查询当前连接状态
                if(start!=0)
                {
                    OSTimeDlyHMSM(0,0,1,0); 
                    /******************************/
                    cops=chk_cops(recv_data);//查询运营商
                    if(cops==1)
                    {
                        gprs_cops=GPRS_CMCC;
                        strGprsStatus=str_CMCC;
                        draw_gprs_status();
                    }
                    else if(cops==2)
                    {
                        gprs_cops=GPRS_UNION;
                        strGprsStatus=str_unicom;
                        draw_gprs_status();
                    }
                    else{
                        break;
                    }
                    /******************************/
                    if(sim900a_send_cmd("AT+CIPSHUT","OK",recv_data,1000,0)==0)break;//
                    if(sim900a_send_cmd("AT+CGCLASS=\"B\"","OK",recv_data,1000,0)==0)break;
                    if(gprs_cops==GPRS_CMCC)
                    {
                        if(sim900a_send_cmd("AT+CGDCONT=1,\"IP\",\"CMNET\"","OK",recv_data,1000,0)==0)break;//
                    }
                    else if(gprs_cops==GPRS_UNION)
                    {
                        if(sim900a_send_cmd("AT+CGDCONT=1,\"IP\",\"3GNET\"","OK",recv_data,1000,0)==0)break;//
                    }
                    else
                    {
                        break;
                    }
                    if(sim900a_send_cmd("AT+CGATT=1","OK",recv_data,1000,0)==0)break;//
                    if(sim900a_send_cmd("AT+CIPHEAD=1","OK",recv_data,1000,0)==0)break;//
                    sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",\"%d\"",sysInfo.sysData.servIP,sysInfo.sysData.port);
                    if(sim900a_send_cmd(cmd,"OK",recv_data,1000,0)==0)break;//
                    if(sim900a_send_cmd(0,"CONNECT OK",recv_data,2000,0)==0)break;//
                    gprs_status=GPRS_ST_CONNECTED;
                    /*发送开机指令包*/
                    sendPackLen=packMake(sendPack,"01");
                    gprs_send(sendPack, recv_data,sendPackLen);
                    /******************************/
                }
                //OSTimeDlyHMSM(0,0,1,0); 
                break;
            case GPRS_ST_CON_FAIL://连接丢失
                if(sim900a_send_cmd("AT+CIPSHUT","OK",recv_data,1000,0)==0)break;//
                sprintf(cmd, "AT+CIPSTART=\"TCP\",\"%s\",\"%d\"",sysInfo.sysData.servIP,sysInfo.sysData.port);
                if(sim900a_send_cmd(cmd,"OK",recv_data,1000,0)==0)break;//
                if(sim900a_send_cmd(0,"CONNECT OK",recv_data,2000,0)==0)break;//
                gprs_status=GPRS_ST_CONNECTED;
                break;
            case GPRS_ST_CONNECTED:
                //cal_signal(recv_data);// 计算信号强度
                // 等待串口数据
                OSMboxPend(gprs_rev_MBOX, 0, &err);
                if(USART3Recieve.bufferCount!=0)
                {
                    recvPackLen=USART3Recieve.bufferCount;
                    for(i=0;i<recvPackLen;i++)
                    {
                        QueueOut(&USART3Recieve, &recv_data[i]);
                    }
                }
                //读取TCP指令
                for(i=0;i<recvPackLen;i++)
                {
                    if((recv_data[i]==0x59)&&(recv_data[i+1]==0x47))
                    {
                        packDecode(&recv_data[i]);//解析指令
                    }
                }
                //OSTimeDlyHMSM(0,0,5,0); // 后期去掉
                break;
        }
    }
}
//接收条码任务
static void Task_barcode(void* p_arg)
{
   INT8U err;
   int len_usart=0,i;
   unsigned char recv_data[24];
    (void)p_arg;  
    OSTimeDlyHMSM(0,0,2,0);
    USART_Cmd(USART1, ENABLE);	
    while(1)
    {
        OSMboxPend(barcode_rev_MBOX, 0, &err);
        if(USART1Recieve.bufferCount!=0)
        {
            barcodeShutCount=0;// 条形码关闭计次重置
            len_usart=USART1Recieve.bufferCount;
            for(i=0;i<len_usart;i++)
            {
                QueueOut(&USART1Recieve, &recv_data[i]);
            }
            recv_data[len_usart-1]='\0';
        }
        PutString(60, 300, recv_data, LCD_ASC8_FONT);
    }
}
/////////////IC读卡(已测试)
static void Task_icread(void* p_arg)
{
   INT8U err;
   int len_usart=0;
   u8 i=0,j=0;
   unsigned char str[]="this is a test";
   u8 card[]={0x00,0x00,0x00,0x00};
   unsigned char recv_data[20];
    char request[]={0x07,0x12,0x41,0x01,0x52,0xf8,0x03};// 请求   len 7
    char crash[]={0x0c, 0x22, 0x42, 0x06, 0x93, 0x00, 0x78, 0x01, 0xa6, 0x00, 0xd9, 0x03};// 防碰撞  len 12
    char stop[]={0x06, 0x32, 0x44, 0x00, 0x8f, 0x03};// 结束   len 6
    (void)p_arg;  
    while(1)
    {
        // 发送请求
        len_usart=ic_command(7,request,recv_data);
        // 寻找包头
        j=0;
        while((j<len_usart)&&(recv_data[j]!=0x06)&&(recv_data[j]!=0x08)&&(recv_data[j]!=0x0A))
            j++;
        if(recv_data[j]==0x06)
        {
            OSTimeDlyHMSM(0,0,0,100); 
            // 发送结束
            len_usart=ic_command(6,stop,recv_data);
        }
        else if(recv_data[j]==0x08)
        {
            OSTimeDlyHMSM(0,0,0,100); 
            //发送防碰撞
            len_usart=ic_command(12,crash,recv_data);
            j=0;
            while((j<len_usart)&&(recv_data[j]!=0x06)&&(recv_data[j]!=0x08)&&(recv_data[j]!=0x0A))
                j++;
            if(recv_data[j]==0x0A)// 寻找到卡号
            {
                SPEAKER_ON();
                for(i=0;i<4;i++)
                {
                    card[i]=recv_data[i+4+j];
                }
                // 处理卡号
                sprintf(str, "card:%02X  %02X  %02X %02X ",card[0],card[1],card[2],card[3] );
                for(i=0;i<strlen((char *)str);i++)
    		    PutChar(90+i*8,80,str[i],LCD_ASC8_FONT);
                /////////////////////////////
                
                OSTimeDlyHMSM(0,0,0,100); 
                SPEAKER_OFF();
                //////////////循环查询至卡离开
                while(recv_data[j]==0x0A)
                {
                    OSTimeDlyHMSM(0,0,0,100); 
                    //发送防碰撞
                    len_usart=ic_command(12,crash,recv_data);
                    j=0;
                    while((j<len_usart)&&(recv_data[j]!=0x06)&&(recv_data[j]!=0x08)&&(recv_data[j]!=0x0A))
                        j++;
                }
                //////////////////////////////////////
            }
            // 发送结束
            len_usart=ic_command(6,stop,recv_data);
        }
        OSTimeDlyHMSM(0,0,0,100); 
    }
}

/*
*********************************************************************************************************
*                                            App_TaskCreate()
*
* Description : Create the application tasks.
*
* Argument : none.
*
* Return   : none.
*
* Caller   : App_TaskStart().
*
* Note     : none.
*********************************************************************************************************
*/

static  void App_TaskCreate(void)
{
   INT8U err;
   screenMutexSem=OSMutexCreate(1,&err);
   barcode_rev_MBOX=OSMboxCreate((void *) 0);
   iccard_rev_MBOX=OSMboxCreate((void *) 0);
   gprs_rev_MBOX=OSMboxCreate((void *) 0);
   keyEventMBOX=OSMboxCreate((void *) 0);
   Screen_OSQ=OSQCreate(&MsgGrp[0],N_MESSAGES);                //创建消息队列    //建立刷屏消息邮箱
   main_OSQ=OSQCreate(&MsgMainGrp[0],N_MESSAGES);

// 创建日历管理任务
   OSTaskCreateExt(Task_calendar,(void *)0,(OS_STK *)&Task_calendarStk[Task_calendar_STK_SIZE-1],Task_calendar_PRIO,Task_calendar_PRIO,(OS_STK *)&Task_calendarStk[0],
                    Task_calendar_STK_SIZE,
                    (void *)0,
                    OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR);
/*
// 创建主显示任务
   OSTaskCreateExt(Task_Screen,(void *)0,(OS_STK *)&Task_screenStk[Task_Screen_STK_SIZE-1],Task_screen_PRIO,Task_screen_PRIO,(OS_STK *)&Task_screenStk[0],
                    Task_Screen_STK_SIZE,
                    (void *)0,
                    OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR);
*/
// 创建定时存盘任务
   OSTaskCreateExt(Task_savedata,(void *)0,(OS_STK *)&Task_savedataStk[Task_savedata_STK_SIZE-1],Task_savedata_PRIO,Task_savedata_PRIO,(OS_STK *)&Task_savedataStk[0],
                    Task_savedata_STK_SIZE,
                    (void *)0,
                    OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR);

// 创建柜子管理任务
   OSTaskCreateExt(Task_guizi,(void *)0,(OS_STK *)&Task_guiziStk[Task_guizi_STK_SIZE-1],Task_guizi_PRIO,Task_guizi_PRIO,(OS_STK *)&Task_guiziStk[0],
                    Task_guizi_STK_SIZE,
                    (void *)0,
                    OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR);
                    
// 创建键盘扫描任务
   OSTaskCreateExt(Task_keyboard,(void *)0,(OS_STK *)&Task_keyboardStk[Task_keyboard_STK_SIZE-1],Task_keyboard_PRIO,Task_keyboard_PRIO,(OS_STK *)&Task_keyboardStk[0],
                    Task_keyboard_STK_SIZE,
                    (void *)0,
                    OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR);
// 创建IC读卡任务
   OSTaskCreateExt(Task_icread,(void *)0,(OS_STK *)&Task_icreadStk[Task_icread_STK_SIZE-1],Task_icread_PRIO,Task_icread_PRIO,(OS_STK *)&Task_icreadStk[0],
                    Task_icread_STK_SIZE,
                    (void *)0,
                    OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR);
// 创建GPRS任务
   OSTaskCreateExt(Task_gprs,(void *)0,(OS_STK *)&Task_gprsStk[Task_gprs_STK_SIZE-1],Task_gprs_PRIO,Task_gprs_PRIO,(OS_STK *)&Task_gprsStk[0],
                    Task_gprs_STK_SIZE,
                    (void *)0,
                    OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR);
// 创建条码任务
   OSTaskCreateExt(Task_barcode,(void *)0,(OS_STK *)&Task_barcodeStk[Task_barcode_STK_SIZE-1],Task_barcode_PRIO,Task_barcode_PRIO,(OS_STK *)&Task_barcodeStk[0],
                    Task_barcode_STK_SIZE,
                    (void *)0,
                    OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR);
// 创建主任务
   OSTaskCreateExt(Task_main,(void *)0,(OS_STK *)&Task_mainStk[Task_main_STK_SIZE-1],Task_main_PRIO,Task_main_PRIO,(OS_STK *)&Task_mainStk[0],
                    Task_main_STK_SIZE,
                    (void *)0,
                    OS_TASK_OPT_STK_CHK|OS_TASK_OPT_STK_CLR);

    //OSMutexPost(screenMutexSem);
}
/**显示GPRS状态**/
void draw_gprs_status()
{
    INT8U err;
    OSMutexPend(screenMutexSem,0,&err);
    if(gprs_status!=GPRS_ST_CONNECTED)//未连接到服务器，设置文字灰色
        SetFG_Color(LCD_DARK);
    PutString_cn((SCR_WIDTH-16*6), 0, strGprsStatus, LCD_HZK16_FONT);
    SetFG_Color(COLOR_FONT);
    OSMutexPost(screenMutexSem);
}
/**画GPRS信号**/
void draw_gprs_sig(u8 type)
{
    // 显示信号质量
    if((gprs_strength_dis!=gprs_strength)||type==ANY_WAY)// 避免刷新过多
    {
        switch(gprs_strength)
        {
            case 0:
                PutBitmap((SCR_WIDTH-32),0,BAR_0);
                break;
            case 1:
                PutBitmap((SCR_WIDTH-32),0,BAR_1);
                break;
            case 2:
                PutBitmap((SCR_WIDTH-32),0,BAR_2);
                break;
            case 3:
                PutBitmap((SCR_WIDTH-32),0,BAR_3);
                break;
            case 4:
                PutBitmap((SCR_WIDTH-32),0,BAR_4);
                break;
            case 5:
                PutBitmap((SCR_WIDTH-32),0,BAR_5);
                break;
        }
        gprs_strength_dis=gprs_strength;
    }
}

void draw_guizi_door(u8 doorNum)
{
    u8 d=33;//柜门图标横间距
    u8 h=35;//柜门图标竖间距
    int x0=20;
    int y0=92;
    int x=30;
    int y=60;
    int r=doorNum/6;//row
    int v=doorNum%6;//

    if(v<3){
        x=x0+d*v;
        y=y0+h*r;
    }
    else{
        x=SCR_WIDTH-x0-d-(5-v)*d;
        y=y0+h*r;
    }
    if(sysInfo.sysData.guizi[doorNum].doorstatus==1)
    {
        if(sysInfo.sysData.guizi[doorNum].status==0)
            PutBitmap(x,y,EMPTY);
            //PutChar_cn(, y, "空", LCD_HZK16_FONT);
        else if(sysInfo.sysData.guizi[doorNum].status==1)
            PutBitmap(x,y,FULL);
            //PutChar_cn(x, y, "满", LCD_HZK16_FONT);
        else if(sysInfo.sysData.guizi[doorNum].status==2)
            PutBitmap(x,y,LOCK);
    }
    else
        PutBitmap(x,y,OPEN);
}
/*******画管理员菜单********/
void draw_adminMenu()
{
    u8 y=50;
    u8 span=30;
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString(MENU_COLUMN_1, y, "1.", LCD_ASC12_FONT);
        //OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString_cn(MENU_COLUMN_1+24, y, "条码存包裹", LCD_HZK24_FONT);
    y+=span;
    /****************/
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString(MENU_COLUMN_1, y, "2.", LCD_ASC12_FONT);
        //OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString_cn(MENU_COLUMN_1+24, y, "管理员存包裹", LCD_HZK24_FONT);
    y+=span;
    /****************/
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString(MENU_COLUMN_1, y, "3.", LCD_ASC12_FONT);
        //OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString_cn(MENU_COLUMN_1+24, y, "管理员取包裹", LCD_HZK24_FONT);
    y+=span;
    /****************/
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString(MENU_COLUMN_1, y, "4.", LCD_ASC12_FONT);
        //OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString_cn(MENU_COLUMN_1+24, y, "打开所有柜门", LCD_HZK24_FONT);
    y+=span;
    /****************/
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString(MENU_COLUMN_1, y, "5.", LCD_ASC12_FONT);
        //OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString_cn(MENU_COLUMN_1+24, y, "打开指定柜门", LCD_HZK24_FONT);
    y+=span;
    /****************/
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString(MENU_COLUMN_1, y, "6.", LCD_ASC12_FONT);
        //OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString_cn(MENU_COLUMN_1+24, y, "设定柜子编号", LCD_HZK24_FONT);
    y=50;
    /****************/
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString(MENU_COLUMN_2, y, "7.", LCD_ASC12_FONT);
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString_cn(MENU_COLUMN_2+24, y, "设定超管密码", LCD_HZK24_FONT);
    y+=span;
    /****************/
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString(MENU_COLUMN_2, y, "8.", LCD_ASC12_FONT);
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString_cn(MENU_COLUMN_2+24, y, "设定管理员密码", LCD_HZK24_FONT);
   y+=span;
    /****************/
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString(MENU_COLUMN_2, y, "9.", LCD_ASC12_FONT);
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString_cn(MENU_COLUMN_2+24, y, "设定服务器参数", LCD_HZK24_FONT);
    y+=span;
    /****************/
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString(MENU_COLUMN_2, y, "10.", LCD_ASC12_FONT);
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString_cn(MENU_COLUMN_2+36, y, "重启", LCD_HZK24_FONT);
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString(MENU_COLUMN_2+84, y, "GPRS", LCD_ASC12_FONT);
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString_cn(MENU_COLUMN_2+132, y, "模块", LCD_HZK24_FONT);
    y+=span;
    /****************/
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString(MENU_COLUMN_2, y, "11.", LCD_ASC12_FONT);
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString_cn(MENU_COLUMN_2+36, y, "重启条形码模块", LCD_HZK24_FONT);
    y+=span;
    /****************/
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString(MENU_COLUMN_2, y, "12.", LCD_ASC12_FONT);
        OSTimeDlyHMSM(0,0,0,DRAW_SPAN_B);
        PutString_cn(MENU_COLUMN_2+36, y, "重启刷卡模块", LCD_HZK24_FONT);
    y+=span;
}

////////////////////
//随机显示背景图
void draw_main_BG()
{
    u8 i=sysInfo.sysData.second%6;
    switch(i)
    {
        case 0:
            PutBitmap(0,16,BG01);
            break;
        case 1:
            PutBitmap(0,16,BG02);
            break;
        case 2:
            PutBitmap(0,16,BG03);
            break;
        case 3:
            PutBitmap(0,16,BG04);
            break;
        case 4:
            PutBitmap(0,16,BG05);
            break;
        case 5:
            PutBitmap(0,16,BG06);
            break;
        default:
            PutBitmap(0,16,BG01);
            break;
    }
}
void moveMenu(u8 last,u8 pos)
{
    u8 x=0,y=20;
    u8 span=30;
    OSTimeDlyHMSM(0,0,0,5);
    if(last>6)PutChar((SCR_WIDTH/2+12), y+(span)*(last-6), ' ', LCD_ASC12_FONT);
    else PutChar(12, y+(span)*last, ' ', LCD_ASC12_FONT);
    OSTimeDlyHMSM(0,0,0,5);
    if(pos>6)PutChar((SCR_WIDTH/2+12), y+(span)*(pos-6), '>', LCD_ASC12_FONT);
    else PutChar(12, y+(span)*pos, '>', LCD_ASC12_FONT);
}
void jump_modMain()
{
    INT8U err;
    u8 msg[2]; 
    u8 i;
    mode=MOD_MAIN;
    noOperateCount=0;
    
    ClrScreen();
    draw_main_BG();
    //PutBitmap(0,16,BG01);
    OSTimeDlyHMSM(0,0,2,0); 
    //画柜门
    /***display***********/
    OSMutexPend(screenMutexSem,0,&err);
    for(i=0;i<sysInfo.sysData.door_num;i++)
    {
        draw_guizi_door(i);
    }


        //test////////////////////////////
    //test////////////////////////////
    //test////////////////////////////
    //PutString(16, 30, "test", LCD_ASC8_FONT);



    OSMutexPost(screenMutexSem);
    /*****display end****/
}
void jump_modAdminPwdIn()
{
    mode=MOD_ADMIN_PWD_IN;
    noOperateCountPage=0;//二级页面倒计时清零
    //刷新显示
    ClrScreen();
    PutString_cn(144, 100, "请输入管理员密码", LCD_HZK24_FONT);
}
/**********管理员界面***********/
void jump_modAdminMenu()
{
    INT8U err;
    mode=MOD_ADMIN_MENU;
    noOperateCountPage=0;
    /***display***********/
    OSMutexPend(screenMutexSem,0,&err);
    ClrScreen();
    draw_adminMenu();
    OSMutexPost(screenMutexSem);
    /*****display end****/
}

void jump_modSetServIP()
{
    INT8U err;
    mode=MOD_SET_IP;
    noOperateCountPage=0;
    /***display***********/
    OSMutexPend(screenMutexSem,0,&err);
    PutString_cn(156, 50, "请输入",LCD_HZK24_FONT);
    PutString(156+24*3, 50, "IP", LCD_ASC12_FONT);
    OSMutexPost(screenMutexSem);
    /*****display end****/
}

void jump_modSetServPort()
{
    mode=MOD_SET_PORT;
    noOperateCountPage=0;
    PutString_cn(180, 50, "请输入端口",LCD_HZK24_FONT);
}
void jump_modSetGuiziID()
{
    mode=MOD_SET_GUIZI_ID;
    noOperateCountPage=0;
    ClrScreen();
    OSTimeDlyHMSM(0,0,0,10); 
    PutString_cn(168, 60, "当前柜子编号",LCD_HZK24_FONT);
    OSTimeDlyHMSM(0,0,0,10); 
    PutString(192, 90, sysInfo.sysData.guizi_id,LCD_ASC12_FONT);
    OSTimeDlyHMSM(0,0,0,10); 
    PutString_cn(96, 120, "重设柜号后将重新连接网络",LCD_HZK24_FONT);
    OSTimeDlyHMSM(0,0,0,10); 
    PutString_cn(156, 150, "请输入柜子编号",LCD_HZK24_FONT);
}
void jump_modOpenDoor()
{
    mode=MOD_OPEN_DOOR;
    noOperateCountPage=0;
    ClrScreen();
    OSTimeDlyHMSM(0,0,0,10); 
    PutString_cn(120, 60, "请输入需要打开的柜门",LCD_HZK24_FONT);
    OSTimeDlyHMSM(0,0,0,10); 
    PutString_cn(84, 90, "柜门为两位数，个位数请补零",LCD_HZK24_FONT);
}
void jump_modResetGPRS()
{
    noOperateCountPage=0;
    ClrScreen();
    OSTimeDlyHMSM(0,0,0,10); 
    PutString(10, 50, "GPRS", LCD_ASC12_FONT);
    OSTimeDlyHMSM(0,0,0,10); 
    PutString_cn(58, 50, "重启中",LCD_HZK24_FONT);
    GPRS_SW_OFF();
    SPEAKER_ON();
    OSTimeDlyHMSM(0,0,0,500); 
    SPEAKER_OFF();
    GPRS_SW_ON();
    OSTimeDlyHMSM(0,0,0,500); 
    ClrScreen();
    OSTimeDlyHMSM(0,0,0,10); 
    PutString(10, 50, "GPRS", LCD_ASC12_FONT);
    OSTimeDlyHMSM(0,0,0,10); 
    PutString_cn(48, 50, "重启完成",LCD_HZK24_FONT);
    OSTimeDlyHMSM(0,0,0,500); 
    jump_modAdminMenu();
    gprs_status=GPRS_ST_NO_MODEL;
}
void jump_modResetBar()
{
    noOperateCountPage=0;
    ClrScreen();
    OSTimeDlyHMSM(0,0,0,10); 
    PutString_cn(10, 50, "条码模块重启中",LCD_HZK24_FONT);
    SMM_SW_OFF();
    SPEAKER_ON();
    OSTimeDlyHMSM(0,0,0,500); 
    SPEAKER_OFF();
    SMM_SW_ON();
    barcodeShutCount=0;
    barcode_on=1;
    OSTimeDlyHMSM(0,0,0,500); 
    ClrScreen();
    OSTimeDlyHMSM(0,0,0,10); 
    PutString_cn(10, 50, "条码模块重启完成",LCD_HZK24_FONT);
    OSTimeDlyHMSM(0,0,0,500); 
    jump_modAdminMenu();
    draw_gprs_status();
    draw_gprs_sig(ANY_WAY);
}
void jump_modResetIC()
{
    noOperateCountPage=0;
    ClrScreen();
    OSTimeDlyHMSM(0,0,0,10); 
    PutString_cn(10, 50, "刷卡模块重启中",LCD_HZK24_FONT);
    SK_SW_OFF();
    SPEAKER_ON();
    OSTimeDlyHMSM(0,0,0,500); 
    SPEAKER_OFF();
    SK_SW_ON();
    OSTimeDlyHMSM(0,0,0,500); 
    ClrScreen();
    OSTimeDlyHMSM(0,0,0,10); 
    PutString_cn(10, 50, "刷卡模块重启完成",LCD_HZK24_FONT);
    OSTimeDlyHMSM(0,0,0,500); 
    jump_modAdminMenu();
    draw_gprs_status();
    draw_gprs_sig(ANY_WAY);
}

/*************************************************/
/***组包*****/
/*|0      |2  |3                 |11 |13               |17                             |24    */
/*|59 47|01|CCCCCCCC|CC|XX XX XX XX|XX XX XX XX XX XX XX|data*/
/*|包头|版本|柜号|门|包长|时间|数据*/
/*C为字符，一个C占一个字节*/
/*时间:|年0xFF 0xFF|月0xFF|日0xFF|时0xFF|分0xFF|秒0xFF|*/
/***********************************************************/
u16 packMake(u8 *packData,u8 *cmd)
{
    u16 i;
    packData[0]=PACK_HEAD1;
    packData[1]=PACK_HEAD2;
    packData[2]=PACK_VER;
    /*加入柜号*/
    for(i=0;i<8;i++)
        packData[3+i]=sysInfo.sysData.guizi_id[i];
    /*加入指令号*/
    for(i=0;i<2;i++)
        packData[11+i]=cmd[i];
    /*加入包长data length(初始为0)*/
    for(i=0;i<4;i++)
        packData[13+i]=0;
    /*加入时间*/
    packData[17]=(u8)(sysInfo.sysData.year>>8);
    packData[18]=(u8)(sysInfo.sysData.year&&0x000000FF);
    packData[19]=(u8)sysInfo.sysData.month;
    packData[20]=(u8)sysInfo.sysData.day;
    packData[21]=(u8)sysInfo.sysData.hour;
    packData[22]=(u8)sysInfo.sysData.minute;
    packData[23]=(u8)sysInfo.sysData.second;
    return 24;
}
/*指令解析函数*/
u8 packDecode(unsigned char *packData)
{
    int i=0,dataLen=0;
    char cmd[3];
    char guiziNum[9];
    //获取柜号
    for(i=0;i<8;i++)
        guiziNum[i]=packData[i+3];
    guiziNum[8]=0x00;
    if(strcmp(guiziNum,sysInfo.sysData.guizi_id)!=0)//判断柜号对错
        return 0;
    //获取指令号
    for(i=0;i<2;i++)
        cmd[i]=packData[i+11];
    cmd[2]=0x00;
    //获取数据域长度
    dataLen=(packData[13]<<24)+(packData[14]<<16)+(packData[15]<<8)+packData[16];

    if (cmd[0]=='0')// 指令"0X"部分
    {
        switch(cmd[1])
        {
            case '2'://"02"   开机指令反馈
            {
                /*取出服务器时间并赋值*/
                sysInfo.sysData.year=((packData[17]<<8)+packData[18]);
                sysInfo.sysData.month=packData[19];
                sysInfo.sysData.day=packData[20];
                sysInfo.sysData.hour=packData[21];
                sysInfo.sysData.minute=packData[22];
                sysInfo.sysData.second=packData[23];
                /***************/
                adjust_screen_light();//判断调整屏幕亮度
                break;
            }
            case '4'://"04"    定时在线指令反馈
            {
                /*取出服务器时间并赋值*/
                sysInfo.sysData.year=((packData[17]<<8)+packData[18]);
                sysInfo.sysData.month=packData[19];
                sysInfo.sysData.day=packData[20];
                sysInfo.sysData.hour=packData[21];
                sysInfo.sysData.minute=packData[22];
                sysInfo.sysData.second=packData[23];
                /***************/
                adjust_screen_light();//判断调整屏幕亮度
                break;
            }
            case '5'://"05"    打开指定柜门
            {
                /*取出待开门的柜号*/
                opendoor(packData[24]);
            }
            default:
                break;
        }
    }
    else if(cmd[0]=='1')// 指令"1X"部分
    {
        
    }
    else 
        return 0;
    return 1;
}

/*****GPRS发送数据******/
u8 gprs_send(u8 *sendData,u8 *recv_data,u16 dataLen)
{
    if(sim900a_send_cmd("AT+CIPSEND",">",recv_data,1000,0)==0)return 0;//
    sim900a_send_cmd(sendData,0,recv_data,1000,dataLen);
    if(sim900a_send_cmd(gprs_send_go,"SEND OK",recv_data,1000,0)==0)return 0;
    return 1;
}
/***********/
u8 chk_cops(u8 *recv_data)
{
    u8 msg[1];
    u8 strength=0;
    u8 *strtmp;
    u8 start=0;// 检测到命令字的位置
    u8 str_unicom[]="CHN-UNICOM";
    u8 str_cmcc[]="CHINA MOBILE";
    u8 cops=0;
    int i;

    start=sim900a_send_cmd("AT+COPS?","OK",recv_data,1000,0);// 查询运营商
    if(start!=0)
    {
        if(strstr(recv_data,str_cmcc)!=NULL)
        {
            cops=1;
        }
        if(strstr(recv_data,str_unicom)!=NULL)
        {
            cops=2;
        }
    }
    return cops;
}
void cal_signal(u8 *recv_data)
{
    u8 strength=0;
    u8 *strtmp;
    u8 start=0;// 检测到命令字的位置

    start=sim900a_send_cmd("AT+CSQ","OK",recv_data,1000,0);// 查询信号质量
    if(start!=0)
    {
        // 计算信号强度
        strtmp=strstr(recv_data,"CSQ:");
        strength=(strtmp[5]&0x0F);
        if(strtmp[6]!=0x2C)// 不等于","表示有两位
        {
            strength=strength*10+(strtmp[6]&0x0F);
        }
        gprs_strength=(strength*6)/31;
        draw_gprs_sig(0);
    }
}

void opendoorAll()
{
    int i=0;
    for (i=0;i<sysInfo.sysData.door_num;i++)
    {
        opendoor(i);
        OSTimeDlyHMSM(0, 0, 1, 0);
    }
}

// 打开指定柜门
void opendoor(u8 doornum)
{
    u8 buff2, z, remain;
    /*
    if (doornum == DOOR_ALL)
    {
        buff2 = 0x00;
        ISendStr(PCA9555_01W, 0x02, &buff2, 1);
        ISendStr(PCA9555_02W, 0x02, &buff2, 1);
        ISendStr(PCA9555_03W, 0x02, &buff2, 1);
        OSTimeDlyHMSM(0, 0, 0, 10); // 延时10ms
        buff2 = 0xFF;
        ISendStr(PCA9555_01W, 0x02, &buff2, 1);
        ISendStr(PCA9555_02W, 0x02, &buff2, 1);
        ISendStr(PCA9555_03W, 0x02, &buff2, 1);
    }
    else
    */
    {
        doornum-=1;
        buff2 = 0xFF;
        z = doornum / 8;
        remain = doornum % 8;
        clrbit(buff2, remain);
        //buff2 = buff2 ^ (buff2 & remain);
        if (z == 0)
        {
            ISendStr(PCA9555_01W, 0x02, &buff2, 1);
            OSTimeDlyHMSM(0, 0, 0, 200); // 延时10ms
            buff2 = 0xFF;
            ISendStr(PCA9555_01W, 0x02, &buff2, 1);
        }
        else if (z == 1)
        {
            ISendStr(PCA9555_02W, 0x02, &buff2, 1);
            OSTimeDlyHMSM(0, 0, 0, 200); // 延时10ms
            buff2 = 0xFF;
            ISendStr(PCA9555_02W, 0x02, &buff2, 1);
        }
        else if (z == 2)
        {
            ISendStr(PCA9555_03W, 0x02, &buff2, 1);
            OSTimeDlyHMSM(0, 0, 0, 200); // 延时10ms
            buff2 = 0xFF;
            ISendStr(PCA9555_03W, 0x02, &buff2, 1);
        }
    }
}


// 键盘扫描程序
int keyScan()
{
    int KeyValue=-1;
    KB_H1_OFF();
    KB_H2_ON();
    KB_H3_ON();
    KB_H4_ON();
    OSTimeDlyHMSM(0,0,0,1); 
    if(!KB_V1_read){
        KeyValue=KEY_1;
        KB_A1_ON();
    }
    else if(!KB_V2_read){
        KeyValue=KEY_2;
        KB_A2_ON();
    }
    else if(!KB_V3_read){
        KeyValue=KEY_3;
        KB_A3_ON();
    }
    else if(!KB_V4_read){
        KeyValue=KEY_UP;
        KB_A4_ON();
    }
    else;
    KB_H1_ON();
    KB_H2_OFF();
    OSTimeDlyHMSM(0,0,0,1); 
    if(!KB_V1_read){
        KeyValue=KEY_4;
        KB_A1_ON();
    }
    else if(!KB_V2_read){
        KeyValue=KEY_5;
        KB_A2_ON();
    }
    else if(!KB_V3_read){
        KeyValue=KEY_6;
        KB_A3_ON();
    }
    else if(!KB_V4_read){
        KeyValue=KEY_CANCEL;
        KB_A4_ON();
    }
    else;
    KB_H2_ON();
    KB_H3_OFF();
    OSTimeDlyHMSM(0,0,0,1); 
    if(!KB_V1_read){
        KeyValue=KEY_7;
        KB_A1_ON();
    }
    else if(!KB_V2_read){
        KeyValue=KEY_8;
        KB_A2_ON();
    }
    else if(!KB_V3_read){
        KeyValue=KEY_9;
        KB_A3_ON();
    }
    else if(!KB_V4_read){
        KeyValue=KEY_ENTER;
        KB_A4_ON();
    }
    else;
    KB_H3_ON();
    KB_H4_OFF();
    delay_ms(1);
    if(!KB_V1_read){
        KeyValue=KEY_PUT;
        KB_A1_ON();
    }
    else if(!KB_V2_read){
        KeyValue=KEY_0;
        KB_A2_ON();
    }
    else if(!KB_V3_read){
        KeyValue=KEY_GET;
        KB_A3_ON();
    }
    else if(!KB_V4_read){
        KeyValue=KEY_DOWN;
        KB_A4_ON();
    }
    else;
    KB_H4_ON();
    OSTimeDlyHMSM(0,0,0,1); 
    return KeyValue;
}

//////IC卡命令
//参数:
//i 命令长度
//command 命令字
//recv_data接收地址
//返回:  接收长度
u16 ic_command(u16 cmd_len,unsigned char *command,unsigned char *recv_data)
{
    INT8U err;
    u16 len_usart=0,i;
    for(i=0;i<cmd_len;i++)
        QueueIn(&USART2Send, command[i]);
    USART_ITConfig(USART2, USART_IT_TXE, ENABLE);						//使能发送缓冲空中断  
    // 等待串口数据
    OSMboxPend(iccard_rev_MBOX, 1000, &err);
    if(USART2Recieve.bufferCount!=0)
    {
        len_usart=USART2Recieve.bufferCount;
        for(i=0;i<len_usart;i++)
        {
            QueueOut(&USART2Recieve, &recv_data[i]);
        }
    }
    return len_usart;
}

//向sim900a发送命令
//cmd:发送的命令字符串(不需要添加回车了),当cmd<0XFF的时候,发送数字(比如发送0X1A),大于的时候发送字符串.
//ack:期待的应答结果,如果为空,则表示不需要等待应答
//waittime:等待时间(单位:10ms)
//cmd_len:要发送数据长度，为0则为实际字符串长度
//返回值:ack位置
u8 sim900a_send_cmd(u8 *cmd,u8 *ack,unsigned char *recv_data,u16 timeout,u16 cmd_len)
{
    INT8U err;
    u8 *res;
    u8 pos=1;
    u16 len_usart=0,i;
    //u16 cmd_len=0;
    res=0;
    if(cmd)
    {
        if(cmd_len==0)//表示发送的非数据
            cmd_len=strlen((char *)cmd);
        for(i=0;i<cmd_len;i++)
            QueueIn(&USART3Send, cmd[i]);
        QueueIn(&USART3Send, 0x0D);
        QueueIn(&USART3Send, 0x0A);
        USART_ITConfig(USART3, USART_IT_TXE, ENABLE);						//使能发送缓冲空中断  
    }
    // 等待串口数据
    OSMboxPend(gprs_rev_MBOX, timeout, &err);
    USART3Send.bufferCount=0;
    if(USART3Recieve.bufferCount!=0)
    {
        len_usart=USART3Recieve.bufferCount;
        for(i=0;i<len_usart;i++)
        {
            QueueOut(&USART3Recieve, &recv_data[i]);
        }
    }
    ////////debug
    //PutString(20, 200, recv_data, LCD_ASC8_FONT);
    /////////////
    if(ack)		//需要等待应答�
    {
        res=strstr((const char*)recv_data,(const char*)ack);	//得到有效数据
        if(res==NULL)
        {
            pos=0;
        }
        else
        {
            pos=res-recv_data;
        }
    }
    return pos;
}

void packInt(unsigned char *pack_data,int data)
{
    
}

int SendChar (int ch)  {                /* Write character to Serial Port     */

  USART_SendData(USART1, (unsigned char) ch);
  while (!(USART1->SR & USART_FLAG_TXE));
  return (ch);
}

/******************************************************
		格式化串口输出函数
        "\r"	回车符	   USART_OUT(USART1, "abcdefg\r")   
		"\n"	换行符	   USART_OUT(USART1, "abcdefg\r\n")
		"%s"	字符串	   USART_OUT(USART1, "字符串是：%s","abcdefg")
		"%d"	十进制	   USART_OUT(USART1, "a=%d",10)
**********************************************************/
void USART_OUT(USART_TypeDef* USARTx, uint8_t *Data,...){ 

	const char *s;
    int d;
   
    char buf[16];
    va_list ap;
    va_start(ap, Data);

	while(*Data!=0){				                          //判断是否到达字符串结束符
		if(*Data==0x5c){									  //'\'
			switch (*++Data){
				case 'r':							          //回车符
					USART_SendData(USARTx, 0x0d);	   

					Data++;
					break;
				case 'n':							          //换行符
					USART_SendData(USARTx, 0x0a);	
					Data++;
					break;
				
				default:
					Data++;
				    break;
			}
			
			 
		}
		else if(*Data=='%'){									  //
			switch (*++Data){				
				case 's':										  //字符串
                	s = va_arg(ap, const char *);
                	for ( ; *s; s++) {
                    	USART_SendData(USARTx,*s);
						while(USART_GetFlagStatus(USARTx, USART_FLAG_TC)==RESET);
                	}
					Data++;
                	break;
            	case 'd':										  //十进制
                	d = va_arg(ap, int);
                	itoa(d, buf, 10);
                	for (s = buf; *s; s++) {
                    	USART_SendData(USARTx,*s);
						while(USART_GetFlagStatus(USARTx, USART_FLAG_TC)==RESET);
                	}
					Data++;
                	break;
				default:
					Data++;
				    break;
			}		 
		}
		else USART_SendData(USARTx, *Data++);
		while(USART_GetFlagStatus(USARTx, USART_FLAG_TC)==RESET);
	}
}

/******************************************************
		整形数据转字符串函数
        char *itoa(int value, char *string, int radix)
		radix=10 标示是10进制	非十进制，转换结果为0;  

	    例：d=-379;
		执行	itoa(d, buf, 10); 后
		
		buf="-379"							   			  
**********************************************************/
char *itoa(int value, char *string, int radix)
{
    int     i, d;
    int     flag = 0;
    char    *ptr = string;

    /* This implementation only works for decimal numbers. */
    if (radix != 10)
    {
        *ptr = 0;
        return string;
    }

    if (!value)
    {
        *ptr++ = 0x30;
        *ptr = 0;
        return string;
    }

    /* if this is a negative value insert the minus sign. */
    if (value < 0)
    {
        *ptr++ = '-';

        /* Make the value positive. */
        value *= -1;
    }

    for (i = 10000; i > 0; i /= 10)
    {
        d = value / i;

        if (d || flag)
        {
            *ptr++ = (char)(d + 0x30);
            value -= (d * i);
            flag = 1;
        }
    }

    /* Null terminate the string. */
    *ptr = 0;

    return string;

} /* NCL_Itoa */

// 设备上电代码段
void device_init()
{
    u8 buff=0x00;
    QD_SW_ON();   
    QDDY_SW_ON();
    GPRS_SW_ON();
    SK_SW_ON();
    SMM_SW_OFF();

    delay_ms(10);

    // 设置PCA9555的工作方式
    buff=0x00;
    ISendStr(PCA9555_01W,0x06,&buff,1);// IO0为输出
    ISendStr(PCA9555_02W,0x06,&buff,1);// IO0为输出
    ISendStr(PCA9555_03W,0x06,&buff,1);// IO0为输出
    delay_ms(10);
    buff=0xFF;
    ISendStr(PCA9555_01W,0x07,&buff,1);// IO1为输入
    ISendStr(PCA9555_02W,0x07,&buff,1);// IO1为输入
    ISendStr(PCA9555_03W,0x07,&buff,1);// IO1为输入
    delay_ms(10);
    /////////////////
    // 关所有柜子
    buff=0xFF;
    ISendStr(PCA9555_01W,0x02,&buff,1);
    ISendStr(PCA9555_02W,0x02,&buff,1);
    ISendStr(PCA9555_03W,0x02,&buff,1);
    delay_ms(10);

    // 按键扫描初始化
    KB_H1_ON();
    KB_H2_ON();
    KB_H3_ON();
    KB_H4_ON();
    KB_A1_OFF();
    KB_A2_OFF();
    KB_A3_OFF();
    KB_A4_OFF();
}

void adjust_screen_light()
{
    INT8U err;
    /****根据时间设置屏幕亮度****/
    if((sysInfo.sysData.hour>=HIGH_LIGHT_HOUR)&&(sysInfo.sysData.hour<LOW_LIGHT_HOUR))// 6点到18点间设置屏幕亮度为9
    {
        if(scrBackLight!=HIGH_LIGHT_LEVEL)
        {
            scrBackLight=HIGH_LIGHT_LEVEL;
            /***display***********/
            OSMutexPend(screenMutexSem,0,&err);
            SetBackLight(scrBackLight);
            OSMutexPost(screenMutexSem);
            /*****display end****/
        }
    }
    else// 其余时间设置屏幕亮度为低
    {
        if(scrBackLight!=LOW_LIGHT_LEVEL)
        {
            scrBackLight=LOW_LIGHT_LEVEL;
            /***display***********/
            OSMutexPend(screenMutexSem,0,&err);
            SetBackLight(scrBackLight);
            OSMutexPost(screenMutexSem);
            /*****display end****/
        }
    }
    /**************************/
}

void clearData()
{
    char door_str[3];
    int i = 0;
    sysInfo.sysData.year=2014;
    sysInfo.sysData.month=01;
    sysInfo.sysData.day=01;
    sysInfo.sysData.hour=19;
    sysInfo.sysData.minute=59;
    sysInfo.sysData.second=30;
    strcpy(sysInfo.sysData.guizi_id, "00000001");
    strcpy(sysInfo.sysData.superAdminPwd, "888888");
    strcpy(sysInfo.sysData.adminPwd, "666666");
    strcpy(sysInfo.sysData.compName, "方缘百帮家事");
    sysInfo.sysData.compNameLen = 12;
    sysInfo.sysData.door_num = GUIZI_NUM;
    strcpy(sysInfo.sysData.servIP, "115.029.250.152");
    sysInfo.sysData.port = 5000;
    for (i = 0; i < GUIZI_NUM; i++)
    {
        sprintf(door_str,"%02d",i+1);//得到柜门的字符串
        strcpy(sysInfo.sysData.guizi[i].door_id, door_str);
        sysInfo.sysData.guizi[i].status = 0;
        sysInfo.sysData.guizi[i].doorstatus= 0;
        strcpy(sysInfo.sysData.guizi[i].userCard, "00000000");
        strcpy(sysInfo.sysData.guizi[i].userPwd, "000000");
    }
}
void sysInit()
{
    SST25_R_BLOCK(0, sysInfo.dataBuff, sizeof(sdata));
    if(sysInfo.sysData.isUsed!=0xAA)//Flash没有数据
    {
        sysInfo.sysData.isUsed=0xAA;
        clearData();
        SST25_W_BLOCK(0, sysInfo.dataBuff, SST25_SECTOR_SIZE);
    }
    if((sysInfo.sysData.superAdminPwd[0]<0x30)||(sysInfo.sysData.superAdminPwd[0]>0x39)||(sysInfo.sysData.superAdminPwd[1]<0x30)||(sysInfo.sysData.superAdminPwd[1]>0x39))
    {
        clearData();
    }
}



/******************************************************************/
/*****STM32普通延时（基于MDK固件库3.0，晶振8M）***/
/******************************************************************/
//粗延时函数，微秒
void delay_us(u16 time)
{
    u16 i=0;
    while(time--)
    {
        i=10;
        while(i--)
		;
    }
}

//毫秒级的延时
void delay_ms(u16 time)
{
    u16 i=0;
    while(time--)
    {
        i=12000;
        while(i--)
		;
    }
}
/******************************************************************/
/******************************************************************/


/*
*********************************************************************************************************
*********************************************************************************************************
*                                          uC/OS-II APP HOOKS
*********************************************************************************************************
*********************************************************************************************************
*/

#if (OS_APP_HOOKS_EN > 0)
/*
*********************************************************************************************************
*                                      TASK CREATION HOOK (APPLICATION)
*
* Description : This function is called when a task is created.
*
* Argument : ptcb   is a pointer to the task control block of the task being created.
*
* Note     : (1) Interrupts are disabled during this call.
*********************************************************************************************************
*/

void App_TaskCreateHook(OS_TCB* ptcb)
{
}

/*
*********************************************************************************************************
*                                    TASK DELETION HOOK (APPLICATION)
*
* Description : This function is called when a task is deleted.
*
* Argument : ptcb   is a pointer to the task control block of the task being deleted.
*
* Note     : (1) Interrupts are disabled during this call.
*********************************************************************************************************
*/

void App_TaskDelHook(OS_TCB* ptcb)
{
   (void) ptcb;
}

/*
*********************************************************************************************************
*                                      IDLE TASK HOOK (APPLICATION)
*
* Description : This function is called by OSTaskIdleHook(), which is called by the idle task.  This hook
*               has been added to allow you to do such things as STOP the CPU to conserve power.
*
* Argument : none.
*
* Note     : (1) Interrupts are enabled during this call.
*********************************************************************************************************
*/

#if OS_VERSION >= 251
void App_TaskIdleHook(void)
{
}
#endif

/*
*********************************************************************************************************
*                                        STATISTIC TASK HOOK (APPLICATION)
*
* Description : This function is called by OSTaskStatHook(), which is called every second by uC/OS-II's
*               statistics task.  This allows your application to add functionality to the statistics task.
*
* Argument : none.
*********************************************************************************************************
*/

void App_TaskStatHook(void)
{
}

/*
*********************************************************************************************************
*                                        TASK SWITCH HOOK (APPLICATION)
*
* Description : This function is called when a task switch is performed.  This allows you to perform other
*               operations during a context switch.
*
* Argument : none.
*
* Note     : 1 Interrupts are disabled during this call.
*
*            2  It is assumed that the global pointer 'OSTCBHighRdy' points to the TCB of the task that
*                   will be 'switched in' (i.e. the highest priority task) and, 'OSTCBCur' points to the
*                  task being switched out (i.e. the preempted task).
*********************************************************************************************************
*/

#if OS_TASK_SW_HOOK_EN > 0
void App_TaskSwHook(void)
{
}
#endif

/*
*********************************************************************************************************
*                                     OS_TCBInit() HOOK (APPLICATION)
*
* Description : This function is called by OSTCBInitHook(), which is called by OS_TCBInit() after setting
*               up most of the TCB.
*
* Argument : ptcb    is a pointer to the TCB of the task being created.
*
* Note     : (1) Interrupts may or may not be ENABLED during this call.
*********************************************************************************************************
*/

#if OS_VERSION >= 204
void App_TCBInitHook(OS_TCB* ptcb)
{
   (void) ptcb;
}
#endif

#endif
