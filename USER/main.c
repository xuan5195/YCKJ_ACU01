#include "led.h"
#include "delay.h"
#include "key.h"
#include "sys.h"
#include "wdg.h"
#include "bsp_24cxx.h"
#include "bsp_myiic.h"
#include "bsp_powerbus.h"
#include "bsp_uart_fifo.h"
#include "bsp_crc8.h"
#include "timer.h"
#include "sram.h"
#include "malloc.h"
#include "string.h"
#include "usmart.h"	
#include "dm9000.h"
#include "lwip/netif.h"
#include "lwip_comm.h"
#include "lwipopts.h"
#include "includes.h"
#include "tcp_client_demo.h"
#include "bsp_useadd.h"
#include "bsp_serialport.h"
#include "bsp_can.h"
#include "bsp_canapp.h"

uint8_t g_RUNDate[32][16]={0};    		//运行数据；
uint8_t KJ_Versions[32]={0};			//卡机版本号缓存
uint8_t g_PowerUpFlag=0;				//上电标志，0xAA上电完成
uint8_t g_lwipADD[4]={0};				//远端IP地址
uint16_t g_lwipPort=0;					//远端端口号
uint8_t g_ACUAdd[16]={0};				//通信码16位
uint8_t g_ACUSN[4]={0};					//区域控制器SN 4位
uint8_t g_SerialDat[32]={0};			//串口通信数据
uint8_t g_Serial_Count=0;				//串口接收计数
uint8_t g_CostNum = 29;					//流量计脉冲数 每升水计量周期
uint8_t g_WaterCost = 5;				//WaterCost=水费 最小扣款金额 0.005元
uint8_t FM1702KeyCRC;					//FM1702KeyCRC
uint8_t FM1702_Key[7];					//FM1702_Key[0]-[5]为Key;FM1702_Key[6]为块地址；

//uint8_t g_SetIPFlag=0x00;	//0xAA:使用静态IP地址
uint8_t g_Setip[5];       	//本机IP地址 g_Setip[0] 为使用标志
uint8_t g_Setnetmask[4]; 	//子网掩码
uint8_t g_Setgateway[4]; 	//默认网关的IP地址
uint8_t g_NewAddFlag=0x00; 	//新增标志

//KEY任务
#define KEY_TASK_PRIO 		9		//任务优先级
#define KEY_STK_SIZE		256		//任务堆栈大小
OS_STK KEY_TASK_STK[KEY_STK_SIZE];	//任务堆栈
void key_task(void *pdata);  		//任务函数
 

//LED任务
#define LED_TASK_PRIO		10		//任务优先级
#define LED_STK_SIZE		512		//任务堆栈大小
OS_STK	LED_TASK_STK[LED_STK_SIZE];	//任务堆栈
void led_task(void *pdata); 		//任务函数
 

//START任务
#define START_TASK_PRIO		12			//任务优先级
#define START_STK_SIZE		128			//任务堆栈大小
OS_STK START_TASK_STK[START_STK_SIZE];	//任务堆栈
void start_task(void *pdata);			//任务函数 

#define MsgGrp_SIZE		64			//消息队列大小

//OS_TMR   * tmr1;			//软件定时器1
OS_EVENT * q_msg_ser;		//消息队列 服务器回复数据
OS_EVENT * q_msg;			//消息队列
void * MsgGrp_ser[MsgGrp_SIZE];		//消息队列存储地址,最大支持100个消息	 服务器回复数据
void * MsgGrp[MsgGrp_SIZE];			//消息队列存储地址,最大支持100个消息	

 int main(void)
 {	 
	delay_init();	    	//延时函数初始化	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	bsp_InitUart(); 	/* 初始化串口 */
 	LED_Init();			    //LED端口初始化
	CAN_Mode_Init(CAN_SJW_1tq,CAN_BS1_8tq,CAN_BS2_7tq,5,CAN_Mode_Normal);//CAN初始化正常模式,波特率450Kbps    

	 usmart_dev.init(72);	//初始化USMART		 
 	//FSMC_SRAM_Init();		//初始化外部SRAM
	my_mem_init(SRAMIN);	//初始化内部内存池
	//my_mem_init(SRAMEX);	//初始化外部内存池
    printf("Starting Up YCKJ-ACU...\r\n");
	printf("VersionNo: %02X...\r\n",VersionNo);
	printf("TestFlag: %d...\r\n",TestFlag);
	AT24CXX_Init();			    	//IIC 初始化
	while(AT24CXX_Check()){};      	//对IIC进行检测
    printf("AT24CXX_Check OK!\r\n");
	delay_ms(500); 
	OSInit();				//UCOS初始化
	while(lwip_comm_init()) //lwip初始化
	{
        printf("Lwip Init failed!\r\n");
		delay_ms(500);		delay_ms(500);
	}
    printf("Lwip Init Success!\r\n");   //lwip初始化成功
	while(tcp_client_init()) 			//初始化tcp_client(创建tcp_client线程)
	{
        printf("TCP Client failed!!\r\n");   //lwip初始化成功
		delay_ms(500);		delay_ms(500);
	}
    delay_ms(500);
    printf("TCP Client Success!\r\n");   //TCP创建成功
	OSTaskCreate(start_task,(void*)0,(OS_STK*)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO);
	OSStart(); //开启UCOS
}
 
//start任务
void start_task(void *pdata)
{
	OS_CPU_SR cpu_sr;
	pdata = pdata ;
	
	q_msg_ser = OSQCreate(&MsgGrp_ser[0],MsgGrp_SIZE);	//创建消息队列 服务器回复
	q_msg = OSQCreate(&MsgGrp[0],MsgGrp_SIZE);	//创建消息队列
	
	OSStatInit();  			//初始化统计任务
	OS_ENTER_CRITICAL();  	//关中断
	OSTaskCreate(led_task,(void*)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO); 	//创建LED任务
	OSTaskCreate(key_task,(void*)0,(OS_STK*)&KEY_TASK_STK[KEY_STK_SIZE-1],KEY_TASK_PRIO); 	//创建KEY任务
	OSTaskSuspend(OS_PRIO_SELF); //挂起start_task任务
	OS_EXIT_CRITICAL();  //开中断
}



//key任务
void key_task(void *pdata)
{
    uint8_t PrintfCount;
    uint16_t Key_Task_Count=0,HeartSendCount=0;
	uint8_t SerialSetFlag=0x00;	//串口设置标志，修改配置后，重启才能与服务器通信	

	while(1)
	{
		
		if(g_PowerUpFlag==0x00)	//等待上电初始化完成
		{
			OSTimeDlyHMSM(0,0,0,50);  //延时50ms
			#if TestFlag
				HeartSendCount=1600;	//测试使用
			#else
				HeartSendCount=0;
			#endif
		}
		else
		{
			if(SerialSetFlag==0x00)	//串口设置标志，修改配置后，重启才能与服务器通信
			{
				if(Key_Task_Count<6000) {	Key_Task_Count++;	}
				else 					{	Key_Task_Count = 0;	} //周期为300秒; 6000*50ms=300 000ms=300s
				if(g_NewAddFlag==0xAA)	{	g_NewAddFlag=0x00;	HeartSendCount=1000;}
				if(HeartSendCount<80)	{	HeartSendCount++;	PrintfCount=0;} //去掉刚上电时卡机还没注册；
				else if(HeartSendCount<1800)
				{
					HeartSendCount++;
					if((Key_Task_Count%30)==0)  //测试改为    s发一次；
					{
						tcp_client_flag |= LWIP_SEND_HeartbeatDATA; //标记LWIP有心跳数据包要发送;
						PrintfCount++;
						printf("\r\n 快递心跳包发送状态,发送次数%2d,计数值：%d.",PrintfCount,HeartSendCount); 
					}
				}
				else
				{
					HeartSendCount = 5000;
//					#if TestFlag
//						if((Key_Task_Count%50)==1)  //2.5秒钟发一次；//测试使用
//							tcp_client_flag |= LWIP_SEND_HeartbeatDATA; //标记LWIP有心跳数据包要发送;				
//					#else
						if((Key_Task_Count%200)==1)  //10秒钟发一次；
							tcp_client_flag |= LWIP_SEND_HeartbeatDATA; //标记LWIP有心跳数据包要发送;				
//					#endif
				}
				
			}
			OSTimeDlyHMSM(0,0,0,50);  //延时50ms		
		}
		//串口3检测 参数设置
		ReceiveSerialDat();
		if( ( 0x80 & g_Serial_Count ) == 0x80 )	//接收到数据
		{
			g_Serial_Count = 0x00;
			SerialSetFlag=0xAA;
			printf("\r\n串口参数设置，断开与服务器通信，重启区域控制器才能恢复与服务器通信;\r\n"); 
			tcp_client_flag = tcp_client_flag&(~LWIP_SEND_HeartbeatDATA);
			//tcp_client_flag = tcp_client_flag&(~LWIP_SEND_DATA);			
			SendSerialPort((uint8_t *)g_SerialDat);	//回复
		}
	}
}

//led任务//总线通信方式
void led_task(void *pdata)
{
	uint8_t Old_CostNum = 29;		//流量计脉冲数 每升水计量周期
	uint8_t Old_WaterCost = 5;		//WaterCost=水费 最小扣款金额 0.005元
	uint8_t Old_FM1702KeyCRC = 0;	//Old_FM1702KeyCRC
	uint8_t Broadcast_Count=0;		//广播发送计数值
	uint8_t PhysicalADD[4] = {0x00,0x00,0x00,0x00};
    uint8_t u8Temp; //存储临时数据
	uint8_t Led_TaskCount = 0;
    uint8_t ucCount = 50;	//上电50秒
	uint8_t CycleCount =0;
	uint32_t BroadTime=0;
	
	u8 *p;
	u8 p_err;
	
	u8* g_SendDate;	 
	u8 q_err; 
	g_SendDate=mymalloc(SRAMIN,16);	//申请16个字节的内存
	
	
    g_RUNDate[0][0] = 0;    //存储卡机使用总数，上电清0
	LED0 = 0;LED1 = 1;LED2 = 1;
	
    OSTimeDlyHMSM(0,0,2,500);  	//上电延时，等待总线上电完成
    PowerUPLogitADDCheck(); 	//上电检测Flash内逻辑地址；
    printf("上电检测Flash内逻辑地址完成! 总线：%2d.\r\n",g_RUNDate[0][0]);  
	OSTimeDlyHMSM(0,0,0,100);  	//延时100ms
	Can_SendBroadcast_Key((uint8_t *)FM1702_Key);
    OSTimeDlyHMSM(0,0,0,100);  	//延时100ms
	Can_UnregisteredBroadcast();   //未注册广播
    printf("未注册广播! \r\n");      
	LED0 = 1;LED1 = 1;LED2 = 1;
	Old_CostNum = g_CostNum;	//流量计脉冲数 每升水计量周期
	Old_WaterCost = g_WaterCost;//WaterCost=水费 最小扣款金额 0.005元
	ucCount = ucCount - g_RUNDate[0][0];
	if((ucCount<20)||(ucCount>50))	ucCount = 20;
	#if TestFlag
		ucCount = 10;	//测试使用
	#endif
    while(ucCount)
    {		
        LED0 = !LED0;LED1 = !LED1;LED2 = !LED2;
        printf("%2d. ",ucCount); 
		if(Can_ReadUnregistered((uint8_t *)PhysicalADD)!=0x00)  //有数据
		{
            printf("物理地址：%02X%02X%02X%02X;",PhysicalADD[0],PhysicalADD[1],PhysicalADD[2],PhysicalADD[3]);   
            u8Temp = Distribute_LogicADD((uint8_t *)PhysicalADD);	//分配物理地址
            printf("逻辑地址:%d;",u8Temp); 
            if(u8Temp<=32)
			{
				KJ_Versions[u8Temp] = Can_WriteLogitADD(u8Temp,(uint8_t *)PhysicalADD);
				if(KJ_Versions[u8Temp]==0x00)	printf("Ver[%02d]=%02X,分配失败！\r\n",u8Temp,KJ_Versions[u8Temp]);
				else							printf("Ver[%02d]=%02X,分配成功！\r\n",u8Temp,KJ_Versions[u8Temp]);
			}
		}
		ucCount--;		
	}
	LED0 = 0;LED1 = 1;LED2 = 1;
	OSTimeDlyHMSM(0,0,0,200);  		
	Can_SendBroadcast_Key((uint8_t *)FM1702_Key);
	OSTimeDlyHMSM(0,0,0,200);  		
	Can_SendBroadcast_Key((uint8_t *)FM1702_Key);
	g_PowerUpFlag = 0xAA;	//初始化完成
    printf("\r\n上电检测未注册超时，进入正常待机状态;\r\n"); 
	IWDG_Init(6,1024);    //与分频数为64,重载值为1024,溢出时间为6s	   
	while(1)
	{
		IWDG_Feed();	//增加看门狗
		if((Old_CostNum != g_CostNum)||(Old_WaterCost != g_WaterCost))
		{
			Can_SendBroadcast_Com(g_WaterCost,g_CostNum);
			if(Broadcast_Count<=2)	Broadcast_Count++;	//重复发送2次
			else
			{
				Broadcast_Count = 0;BroadTime=0;
				Old_CostNum = g_CostNum;	//流量计脉冲数 每升水计量周期
				Old_WaterCost = g_WaterCost;//WaterCost=水费 最小扣款金额 0.005元
			}
		}
		if(Old_FM1702KeyCRC!=FM1702KeyCRC)
		{
			Can_SendBroadcast_Key((uint8_t *)FM1702_Key);
			if(Broadcast_Count<=2)	Broadcast_Count++;	//重复发送2次
			else
			{
				Broadcast_Count = 0;
				Old_FM1702KeyCRC = FM1702KeyCRC;
			}
		}
		if(Led_TaskCount<BUSNUM_SIZE)
		{   //2.轮循方式取数据
			ReadRunningData(Led_TaskCount+1);			

			if(g_RUNDate[Led_TaskCount][1] == 0xAA)	//有数据
			{
				g_SendDate[0] = 0xAA;	//标志为使用
				if(g_RUNDate[Led_TaskCount][2] == 0x05)		g_SendDate[1] = 0x11;	//插卡
				else if(g_RUNDate[Led_TaskCount][2] == 0x06)g_SendDate[1] = 0x13;	//拔卡
				g_SendDate[2] = g_RUNDate[Led_TaskCount][3];	//卡机SN
				g_SendDate[3] = g_RUNDate[Led_TaskCount][4];	//卡机SN
				g_SendDate[4] = g_RUNDate[Led_TaskCount][5];	//卡机SN
				g_SendDate[5] = g_RUNDate[Led_TaskCount][6];	//卡机SN
				g_SendDate[6] = g_RUNDate[Led_TaskCount][7];	//CardSN
				g_SendDate[7] = g_RUNDate[Led_TaskCount][8];	//CardSN
				g_SendDate[8] = g_RUNDate[Led_TaskCount][9];	//CardSN
				g_SendDate[9] = g_RUNDate[Led_TaskCount][10];	//CardSN
				g_SendDate[10] = g_RUNDate[Led_TaskCount][11];	//Card金额1
				g_SendDate[11] = g_RUNDate[Led_TaskCount][12];	//Card金额2	
				g_SendDate[12] = g_RUNDate[Led_TaskCount][13];	//Card金额3		
				g_SendDate[13] = g_RUNDate[Led_TaskCount][14];	//校验	
				g_SendDate[14] = Led_TaskCount;//逻辑地址	
				g_SendDate[15] = g_RUNDate[Led_TaskCount][15];//通信码
				q_err=OSQPost(q_msg,g_SendDate);	//发送队列
				if(q_err!=OS_ERR_NONE) 	myfree(SRAMIN,g_SendDate);	//发送失败,释放内存
				g_RUNDate[Led_TaskCount][1] = 0xDD;					
			}

		}
		p=OSQPend(q_msg_ser,1,&p_err);//请求消息队列
		if((OS_ERR_NONE==p_err)&&(p!=NULL))
		{
			WriteRFIDData((uint8_t *)p);	
		}
		myfree(SRAMIN,p);

		if( Led_TaskCount >= (BUSNUM_SIZE+1) )    
		{   
			printf("\r\n\r\n");    	Led_TaskCount = 0;  
			if(CycleCount<50)		CycleCount++;	
			else	
			{	
				CycleCount = 0;
				if(Binary_searchSN()==0x00)
				{
					OSTimeDlyHMSM(0,0,0,50);  //延时50ms
					SendBroadcast_Com(g_WaterCost,g_CostNum);
					OSTimeDlyHMSM(0,0,0,200);  //延时200ms
					SendBroadcast_Key((uint8_t *)FM1702_Key);
					OSTimeDlyHMSM(0,0,0,200);  //延时200ms
					g_NewAddFlag = 0xAA;
				}					
			}
		}            
        else Led_TaskCount++;  
		if(BroadTime>0x000F0000)	//20ms *1000
		{
			BroadTime=0;
			SendBroadcast_Com(g_WaterCost,g_CostNum);
		}
		else BroadTime++;
		OSTimeDlyHMSM(0,0,0,10);  //延时10ms
 	}
}
