#include "tcp_client_demo.h"
#include "lwip/opt.h"
#include "lwip_comm.h"
#include "lwip/lwip_sys.h"
#include "lwip/api.h"
#include "includes.h"
#include "key.h"
#include "bsp_powerbus.h"
#include "bsp_useadd.h"
#include "led.h"
#include "bsp_crc8.h"
#include "bsp_serialport.h"
#include "malloc.h"

 
struct netconn *tcp_clientconn;					//TCP CLIENT�������ӽṹ��
u8 tcp_client_recvbuf[TCP_CLIENT_RX_BUFSIZE];	//TCP�ͻ��˽������ݻ�����
u8 tcp_client_heartbeatsendbuf[32]=	//TCP�ͻ��˷������������ݻ�����
{0xF3,0x1F,0x01,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x2D,0x62,0x37,0x36,0x31,0x63,0x32,0x62,0x30,0xF1,0xF2,0xF3,0xF4,0xD1,0xD2,0xD3,0xD4,0x00,0x00,0xAA,0x00};	
u8 tcp_client_cardsendbuf[0x21]=	    //TCP�ͻ��˷��Ϳ����ݰ����ݻ�����
{0xF3,0x21,0x11,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x2D,0x62,0x37,0x36,0x31,0x63,0x32,0x62,0x30,0xD1,0xD2,0xD3,0xD4,0xC1,0xC2,0xC3,0xC4,0xD9,0x4E,0x20,0x00,0x00};	
u8 tcp_client_flag;		//TCP�ͻ������ݷ��ͱ�־λ
extern uint8_t g_RUNDate[32][17];    //�������ݣ�
extern uint8_t KJ_Versions[32];	//�����汾��

uint8_t g_LockFlag=0;
	
extern uint8_t g_lwipADD[4];	//Զ��IP��ַ
extern uint16_t g_lwipPort;		//Զ�˶˿ں�
extern uint8_t g_ACUSN[4];		//���������SN 4λ
extern uint8_t g_ACUAdd[16];	//ͨ����16λ

extern uint8_t g_CostNum;		//������������ ÿ��ˮ��������
extern uint8_t g_WaterCost;		//WaterCost=ˮ�� ��С�ۿ��� 0.005Ԫ
extern uint8_t FM1702KeyCRC;
extern uint8_t FM1702_Key[7];		//FM1702_Key[0]-[5]ΪKey;FM1702_Key[6]Ϊ���ַ��
extern OS_EVENT * q_msg;			//��Ϣ����
extern OS_EVENT * q_msg_ser;		//��Ϣ���� �������ظ�����


//TCP�ͻ�������
#define TCPCLIENT_PRIO		8
//�����ջ��С
#define TCPCLIENT_STK_SIZE	350
//�����ջ
OS_STK TCPCLIENT_TASK_STK[TCPCLIENT_STK_SIZE];

//tcp�ͻ���������
static void tcp_client_thread(void *arg)
{
	OS_CPU_SR cpu_sr;
	u8 _ucNo=0,j;
	u32 data_len = 0;
	struct pbuf *q;
	err_t err,recv_err;
	static ip_addr_t server_ipaddr,loca_ipaddr;
	static u16_t 		 server_port,loca_port;
    uint8_t i_SendCount=0;	//i_SendCount�������ݰ����ͼ���
	uint8_t SendRecvCount=0;	//���ͼ�1�����ռ�1������ֵ����10��(10�η���δ�յ��ظ�)ʱ���Ͽ�TCP��������

	u8 *p;
	u8 p_err;

	u8* ser_Date;	 
	u8 ser_err; 
	ser_Date=mymalloc(SRAMIN,11);	//����11���ֽڵ��ڴ�
		
	LWIP_UNUSED_ARG(arg);
//	server_port = REMOTE_PORT;	//����ʹ��
	server_port = g_lwipPort;	//Զ�˶˿ں�
	IP4_ADDR(&server_ipaddr, lwipdev.remoteip[0],lwipdev.remoteip[1], lwipdev.remoteip[2],lwipdev.remoteip[3]);

	for(j=0;j<16;j++)
	{
		tcp_client_heartbeatsendbuf[3+j] = g_ACUAdd[j];
		tcp_client_cardsendbuf[3+j] = g_ACUAdd[j];
	}
	tcp_client_heartbeatsendbuf[19] = g_ACUSN[0];	tcp_client_heartbeatsendbuf[20] = g_ACUSN[1];
	tcp_client_heartbeatsendbuf[21] = g_ACUSN[2];	tcp_client_heartbeatsendbuf[22] = g_ACUSN[3];
	
	while (1) 
	{
		tcp_clientconn = netconn_new(NETCONN_TCP);  //����һ��TCP����
		err = netconn_connect(tcp_clientconn,&server_ipaddr,server_port);//���ӷ�����
		printf("����һ��TCP����,���ӷ����� %d;\r\n",err);
		if(err != ERR_OK)  netconn_delete(tcp_clientconn); //����ֵ������ERR_OK,ɾ��tcp_clientconn����
		else if (err == ERR_OK)    //���������ӵ�����
		{ 
			struct netbuf *recvbuf;
			tcp_clientconn->recv_timeout = 10;
			netconn_getaddr(tcp_clientconn,&loca_ipaddr,&loca_port,1); //��ȡ����IP����IP��ַ�Ͷ˿ں�
			printf("�����Ϸ�����%d.%d.%d.%d,�����˿ں�Ϊ:%d\r\n",lwipdev.remoteip[0],lwipdev.remoteip[1], lwipdev.remoteip[2],lwipdev.remoteip[3],loca_port);
			SendRecvCount = 0;
			while(1)
			{
				if((tcp_client_flag & LWIP_SEND_HeartbeatDATA) == LWIP_SEND_HeartbeatDATA) //���������ݰ�Ҫ����
                {
					LED2 = 1;
					if(i_SendCount>=BUSNUM_SIZE)	i_SendCount = 1;
					else							i_SendCount++;
					printf("\r\n��������%2d\r\n",i_SendCount);
					if(g_RUNDate[i_SendCount][0] != 0x00)
					{
						if(SendRecvCount<200)	SendRecvCount++;
						tcp_client_heartbeatsendbuf[23] = g_RUNDate[i_SendCount][3]; tcp_client_heartbeatsendbuf[24] = g_RUNDate[i_SendCount][4];   //����SN
						tcp_client_heartbeatsendbuf[25] = g_RUNDate[i_SendCount][5]; tcp_client_heartbeatsendbuf[26] = g_RUNDate[i_SendCount][6];   //����SN
						tcp_client_heartbeatsendbuf[27] = VersionNo;	//����̼��汾��
						tcp_client_heartbeatsendbuf[28] = KJ_Versions[i_SendCount];	//�����̼��汾��
						if(g_RUNDate[i_SendCount][1] != 0x00)	tcp_client_heartbeatsendbuf[29] = 0xAA;	//����
						else            						tcp_client_heartbeatsendbuf[29] = 0x00;	//ʧ��
						tcp_client_heartbeatsendbuf[30] = i_SendCount;	//�����߼���ַ
                        err = netconn_write(tcp_clientconn ,tcp_client_heartbeatsendbuf,tcp_client_heartbeatsendbuf[1],NETCONN_COPY); //����tcp_server_sentbuf�е�����
						if(err != ERR_OK)		printf("����ʧ��\r\n");					
					}
//					else
//					{
//						tcp_client_heartbeatsendbuf[30] = i_SendCount;	//�����߼���ַ
//                        err = netconn_write(tcp_clientconn ,tcp_client_heartbeatsendbuf,tcp_client_heartbeatsendbuf[1],NETCONN_COPY); //����tcp_server_sentbuf�е�����
//						if(err != ERR_OK)		printf("����ʧ��\r\n");										
//					}
					tcp_client_flag &= ~LWIP_SEND_HeartbeatDATA;	//������ɣ����־λ
				}
				{
					p=OSQPend(q_msg,100,&p_err);//������Ϣ����
					if((OS_ERR_NONE==p_err)&&(p!=NULL))
					{
						tcp_client_cardsendbuf[2] = p[1];	//0x11Ϊ�忨���ݣ�0x13Ϊ�ο�����
						tcp_client_cardsendbuf[19] = p[2];	tcp_client_cardsendbuf[20] = p[3];	//����SN
						tcp_client_cardsendbuf[21] = p[4];	tcp_client_cardsendbuf[22] = p[5];	//����SN
						tcp_client_cardsendbuf[23] = p[6];	tcp_client_cardsendbuf[24] = p[7];	//CardSN
						tcp_client_cardsendbuf[25] = p[8];	tcp_client_cardsendbuf[26] = p[9];	//CardSN
						tcp_client_cardsendbuf[27] = p[13];	//Card У��
						tcp_client_cardsendbuf[28] = p[10];	tcp_client_cardsendbuf[29] = p[11];	//�����
						tcp_client_cardsendbuf[30] = p[12];	//�����
						tcp_client_cardsendbuf[31] = p[14];	//�����߼���ַ
						tcp_client_cardsendbuf[32] = p[15];	//ͨ����
						err = netconn_write(tcp_clientconn ,tcp_client_cardsendbuf,tcp_client_cardsendbuf[1],NETCONN_COPY); //����tcp_server_sentbuf�е�����
						if(err != ERR_OK)		printf("����ʧ��\r\n");
					}
					myfree(SRAMIN,p);	  
				}
					
				if((recv_err = netconn_recv(tcp_clientconn,&recvbuf)) == ERR_OK)  //���յ�����
				{
					LED2 = 0;
					OS_ENTER_CRITICAL(); //���ж�
					memset(tcp_client_recvbuf,0,TCP_CLIENT_RX_BUFSIZE);  //���ݽ��ջ���������
					for(q=recvbuf->p;q!=NULL;q=q->next)  //����������pbuf����
					{
						//�ж�Ҫ������TCP_CLIENT_RX_BUFSIZE�е������Ƿ����TCP_CLIENT_RX_BUFSIZE��ʣ��ռ䣬�������
						//�Ļ���ֻ����TCP_CLIENT_RX_BUFSIZE��ʣ�೤�ȵ����ݣ�����Ļ��Ϳ������е�����
						if(q->len > (TCP_CLIENT_RX_BUFSIZE-data_len)) memcpy(tcp_client_recvbuf+data_len,q->payload,(TCP_CLIENT_RX_BUFSIZE-data_len));//��������
						else memcpy(tcp_client_recvbuf+data_len,q->payload,q->len);
						data_len += q->len;  	
						if(data_len > TCP_CLIENT_RX_BUFSIZE) break; //����TCP�ͻ��˽�������,����	
					}
					OS_EXIT_CRITICAL();  //���ж�
					data_len=0;  //������ɺ�data_lenҪ���㡣					
					printf("\r\nTCP_Recv<<%02X%02X%02X%02X%02X",tcp_client_recvbuf[0],tcp_client_recvbuf[1],tcp_client_recvbuf[2],tcp_client_recvbuf[3],tcp_client_recvbuf[4]);
					printf("%02X%02X%02X%02X%02X",tcp_client_recvbuf[5],tcp_client_recvbuf[6],tcp_client_recvbuf[7],tcp_client_recvbuf[8],tcp_client_recvbuf[9]);
					printf("%02X%02X%02X%02X%02X",tcp_client_recvbuf[10],tcp_client_recvbuf[11],tcp_client_recvbuf[12],tcp_client_recvbuf[13],tcp_client_recvbuf[14]);
					printf("%02X%02X%02X%02X%02X\r\n",tcp_client_recvbuf[15],tcp_client_recvbuf[16],tcp_client_recvbuf[17],tcp_client_recvbuf[18],tcp_client_recvbuf[19]);
					if(tcp_client_recvbuf[0]==0xF3)
					{
						if(SendRecvCount>0)	SendRecvCount--;
						if(tcp_client_recvbuf[3]==0x00)	//�쳣�ظ�
						{
							if(tcp_client_recvbuf[2]==0x02)
							{
								_ucNo = tcp_client_recvbuf[tcp_client_recvbuf[1]-1];	//��ȡ�߼���ַ 
								g_LockFlag = _ucNo;
								g_RUNDate[_ucNo][0] = 0x55;
								g_RUNDate[_ucNo][1] = tcp_client_recvbuf[2];	//�������ظ���ʾ 0x02
								g_RUNDate[_ucNo][7] = tcp_client_recvbuf[4];	//"E" 0x45
								g_RUNDate[_ucNo][8] = tcp_client_recvbuf[5];
								g_RUNDate[_ucNo][9] = tcp_client_recvbuf[6];
								g_RUNDate[_ucNo][10] = tcp_client_recvbuf[7];
								g_RUNDate[_ucNo][15] = 0;// ��ͨ����
								g_LockFlag = 0x00;
							}								
							else if((tcp_client_recvbuf[2]==0x12)&&(tcp_client_recvbuf[1]==0x12))//�忨���ݰ�
							{
								_ucNo = tcp_client_recvbuf[tcp_client_recvbuf[1]-2];	//��ȡ�߼���ַ ������δβ����ͨ���룬�����߼���ַ�ڵ����ڶ�λ								
								g_LockFlag = _ucNo;
								g_RUNDate[_ucNo][0] = 0xAA;
								g_LockFlag = 0x00;
								printf("�忨���ݰ� ���ţ�%02X%02X%02X%02X  ",tcp_client_recvbuf[8],tcp_client_recvbuf[9],tcp_client_recvbuf[10],tcp_client_recvbuf[11]);
								printf("�߼���ַ��%02d; ", tcp_client_recvbuf[16]);
								printf("������룺%c%c%c%c; ", tcp_client_recvbuf[4],tcp_client_recvbuf[5],tcp_client_recvbuf[6],tcp_client_recvbuf[7]);
								printf("ͨ����%02X \r\n",tcp_client_recvbuf[17]);
								ser_Date[0] = 0xBB;	//��־������ �쳣
								ser_Date[1] = tcp_client_recvbuf[16];	//�߼���ַ
								ser_Date[2] = tcp_client_recvbuf[4];	//�������
								ser_Date[3] = tcp_client_recvbuf[5];	//�������
								ser_Date[4] = tcp_client_recvbuf[6];	//�������
								ser_Date[5] = tcp_client_recvbuf[7];	//�������
								ser_Date[6] = tcp_client_recvbuf[17];	//ͨ����
								ser_Date[9] = _ucNo;	//�߼���ַ
								ser_err=OSQPost(q_msg_ser,ser_Date);	//���Ͷ���
								if(ser_err!=OS_ERR_NONE) 	myfree(SRAMIN,ser_Date);	//����ʧ��,�ͷ��ڴ�							
							}
						}
						else if(tcp_client_recvbuf[3]==0xAA)	//�����ظ�
						{
							if(tcp_client_recvbuf[2]==0x02)
								_ucNo = tcp_client_recvbuf[tcp_client_recvbuf[1]-1];	//��ȡ�߼���ַ 								
							else
								_ucNo = tcp_client_recvbuf[tcp_client_recvbuf[1]-2];	//��ȡ�߼���ַ ������δβ����ͨ���룬�����߼���ַ�ڵ����ڶ�λ								
							g_LockFlag = _ucNo;
							g_RUNDate[_ucNo][0] = 0xAA;
							g_LockFlag = 0x00;
							if((tcp_client_recvbuf[1]==0x0C)&&(tcp_client_recvbuf[2]==0x02)&&(tcp_client_recvbuf[4]==0xAA))//������
							{
								g_CostNum = tcp_client_recvbuf[10];	//������������ ÿ��ˮ��������
								g_WaterCost = tcp_client_recvbuf[9];//WaterCost=ˮ�� ��С�ۿ��� 0.005Ԫ
							}
							else if((tcp_client_recvbuf[2]==0x12)&&(tcp_client_recvbuf[1]==0x12))//�忨���ݰ�
							{
								printf("�忨���ݰ� ���ţ�%02X%02X%02X%02X",tcp_client_recvbuf[8],tcp_client_recvbuf[9],tcp_client_recvbuf[10],tcp_client_recvbuf[11]);
								printf("��%6d;   ", (tcp_client_recvbuf[13]<<16)|(tcp_client_recvbuf[14]<<8)|tcp_client_recvbuf[15]);
								printf("ͨ����%02X \r\n",tcp_client_recvbuf[17]);
								ser_Date[0] = 0xAA;	//��־������
								ser_Date[1] = tcp_client_recvbuf[8];	//��SN 1
								ser_Date[2] = tcp_client_recvbuf[9];	//��SN 2
								ser_Date[3] = tcp_client_recvbuf[10];	//��SN 3
								ser_Date[4] = tcp_client_recvbuf[11];	//��SN 4
								ser_Date[5] = tcp_client_recvbuf[12];	//����У������
								ser_Date[6] = tcp_client_recvbuf[13];	//���ڽ��1 ��λ
								ser_Date[7] = tcp_client_recvbuf[14];	//���ڽ��2 ��λ
								ser_Date[8] = tcp_client_recvbuf[15];	//���ڽ��3 ��λ
								ser_Date[9] = _ucNo;	//�߼���ַ
								ser_Date[10] = tcp_client_recvbuf[17];	//ͨ����
								//ser_Date[9] = 0x00;	//��־������ ����
								ser_err=OSQPost(q_msg_ser,ser_Date);	//���Ͷ���
								if(ser_err!=OS_ERR_NONE) 	myfree(SRAMIN,ser_Date);	//����ʧ��,�ͷ��ڴ�							
							}

						}
					}
					netbuf_delete(recvbuf);
				
				}
                else if(recv_err == ERR_CLSD)  //�ر�����
				{
					netconn_close(tcp_clientconn);
					netconn_delete(tcp_clientconn);
					printf("\r\n������%d.%d.%d.%d�Ͽ�����\r\n",lwipdev.remoteip[0],lwipdev.remoteip[1], lwipdev.remoteip[2],lwipdev.remoteip[3]);
					break;
				}
				else
				{
					if(SendRecvCount>20)
					{
						SendRecvCount = 0;
						netconn_close(tcp_clientconn);
						netconn_delete(tcp_clientconn);
						printf("\r\n-----������%d.%d.%d.%d�Ͽ�����\r\n",lwipdev.remoteip[0],lwipdev.remoteip[1], lwipdev.remoteip[2],lwipdev.remoteip[3]);
						break;
					}
				}

			}
		}
	}
}

//����TCP�ͻ����߳�
//����ֵ:0 TCP�ͻ��˴����ɹ�
//		���� TCP�ͻ��˴���ʧ��
INT8U tcp_client_init(void)
{
	INT8U res;
	OS_CPU_SR cpu_sr;
	
	OS_ENTER_CRITICAL();	//���ж�
	res = OSTaskCreate(tcp_client_thread,(void*)0,(OS_STK*)&TCPCLIENT_TASK_STK[TCPCLIENT_STK_SIZE-1],TCPCLIENT_PRIO); //����TCP�ͻ����߳�
	OS_EXIT_CRITICAL();		//���ж�
	
	return res;
}

