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
#include "bsp_spi_flash.h"
#include "bsp_spi_bus.h"
//#include "common.h"
//#include "ymodem.h"

uint8_t g_RxMessage[8]={0};	//CAN��������
uint8_t g_RxMessFlag=0;		//CAN�������� ��־
uint8_t g_RUNDate[BUSNUM_SIZE+1][14]={0};    	//�������ݣ�
uint8_t KJ_Versions[BUSNUM_SIZE+1]={0};			//�����汾�Ż���
uint8_t KJ_NonResponse[BUSNUM_SIZE+1]={0};		//�����޻ظ���־ ����ֵ����20ʱ���忨��������ֵΪ5��10��15ʱ����һ��д�߼���ַ����
uint8_t g_PowerUpFlag=0;				//�ϵ��־��0xAA�ϵ����
uint8_t g_lwipADD[4]={0};				//Զ��IP��ַ
uint16_t g_lwipPort=0;					//Զ�˶˿ں�
uint8_t g_ACUAdd[16]={0};				//ͨ����16λ
uint8_t g_ACUSN[4]={0};					//���������SN 4λ

uint8_t g_SerialDat[UART3_RX_BUF_SIZE]={0};		//����ͨ������
uint16_t g_Serial_Count=0;						//���ڽ��ռ��� 

uint8_t g_CostNum = 29;					//������������ ÿ��ˮ��������
uint8_t g_WaterCost = 5;				//WaterCost=ˮ�� ��С�ۿ��� 0.005Ԫ
uint8_t FM1702KeyCRC;					//FM1702KeyCRC
uint8_t FM1702_Key[7];					//FM1702_Key[0]-[5]ΪKey;FM1702_Key[6]Ϊ���ַ��

//uint8_t g_SetIPFlag=0x00;	//0xAA:ʹ�þ�̬IP��ַ
uint8_t g_Setip[5];       	//����IP��ַ g_Setip[0] Ϊʹ�ñ�־
uint8_t g_Setnetmask[4]; 	//��������
uint8_t g_Setgateway[4]; 	//Ĭ�����ص�IP��ַ
uint8_t g_NewAddFlag=0x00; 	//������־
uint8_t g_SPI_Flash_Show=0;	//���ڲ���ʹ��
uint8_t g_IAPFlag = 0x00;	//����������־��0xAAʱ��ʾ�������������С���ʱ�����п���ͨ�š�TCPͨ��


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
	CAN_Mode_Init(CAN_SJW_1tq,CAN_BS1_8tq,CAN_BS2_7tq,18,CAN_Mode_Normal);//CAN��ʼ������ģʽ,������450Kbps 

	//usmart_dev.init(72);	//��ʼ��USMART		 
 	//FSMC_SRAM_Init();		//��ʼ���ⲿSRAM
	//my_mem_init(SRAMEX);	//��ʼ���ⲿ�ڴ��
	my_mem_init(SRAMIN);	//��ʼ���ڲ��ڴ��
    printf("Starting Up YCKJ-ACU...\r\n");
	printf("VersionNo: %02X...\r\n",VersionNo);
	printf("TestFlag: %d...\r\n",TestFlag);
	bsp_InitSPIBus();	   	//����SPI����
	bsp_InitSFlash();		//��ʼ������Flash. �ú�����ʶ����FLASH�ͺ� 
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
						//printf("\r\n �������������״̬,���ʹ���%2d,����ֵ��%d.",PrintfCount,HeartSendCount); 
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
				OSTimeDlyHMSM(0,0,0,50);  //��ʱ50ms						
			}
		}
		//����3��� ��������
		ReceiveSerialDat();
		if( ( 0x8000 & g_Serial_Count ) == 0x8000 )	//���յ�����
		{
			g_Serial_Count = 0x0000;
			SerialSetFlag=0xAA;
			printf("\r\nUART3 >> %02X%02X%02X%02X%02X%02X%02X%02X;",g_SerialDat[0],g_SerialDat[1],\
			g_SerialDat[2],g_SerialDat[3],g_SerialDat[4],g_SerialDat[5],g_SerialDat[6],g_SerialDat[7]);
			printf("\r\n���ڲ������ã��Ͽ��������ͨ�ţ�����������������ָܻ��������ͨ��;\r\n"); 
			
			tcp_client_flag = tcp_client_flag&(~LWIP_SEND_HeartbeatDATA);
			//tcp_client_flag = tcp_client_flag&(~LWIP_SEND_DATA);
			if(g_SerialDat[0]==0xF3)
			{
				SendSerialPort((uint8_t *)g_SerialDat);	//�ظ�
			}
			else if(g_SerialDat[0]==0xF1)
			{
				ReceivePacketDat((uint8_t *)g_SerialDat);//�������ݰ�
			}
		}
		
		if(g_SPI_Flash_Show==0xAA)
		{
			g_SPI_Flash_Show = 0;
			Show_FlashData(0);	//��ʾFlash���� 4K  0x0100 0000 - 0x0100 0FFF
			Show_FlashData(1);	//��ʾFlash���� 4K  0x0100 1000 - 0x0100 1FFF
			Show_FlashData(2);	//��ʾFlash���� 4K  0x0100 2000 - 0x0100 2FFF
			Show_FlashData(3);	//��ʾFlash���� 4K  0x0100 3000 - 0x0100 3FFF
			Show_FlashData(4);	//��ʾFlash���� 4K  0x0100 4000 - 0x0100 4FFF
			Show_FlashData(5);	//��ʾFlash���� 4K  0x0100 5000 - 0x0100 5FFF
		}
		OSTimeDlyHMSM(0,0,0,5);  //��ʱ5ms
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
    uint8_t ucCount = 100;	//�ϵ�100��
	uint8_t CycleCount =0;
	uint32_t BroadTime=0;
	uint8_t i;
	
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
	OSTimeDlyHMSM(0,0,0,200);  	//��ʱ200ms
	Can_SendBroadcast_Key((uint8_t *)FM1702_Key);
    OSTimeDlyHMSM(0,0,0,200);  	//��ʱ200ms
    printf("\r\nδע��㲥! \r\n");      
	Can_UnregisteredBroadcast();   //δע��㲥
	LED0 = 1;LED1 = 1;LED2 = 1;
	Old_CostNum = g_CostNum;	//������������ ÿ��ˮ��������
	Old_WaterCost = g_WaterCost;//WaterCost=ˮ�� ��С�ۿ��� 0.005Ԫ
//	ucCount = ucCount - g_RUNDate[0][0];
//	if((ucCount<20)||(ucCount>50))	ucCount = 20;
//	#if TestFlag
//		ucCount = 10;	//����ʹ��
//	#endif
    while(ucCount)
    {		
        LED0 = !LED0;LED1 = !LED1;LED2 = !LED2;
        printf("%02d. ",ucCount); 
		if(Can_ReadUnregistered((uint8_t *)PhysicalADD)!=0x00)  //������
		{
            printf("����%02X%02X%02X%02X;",PhysicalADD[0],PhysicalADD[1],PhysicalADD[2],PhysicalADD[3]);   
            u8Temp = Distribute_LogicADD((uint8_t *)PhysicalADD);	//���������ַ
            printf("�߼�:%0d;",u8Temp); 
            if( u8Temp <= ( BUSNUM_SIZE+1 ) )
			{
				KJ_Versions[u8Temp] = Can_WriteLogitADD(u8Temp,(uint8_t *)PhysicalADD);
				KJ_NonResponse[u8Temp]=0;
				if(KJ_Versions[u8Temp]==0x00)	printf("Ver[%02d]=%02X,����ʧ�ܣ�\r\n",u8Temp,KJ_Versions[u8Temp]);
				else
				{
					printf("Ver[%02d]=%02X,����ɹ���\r\n",u8Temp,KJ_Versions[u8Temp]);
					++g_RUNDate[0][0]; 				//����+1���õ�ַʹ�ã�
					g_RUNDate[u8Temp][0] = 0x80;	//����+1���õ�ַʹ�ã�
					g_RUNDate[u8Temp][1] = PhysicalADD[0]; g_RUNDate[u8Temp][2] = PhysicalADD[1];	//����SN
					g_RUNDate[u8Temp][3] = PhysicalADD[2]; g_RUNDate[u8Temp][4] = PhysicalADD[3];
				}
			}
		}
		ucCount--;
		OSTimeDlyHMSM(0, 0, 0, 5);		
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
		if(g_IAPFlag==0xD0)	//IAP������
		{
			for(i=0;i<168;i++)
			{
				IWDG_Feed();	//���ӿ��Ź�
				Send_IAPDate(i);
				OSTimeDlyHMSM(0,0,0,100);  //��ʱ10ms
			}
			SendSerialAsk(0xDD);	//����������ɻظ���UART3 USBת������
//			Send_IAPDate0(0xFC);	//�ÿ�����ת������
			g_IAPFlag = 0;
		}
		else if(g_IAPFlag==0xD1)	//����FLash
		{
			Send_IAPDate0(0xFD);
			g_IAPFlag = 0;
		}
		else if(g_IAPFlag==0xD2)	//��ʾFLash������
		{
			Send_IAPDate0(0xFB);			
			g_IAPFlag = 0;
		}
		else if(g_IAPFlag==0xD3)	//��תAPP
		{
			Send_IAPDate0(0xFC);			
			g_IAPFlag = 0;
		}
		else if(g_IAPFlag==0xD4)	//��λ����
		{
			Package_Send(0xD4,(u8 *)PhysicalADD);			
			g_IAPFlag = 0;
		}
		else
		{
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

				if((g_RUNDate[Led_TaskCount][0]&0x03) != 0x00)	//������
				{
					g_SendDate[0] = 0xAA;	//��־Ϊʹ��
					if((g_RUNDate[Led_TaskCount][0]&0x03) == 0x01)		g_SendDate[1] = 0x11;	//�忨
					else if((g_RUNDate[Led_TaskCount][0]&0x03) == 0x02)g_SendDate[1] = 0x13;	//�ο�
					g_SendDate[2] = g_RUNDate[Led_TaskCount][1];	//����SN
					g_SendDate[3] = g_RUNDate[Led_TaskCount][2];	//����SN
					g_SendDate[4] = g_RUNDate[Led_TaskCount][3];	//����SN
					g_SendDate[5] = g_RUNDate[Led_TaskCount][4];	//����SN
					g_SendDate[6] = g_RUNDate[Led_TaskCount][5];	//CardSN
					g_SendDate[7] = g_RUNDate[Led_TaskCount][6];	//CardSN
					g_SendDate[8] = g_RUNDate[Led_TaskCount][7];	//CardSN
					g_SendDate[9] = g_RUNDate[Led_TaskCount][8];	//CardSN
					g_SendDate[10] = g_RUNDate[Led_TaskCount][9];	//Card���1
					g_SendDate[11] = g_RUNDate[Led_TaskCount][10];	//Card���2	
					g_SendDate[12] = g_RUNDate[Led_TaskCount][11];	//Card���3		
					g_SendDate[13] = g_RUNDate[Led_TaskCount][12];	//У��	
					g_SendDate[14] = Led_TaskCount;//�߼���ַ	
					g_SendDate[15] = g_RUNDate[Led_TaskCount][13];//ͨ����
					q_err=OSQPost(q_msg,g_SendDate);	//���Ͷ���
					if(q_err!=OS_ERR_NONE) 	myfree(SRAMIN,g_SendDate);	//����ʧ��,�ͷ��ڴ�
					g_RUNDate[Led_TaskCount][0] = g_RUNDate[Led_TaskCount][0]&(~0x03);	//������ɣ������ݱ�־λ					
				}

			}
			p=OSQPend(q_msg_ser,1,&p_err);//������Ϣ����
			if((OS_ERR_NONE==p_err)&&(p!=NULL))
			{
				WriteRFIDData((uint8_t *)p);	
			}
			myfree(SRAMIN,p);

			if( Led_TaskCount > (BUSNUM_SIZE+1) )    
			{   
				printf("\r\n\r\n"); 	Led_TaskCount = 0;  
				if(CycleCount<250)		CycleCount++;	//Լ5���ӽ������һ��	
				else	
				{	
					CycleCount = 0;
					if( Binary_searchSN() != 0x00 )	//���߲��ҷ���1�볬ʱ����
					{
						OSTimeDlyHMSM(0,0,0,20);  //��ʱ20ms
						Can_SendBroadcast_Com(g_WaterCost,g_CostNum);
						OSTimeDlyHMSM(0,0,0,20);  //��ʱ20ms
						Can_SendBroadcast_Key((uint8_t *)FM1702_Key);
						g_NewAddFlag = 0xAA;		//�����ӱ�־�����ڱ��������ģ�鷢��������
					}					
					if(BroadTime>0x000F0000)	//
					{
						BroadTime=0;	printf("��ʱ�㲥����, ");
						Can_SendBroadcast_Com(g_WaterCost,g_CostNum);	//��ʱ�㲥ˮ��һ��
					}
					else 	{	BroadTime++;	}
				}
			}            
			else {	Led_TaskCount++;	}  
		}
		OSTimeDlyHMSM(0,0,0,10);  //��ʱ10ms
 	}
}
