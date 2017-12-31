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

uint8_t g_RUNDate[32][16]={0};    		//�������ݣ�
uint8_t KJ_Versions[32]={0};			//�����汾�Ż���
uint8_t g_PowerUpFlag=0;				//�ϵ��־��0xAA�ϵ����
uint8_t g_lwipADD[4]={0};				//Զ��IP��ַ
uint16_t g_lwipPort=0;					//Զ�˶˿ں�
uint8_t g_ACUAdd[16]={0};				//ͨ����16λ
uint8_t g_ACUSN[4]={0};					//���������SN 4λ
uint8_t g_SerialDat[32]={0};			//����ͨ������
uint8_t g_Serial_Count=0;				//���ڽ��ռ���
uint8_t g_CostNum = 29;					//������������ ÿ��ˮ��������
uint8_t g_WaterCost = 5;				//WaterCost=ˮ�� ��С�ۿ��� 0.005Ԫ
uint8_t FM1702KeyCRC;					//FM1702KeyCRC
uint8_t FM1702_Key[7];					//FM1702_Key[0]-[5]ΪKey;FM1702_Key[6]Ϊ���ַ��

//uint8_t g_SetIPFlag=0x00;	//0xAA:ʹ�þ�̬IP��ַ
uint8_t g_Setip[5];       	//����IP��ַ g_Setip[0] Ϊʹ�ñ�־
uint8_t g_Setnetmask[4]; 	//��������
uint8_t g_Setgateway[4]; 	//Ĭ�����ص�IP��ַ
uint8_t g_NewAddFlag=0x00; 	//������־

//KEY����
#define KEY_TASK_PRIO 		9		//�������ȼ�
#define KEY_STK_SIZE		256		//�����ջ��С
OS_STK KEY_TASK_STK[KEY_STK_SIZE];	//�����ջ
void key_task(void *pdata);  		//������
 

//LED����
#define LED_TASK_PRIO		10		//�������ȼ�
#define LED_STK_SIZE		512		//�����ջ��С
OS_STK	LED_TASK_STK[LED_STK_SIZE];	//�����ջ
void led_task(void *pdata); 		//������
 

//START����
#define START_TASK_PRIO		12			//�������ȼ�
#define START_STK_SIZE		128			//�����ջ��С
OS_STK START_TASK_STK[START_STK_SIZE];	//�����ջ
void start_task(void *pdata);			//������ 

#define MsgGrp_SIZE		64			//��Ϣ���д�С

//OS_TMR   * tmr1;			//�����ʱ��1
OS_EVENT * q_msg_ser;		//��Ϣ���� �������ظ�����
OS_EVENT * q_msg;			//��Ϣ����
void * MsgGrp_ser[MsgGrp_SIZE];		//��Ϣ���д洢��ַ,���֧��100����Ϣ	 �������ظ�����
void * MsgGrp[MsgGrp_SIZE];			//��Ϣ���д洢��ַ,���֧��100����Ϣ	

 int main(void)
 {	 
	delay_init();	    	//��ʱ������ʼ��	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
	bsp_InitUart(); 	/* ��ʼ������ */
 	LED_Init();			    //LED�˿ڳ�ʼ��
	CAN_Mode_Init(CAN_SJW_1tq,CAN_BS1_8tq,CAN_BS2_7tq,5,CAN_Mode_Normal);//CAN��ʼ������ģʽ,������450Kbps    

	 usmart_dev.init(72);	//��ʼ��USMART		 
 	//FSMC_SRAM_Init();		//��ʼ���ⲿSRAM
	my_mem_init(SRAMIN);	//��ʼ���ڲ��ڴ��
	//my_mem_init(SRAMEX);	//��ʼ���ⲿ�ڴ��
    printf("Starting Up YCKJ-ACU...\r\n");
	printf("VersionNo: %02X...\r\n",VersionNo);
	printf("TestFlag: %d...\r\n",TestFlag);
	AT24CXX_Init();			    	//IIC ��ʼ��
	while(AT24CXX_Check()){};      	//��IIC���м��
    printf("AT24CXX_Check OK!\r\n");
	delay_ms(500); 
	OSInit();				//UCOS��ʼ��
	while(lwip_comm_init()) //lwip��ʼ��
	{
        printf("Lwip Init failed!\r\n");
		delay_ms(500);		delay_ms(500);
	}
    printf("Lwip Init Success!\r\n");   //lwip��ʼ���ɹ�
	while(tcp_client_init()) 			//��ʼ��tcp_client(����tcp_client�߳�)
	{
        printf("TCP Client failed!!\r\n");   //lwip��ʼ���ɹ�
		delay_ms(500);		delay_ms(500);
	}
    delay_ms(500);
    printf("TCP Client Success!\r\n");   //TCP�����ɹ�
	OSTaskCreate(start_task,(void*)0,(OS_STK*)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO);
	OSStart(); //����UCOS
}
 
//start����
void start_task(void *pdata)
{
	OS_CPU_SR cpu_sr;
	pdata = pdata ;
	
	q_msg_ser = OSQCreate(&MsgGrp_ser[0],MsgGrp_SIZE);	//������Ϣ���� �������ظ�
	q_msg = OSQCreate(&MsgGrp[0],MsgGrp_SIZE);	//������Ϣ����
	
	OSStatInit();  			//��ʼ��ͳ������
	OS_ENTER_CRITICAL();  	//���ж�
	OSTaskCreate(led_task,(void*)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO); 	//����LED����
	OSTaskCreate(key_task,(void*)0,(OS_STK*)&KEY_TASK_STK[KEY_STK_SIZE-1],KEY_TASK_PRIO); 	//����KEY����
	OSTaskSuspend(OS_PRIO_SELF); //����start_task����
	OS_EXIT_CRITICAL();  //���ж�
}



//key����
void key_task(void *pdata)
{
    uint8_t PrintfCount;
    uint16_t Key_Task_Count=0,HeartSendCount=0;
	uint8_t SerialSetFlag=0x00;	//�������ñ�־���޸����ú����������������ͨ��	

	while(1)
	{
		
		if(g_PowerUpFlag==0x00)	//�ȴ��ϵ��ʼ�����
		{
			OSTimeDlyHMSM(0,0,0,50);  //��ʱ50ms
			#if TestFlag
				HeartSendCount=1600;	//����ʹ��
			#else
				HeartSendCount=0;
			#endif
		}
		else
		{
			if(SerialSetFlag==0x00)	//�������ñ�־���޸����ú����������������ͨ��
			{
				if(Key_Task_Count<6000) {	Key_Task_Count++;	}
				else 					{	Key_Task_Count = 0;	} //����Ϊ300��; 6000*50ms=300 000ms=300s
				if(g_NewAddFlag==0xAA)	{	g_NewAddFlag=0x00;	HeartSendCount=1000;}
				if(HeartSendCount<80)	{	HeartSendCount++;	PrintfCount=0;} //ȥ�����ϵ�ʱ������ûע�᣻
				else if(HeartSendCount<1800)
				{
					HeartSendCount++;
					if((Key_Task_Count%30)==0)  //���Ը�Ϊ    s��һ�Σ�
					{
						tcp_client_flag |= LWIP_SEND_HeartbeatDATA; //���LWIP���������ݰ�Ҫ����;
						PrintfCount++;
						printf("\r\n �������������״̬,���ʹ���%2d,����ֵ��%d.",PrintfCount,HeartSendCount); 
					}
				}
				else
				{
					HeartSendCount = 5000;
//					#if TestFlag
//						if((Key_Task_Count%50)==1)  //2.5���ӷ�һ�Σ�//����ʹ��
//							tcp_client_flag |= LWIP_SEND_HeartbeatDATA; //���LWIP���������ݰ�Ҫ����;				
//					#else
						if((Key_Task_Count%200)==1)  //10���ӷ�һ�Σ�
							tcp_client_flag |= LWIP_SEND_HeartbeatDATA; //���LWIP���������ݰ�Ҫ����;				
//					#endif
				}
				
			}
			OSTimeDlyHMSM(0,0,0,50);  //��ʱ50ms		
		}
		//����3��� ��������
		ReceiveSerialDat();
		if( ( 0x80 & g_Serial_Count ) == 0x80 )	//���յ�����
		{
			g_Serial_Count = 0x00;
			SerialSetFlag=0xAA;
			printf("\r\n���ڲ������ã��Ͽ��������ͨ�ţ�����������������ָܻ��������ͨ��;\r\n"); 
			tcp_client_flag = tcp_client_flag&(~LWIP_SEND_HeartbeatDATA);
			//tcp_client_flag = tcp_client_flag&(~LWIP_SEND_DATA);			
			SendSerialPort((uint8_t *)g_SerialDat);	//�ظ�
		}
	}
}

//led����//����ͨ�ŷ�ʽ
void led_task(void *pdata)
{
	uint8_t Old_CostNum = 29;		//������������ ÿ��ˮ��������
	uint8_t Old_WaterCost = 5;		//WaterCost=ˮ�� ��С�ۿ��� 0.005Ԫ
	uint8_t Old_FM1702KeyCRC = 0;	//Old_FM1702KeyCRC
	uint8_t Broadcast_Count=0;		//�㲥���ͼ���ֵ
	uint8_t PhysicalADD[4] = {0x00,0x00,0x00,0x00};
    uint8_t u8Temp; //�洢��ʱ����
	uint8_t Led_TaskCount = 0;
    uint8_t ucCount = 50;	//�ϵ�50��
	uint8_t CycleCount =0;
	uint32_t BroadTime=0;
	
	u8 *p;
	u8 p_err;
	
	u8* g_SendDate;	 
	u8 q_err; 
	g_SendDate=mymalloc(SRAMIN,16);	//����16���ֽڵ��ڴ�
	
	
    g_RUNDate[0][0] = 0;    //�洢����ʹ���������ϵ���0
	LED0 = 0;LED1 = 1;LED2 = 1;
	
    OSTimeDlyHMSM(0,0,2,500);  	//�ϵ���ʱ���ȴ������ϵ����
    PowerUPLogitADDCheck(); 	//�ϵ���Flash���߼���ַ��
    printf("�ϵ���Flash���߼���ַ���! ���ߣ�%2d.\r\n",g_RUNDate[0][0]);  
	OSTimeDlyHMSM(0,0,0,100);  	//��ʱ100ms
	Can_SendBroadcast_Key((uint8_t *)FM1702_Key);
    OSTimeDlyHMSM(0,0,0,100);  	//��ʱ100ms
	Can_UnregisteredBroadcast();   //δע��㲥
    printf("δע��㲥! \r\n");      
	LED0 = 1;LED1 = 1;LED2 = 1;
	Old_CostNum = g_CostNum;	//������������ ÿ��ˮ��������
	Old_WaterCost = g_WaterCost;//WaterCost=ˮ�� ��С�ۿ��� 0.005Ԫ
	ucCount = ucCount - g_RUNDate[0][0];
	if((ucCount<20)||(ucCount>50))	ucCount = 20;
	#if TestFlag
		ucCount = 10;	//����ʹ��
	#endif
    while(ucCount)
    {		
        LED0 = !LED0;LED1 = !LED1;LED2 = !LED2;
        printf("%2d. ",ucCount); 
		if(Can_ReadUnregistered((uint8_t *)PhysicalADD)!=0x00)  //������
		{
            printf("�����ַ��%02X%02X%02X%02X;",PhysicalADD[0],PhysicalADD[1],PhysicalADD[2],PhysicalADD[3]);   
            u8Temp = Distribute_LogicADD((uint8_t *)PhysicalADD);	//���������ַ
            printf("�߼���ַ:%d;",u8Temp); 
            if(u8Temp<=32)
			{
				KJ_Versions[u8Temp] = Can_WriteLogitADD(u8Temp,(uint8_t *)PhysicalADD);
				if(KJ_Versions[u8Temp]==0x00)	printf("Ver[%02d]=%02X,����ʧ�ܣ�\r\n",u8Temp,KJ_Versions[u8Temp]);
				else							printf("Ver[%02d]=%02X,����ɹ���\r\n",u8Temp,KJ_Versions[u8Temp]);
			}
		}
		ucCount--;		
	}
	LED0 = 0;LED1 = 1;LED2 = 1;
	OSTimeDlyHMSM(0,0,0,200);  		
	Can_SendBroadcast_Key((uint8_t *)FM1702_Key);
	OSTimeDlyHMSM(0,0,0,200);  		
	Can_SendBroadcast_Key((uint8_t *)FM1702_Key);
	g_PowerUpFlag = 0xAA;	//��ʼ�����
    printf("\r\n�ϵ���δע�ᳬʱ��������������״̬;\r\n"); 
	IWDG_Init(6,1024);    //���Ƶ��Ϊ64,����ֵΪ1024,���ʱ��Ϊ6s	   
	while(1)
	{
		IWDG_Feed();	//���ӿ��Ź�
		if((Old_CostNum != g_CostNum)||(Old_WaterCost != g_WaterCost))
		{
			Can_SendBroadcast_Com(g_WaterCost,g_CostNum);
			if(Broadcast_Count<=2)	Broadcast_Count++;	//�ظ�����2��
			else
			{
				Broadcast_Count = 0;BroadTime=0;
				Old_CostNum = g_CostNum;	//������������ ÿ��ˮ��������
				Old_WaterCost = g_WaterCost;//WaterCost=ˮ�� ��С�ۿ��� 0.005Ԫ
			}
		}
		if(Old_FM1702KeyCRC!=FM1702KeyCRC)
		{
			Can_SendBroadcast_Key((uint8_t *)FM1702_Key);
			if(Broadcast_Count<=2)	Broadcast_Count++;	//�ظ�����2��
			else
			{
				Broadcast_Count = 0;
				Old_FM1702KeyCRC = FM1702KeyCRC;
			}
		}
		if(Led_TaskCount<BUSNUM_SIZE)
		{   //2.��ѭ��ʽȡ����
			ReadRunningData(Led_TaskCount+1);			

			if(g_RUNDate[Led_TaskCount][1] == 0xAA)	//������
			{
				g_SendDate[0] = 0xAA;	//��־Ϊʹ��
				if(g_RUNDate[Led_TaskCount][2] == 0x05)		g_SendDate[1] = 0x11;	//�忨
				else if(g_RUNDate[Led_TaskCount][2] == 0x06)g_SendDate[1] = 0x13;	//�ο�
				g_SendDate[2] = g_RUNDate[Led_TaskCount][3];	//����SN
				g_SendDate[3] = g_RUNDate[Led_TaskCount][4];	//����SN
				g_SendDate[4] = g_RUNDate[Led_TaskCount][5];	//����SN
				g_SendDate[5] = g_RUNDate[Led_TaskCount][6];	//����SN
				g_SendDate[6] = g_RUNDate[Led_TaskCount][7];	//CardSN
				g_SendDate[7] = g_RUNDate[Led_TaskCount][8];	//CardSN
				g_SendDate[8] = g_RUNDate[Led_TaskCount][9];	//CardSN
				g_SendDate[9] = g_RUNDate[Led_TaskCount][10];	//CardSN
				g_SendDate[10] = g_RUNDate[Led_TaskCount][11];	//Card���1
				g_SendDate[11] = g_RUNDate[Led_TaskCount][12];	//Card���2	
				g_SendDate[12] = g_RUNDate[Led_TaskCount][13];	//Card���3		
				g_SendDate[13] = g_RUNDate[Led_TaskCount][14];	//У��	
				g_SendDate[14] = Led_TaskCount;//�߼���ַ	
				g_SendDate[15] = g_RUNDate[Led_TaskCount][15];//ͨ����
				q_err=OSQPost(q_msg,g_SendDate);	//���Ͷ���
				if(q_err!=OS_ERR_NONE) 	myfree(SRAMIN,g_SendDate);	//����ʧ��,�ͷ��ڴ�
				g_RUNDate[Led_TaskCount][1] = 0xDD;					
			}

		}
		p=OSQPend(q_msg_ser,1,&p_err);//������Ϣ����
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
					OSTimeDlyHMSM(0,0,0,50);  //��ʱ50ms
					SendBroadcast_Com(g_WaterCost,g_CostNum);
					OSTimeDlyHMSM(0,0,0,200);  //��ʱ200ms
					SendBroadcast_Key((uint8_t *)FM1702_Key);
					OSTimeDlyHMSM(0,0,0,200);  //��ʱ200ms
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
		OSTimeDlyHMSM(0,0,0,10);  //��ʱ10ms
 	}
}
