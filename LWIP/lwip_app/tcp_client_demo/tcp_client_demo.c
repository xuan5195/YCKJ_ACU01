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

 
struct netconn *tcp_clientconn;					//TCP CLIENT网络连接结构体
u8 tcp_client_recvbuf[TCP_CLIENT_RX_BUFSIZE];	//TCP客户端接收数据缓冲区
u8 tcp_client_heartbeatsendbuf[32]=	//TCP客户端发送心跳包数据缓冲区
{0xF3,0x1F,0x01,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x2D,0x62,0x37,0x36,0x31,0x63,0x32,0x62,0x30,0xF1,0xF2,0xF3,0xF4,0xD1,0xD2,0xD3,0xD4,0x00,0x00,0xAA,0x00};	
u8 tcp_client_cardsendbuf[0x21]=	    //TCP客户端发送卡数据包数据缓冲区
{0xF3,0x21,0x11,0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x2D,0x62,0x37,0x36,0x31,0x63,0x32,0x62,0x30,0xD1,0xD2,0xD3,0xD4,0xC1,0xC2,0xC3,0xC4,0xD9,0x4E,0x20,0x00,0x00};	
u8 tcp_client_flag;		//TCP客户端数据发送标志位
extern uint8_t g_RUNDate[32][17];    //运行数据；
extern uint8_t KJ_Versions[32];	//卡机版本号

uint8_t g_LockFlag=0;
	
extern uint8_t g_lwipADD[4];	//远端IP地址
extern uint16_t g_lwipPort;		//远端端口号
extern uint8_t g_ACUSN[4];		//区域控制器SN 4位
extern uint8_t g_ACUAdd[16];	//通信码16位

extern uint8_t g_CostNum;		//流量计脉冲数 每升水计量周期
extern uint8_t g_WaterCost;		//WaterCost=水费 最小扣款金额 0.005元
extern uint8_t FM1702KeyCRC;
extern uint8_t FM1702_Key[7];		//FM1702_Key[0]-[5]为Key;FM1702_Key[6]为块地址；
extern OS_EVENT * q_msg;			//消息队列
extern OS_EVENT * q_msg_ser;		//消息队列 服务器回复数据


//TCP客户端任务
#define TCPCLIENT_PRIO		8
//任务堆栈大小
#define TCPCLIENT_STK_SIZE	350
//任务堆栈
OS_STK TCPCLIENT_TASK_STK[TCPCLIENT_STK_SIZE];

//tcp客户端任务函数
static void tcp_client_thread(void *arg)
{
	OS_CPU_SR cpu_sr;
	u8 _ucNo=0,j;
	u32 data_len = 0;
	struct pbuf *q;
	err_t err,recv_err;
	static ip_addr_t server_ipaddr,loca_ipaddr;
	static u16_t 		 server_port,loca_port;
    uint8_t i_SendCount=0;	//i_SendCount心跳数据包发送计数
	uint8_t SendRecvCount=0;	//发送加1，接收减1，当该值大于10次(10次发送未收到回复)时，断开TCP链接重连

	u8 *p;
	u8 p_err;

	u8* ser_Date;	 
	u8 ser_err; 
	ser_Date=mymalloc(SRAMIN,11);	//申请11个字节的内存
		
	LWIP_UNUSED_ARG(arg);
//	server_port = REMOTE_PORT;	//测试使用
	server_port = g_lwipPort;	//远端端口号
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
		tcp_clientconn = netconn_new(NETCONN_TCP);  //创建一个TCP链接
		err = netconn_connect(tcp_clientconn,&server_ipaddr,server_port);//连接服务器
		printf("创建一个TCP链接,连接服务器 %d;\r\n",err);
		if(err != ERR_OK)  netconn_delete(tcp_clientconn); //返回值不等于ERR_OK,删除tcp_clientconn连接
		else if (err == ERR_OK)    //处理新连接的数据
		{ 
			struct netbuf *recvbuf;
			tcp_clientconn->recv_timeout = 10;
			netconn_getaddr(tcp_clientconn,&loca_ipaddr,&loca_port,1); //获取本地IP主机IP地址和端口号
			printf("连接上服务器%d.%d.%d.%d,本机端口号为:%d\r\n",lwipdev.remoteip[0],lwipdev.remoteip[1], lwipdev.remoteip[2],lwipdev.remoteip[3],loca_port);
			SendRecvCount = 0;
			while(1)
			{
				if((tcp_client_flag & LWIP_SEND_HeartbeatDATA) == LWIP_SEND_HeartbeatDATA) //有心跳数据包要发送
                {
					LED2 = 1;
					if(i_SendCount>=BUSNUM_SIZE)	i_SendCount = 1;
					else							i_SendCount++;
					printf("\r\n心跳包：%2d\r\n",i_SendCount);
					if(g_RUNDate[i_SendCount][0] != 0x00)
					{
						if(SendRecvCount<200)	SendRecvCount++;
						tcp_client_heartbeatsendbuf[23] = g_RUNDate[i_SendCount][3]; tcp_client_heartbeatsendbuf[24] = g_RUNDate[i_SendCount][4];   //卡机SN
						tcp_client_heartbeatsendbuf[25] = g_RUNDate[i_SendCount][5]; tcp_client_heartbeatsendbuf[26] = g_RUNDate[i_SendCount][6];   //卡机SN
						tcp_client_heartbeatsendbuf[27] = VersionNo;	//区域固件版本号
						tcp_client_heartbeatsendbuf[28] = KJ_Versions[i_SendCount];	//卡机固件版本号
						if(g_RUNDate[i_SendCount][1] != 0x00)	tcp_client_heartbeatsendbuf[29] = 0xAA;	//正常
						else            						tcp_client_heartbeatsendbuf[29] = 0x00;	//失联
						tcp_client_heartbeatsendbuf[30] = i_SendCount;	//卡机逻辑地址
                        err = netconn_write(tcp_clientconn ,tcp_client_heartbeatsendbuf,tcp_client_heartbeatsendbuf[1],NETCONN_COPY); //发送tcp_server_sentbuf中的数据
						if(err != ERR_OK)		printf("发送失败\r\n");					
					}
//					else
//					{
//						tcp_client_heartbeatsendbuf[30] = i_SendCount;	//卡机逻辑地址
//                        err = netconn_write(tcp_clientconn ,tcp_client_heartbeatsendbuf,tcp_client_heartbeatsendbuf[1],NETCONN_COPY); //发送tcp_server_sentbuf中的数据
//						if(err != ERR_OK)		printf("发送失败\r\n");										
//					}
					tcp_client_flag &= ~LWIP_SEND_HeartbeatDATA;	//发送完成，清标志位
				}
				{
					p=OSQPend(q_msg,100,&p_err);//请求消息队列
					if((OS_ERR_NONE==p_err)&&(p!=NULL))
					{
						tcp_client_cardsendbuf[2] = p[1];	//0x11为插卡数据；0x13为拔卡数据
						tcp_client_cardsendbuf[19] = p[2];	tcp_client_cardsendbuf[20] = p[3];	//卡机SN
						tcp_client_cardsendbuf[21] = p[4];	tcp_client_cardsendbuf[22] = p[5];	//卡机SN
						tcp_client_cardsendbuf[23] = p[6];	tcp_client_cardsendbuf[24] = p[7];	//CardSN
						tcp_client_cardsendbuf[25] = p[8];	tcp_client_cardsendbuf[26] = p[9];	//CardSN
						tcp_client_cardsendbuf[27] = p[13];	//Card 校验
						tcp_client_cardsendbuf[28] = p[10];	tcp_client_cardsendbuf[29] = p[11];	//卡金额
						tcp_client_cardsendbuf[30] = p[12];	//卡金额
						tcp_client_cardsendbuf[31] = p[14];	//卡机逻辑地址
						tcp_client_cardsendbuf[32] = p[15];	//通信码
						err = netconn_write(tcp_clientconn ,tcp_client_cardsendbuf,tcp_client_cardsendbuf[1],NETCONN_COPY); //发送tcp_server_sentbuf中的数据
						if(err != ERR_OK)		printf("发送失败\r\n");
					}
					myfree(SRAMIN,p);	  
				}
					
				if((recv_err = netconn_recv(tcp_clientconn,&recvbuf)) == ERR_OK)  //接收到数据
				{
					LED2 = 0;
					OS_ENTER_CRITICAL(); //关中断
					memset(tcp_client_recvbuf,0,TCP_CLIENT_RX_BUFSIZE);  //数据接收缓冲区清零
					for(q=recvbuf->p;q!=NULL;q=q->next)  //遍历完整个pbuf链表
					{
						//判断要拷贝到TCP_CLIENT_RX_BUFSIZE中的数据是否大于TCP_CLIENT_RX_BUFSIZE的剩余空间，如果大于
						//的话就只拷贝TCP_CLIENT_RX_BUFSIZE中剩余长度的数据，否则的话就拷贝所有的数据
						if(q->len > (TCP_CLIENT_RX_BUFSIZE-data_len)) memcpy(tcp_client_recvbuf+data_len,q->payload,(TCP_CLIENT_RX_BUFSIZE-data_len));//拷贝数据
						else memcpy(tcp_client_recvbuf+data_len,q->payload,q->len);
						data_len += q->len;  	
						if(data_len > TCP_CLIENT_RX_BUFSIZE) break; //超出TCP客户端接收数组,跳出	
					}
					OS_EXIT_CRITICAL();  //开中断
					data_len=0;  //复制完成后data_len要清零。					
					printf("\r\nTCP_Recv<<%02X%02X%02X%02X%02X",tcp_client_recvbuf[0],tcp_client_recvbuf[1],tcp_client_recvbuf[2],tcp_client_recvbuf[3],tcp_client_recvbuf[4]);
					printf("%02X%02X%02X%02X%02X",tcp_client_recvbuf[5],tcp_client_recvbuf[6],tcp_client_recvbuf[7],tcp_client_recvbuf[8],tcp_client_recvbuf[9]);
					printf("%02X%02X%02X%02X%02X",tcp_client_recvbuf[10],tcp_client_recvbuf[11],tcp_client_recvbuf[12],tcp_client_recvbuf[13],tcp_client_recvbuf[14]);
					printf("%02X%02X%02X%02X%02X\r\n",tcp_client_recvbuf[15],tcp_client_recvbuf[16],tcp_client_recvbuf[17],tcp_client_recvbuf[18],tcp_client_recvbuf[19]);
					if(tcp_client_recvbuf[0]==0xF3)
					{
						if(SendRecvCount>0)	SendRecvCount--;
						if(tcp_client_recvbuf[3]==0x00)	//异常回复
						{
							if(tcp_client_recvbuf[2]==0x02)
							{
								_ucNo = tcp_client_recvbuf[tcp_client_recvbuf[1]-1];	//读取逻辑地址 
								g_LockFlag = _ucNo;
								g_RUNDate[_ucNo][0] = 0x55;
								g_RUNDate[_ucNo][1] = tcp_client_recvbuf[2];	//心跳包回复标示 0x02
								g_RUNDate[_ucNo][7] = tcp_client_recvbuf[4];	//"E" 0x45
								g_RUNDate[_ucNo][8] = tcp_client_recvbuf[5];
								g_RUNDate[_ucNo][9] = tcp_client_recvbuf[6];
								g_RUNDate[_ucNo][10] = tcp_client_recvbuf[7];
								g_RUNDate[_ucNo][15] = 0;// 无通信码
								g_LockFlag = 0x00;
							}								
							else if((tcp_client_recvbuf[2]==0x12)&&(tcp_client_recvbuf[1]==0x12))//插卡数据包
							{
								_ucNo = tcp_client_recvbuf[tcp_client_recvbuf[1]-2];	//读取逻辑地址 由于在未尾增加通信码，所以逻辑地址在倒数第二位								
								g_LockFlag = _ucNo;
								g_RUNDate[_ucNo][0] = 0xAA;
								g_LockFlag = 0x00;
								printf("插卡数据包 卡号：%02X%02X%02X%02X  ",tcp_client_recvbuf[8],tcp_client_recvbuf[9],tcp_client_recvbuf[10],tcp_client_recvbuf[11]);
								printf("逻辑地址：%02d; ", tcp_client_recvbuf[16]);
								printf("错误代码：%c%c%c%c; ", tcp_client_recvbuf[4],tcp_client_recvbuf[5],tcp_client_recvbuf[6],tcp_client_recvbuf[7]);
								printf("通信码%02X \r\n",tcp_client_recvbuf[17]);
								ser_Date[0] = 0xBB;	//标志有数据 异常
								ser_Date[1] = tcp_client_recvbuf[16];	//逻辑地址
								ser_Date[2] = tcp_client_recvbuf[4];	//错误代码
								ser_Date[3] = tcp_client_recvbuf[5];	//错误代码
								ser_Date[4] = tcp_client_recvbuf[6];	//错误代码
								ser_Date[5] = tcp_client_recvbuf[7];	//错误代码
								ser_Date[6] = tcp_client_recvbuf[17];	//通信码
								ser_Date[9] = _ucNo;	//逻辑地址
								ser_err=OSQPost(q_msg_ser,ser_Date);	//发送队列
								if(ser_err!=OS_ERR_NONE) 	myfree(SRAMIN,ser_Date);	//发送失败,释放内存							
							}
						}
						else if(tcp_client_recvbuf[3]==0xAA)	//正常回复
						{
							if(tcp_client_recvbuf[2]==0x02)
								_ucNo = tcp_client_recvbuf[tcp_client_recvbuf[1]-1];	//读取逻辑地址 								
							else
								_ucNo = tcp_client_recvbuf[tcp_client_recvbuf[1]-2];	//读取逻辑地址 由于在未尾增加通信码，所以逻辑地址在倒数第二位								
							g_LockFlag = _ucNo;
							g_RUNDate[_ucNo][0] = 0xAA;
							g_LockFlag = 0x00;
							if((tcp_client_recvbuf[1]==0x0C)&&(tcp_client_recvbuf[2]==0x02)&&(tcp_client_recvbuf[4]==0xAA))//心跳包
							{
								g_CostNum = tcp_client_recvbuf[10];	//流量计脉冲数 每升水计量周期
								g_WaterCost = tcp_client_recvbuf[9];//WaterCost=水费 最小扣款金额 0.005元
							}
							else if((tcp_client_recvbuf[2]==0x12)&&(tcp_client_recvbuf[1]==0x12))//插卡数据包
							{
								printf("插卡数据包 卡号：%02X%02X%02X%02X",tcp_client_recvbuf[8],tcp_client_recvbuf[9],tcp_client_recvbuf[10],tcp_client_recvbuf[11]);
								printf("金额：%6d;   ", (tcp_client_recvbuf[13]<<16)|(tcp_client_recvbuf[14]<<8)|tcp_client_recvbuf[15]);
								printf("通信码%02X \r\n",tcp_client_recvbuf[17]);
								ser_Date[0] = 0xAA;	//标志有数据
								ser_Date[1] = tcp_client_recvbuf[8];	//卡SN 1
								ser_Date[2] = tcp_client_recvbuf[9];	//卡SN 2
								ser_Date[3] = tcp_client_recvbuf[10];	//卡SN 3
								ser_Date[4] = tcp_client_recvbuf[11];	//卡SN 4
								ser_Date[5] = tcp_client_recvbuf[12];	//卡内校验数据
								ser_Date[6] = tcp_client_recvbuf[13];	//卡内金额1 高位
								ser_Date[7] = tcp_client_recvbuf[14];	//卡内金额2 高位
								ser_Date[8] = tcp_client_recvbuf[15];	//卡内金额3 高位
								ser_Date[9] = _ucNo;	//逻辑地址
								ser_Date[10] = tcp_client_recvbuf[17];	//通信码
								//ser_Date[9] = 0x00;	//标志数据锁 解锁
								ser_err=OSQPost(q_msg_ser,ser_Date);	//发送队列
								if(ser_err!=OS_ERR_NONE) 	myfree(SRAMIN,ser_Date);	//发送失败,释放内存							
							}

						}
					}
					netbuf_delete(recvbuf);
				
				}
                else if(recv_err == ERR_CLSD)  //关闭连接
				{
					netconn_close(tcp_clientconn);
					netconn_delete(tcp_clientconn);
					printf("\r\n服务器%d.%d.%d.%d断开连接\r\n",lwipdev.remoteip[0],lwipdev.remoteip[1], lwipdev.remoteip[2],lwipdev.remoteip[3]);
					break;
				}
				else
				{
					if(SendRecvCount>20)
					{
						SendRecvCount = 0;
						netconn_close(tcp_clientconn);
						netconn_delete(tcp_clientconn);
						printf("\r\n-----服务器%d.%d.%d.%d断开连接\r\n",lwipdev.remoteip[0],lwipdev.remoteip[1], lwipdev.remoteip[2],lwipdev.remoteip[3]);
						break;
					}
				}

			}
		}
	}
}

//创建TCP客户端线程
//返回值:0 TCP客户端创建成功
//		其他 TCP客户端创建失败
INT8U tcp_client_init(void)
{
	INT8U res;
	OS_CPU_SR cpu_sr;
	
	OS_ENTER_CRITICAL();	//关中断
	res = OSTaskCreate(tcp_client_thread,(void*)0,(OS_STK*)&TCPCLIENT_TASK_STK[TCPCLIENT_STK_SIZE-1],TCPCLIENT_PRIO); //创建TCP客户端线程
	OS_EXIT_CRITICAL();		//开中断
	
	return res;
}

