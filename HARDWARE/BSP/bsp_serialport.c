#include "bsp_serialport.h"
#include "usart.h"//#include "bsp_uart_fifo.h"
#include "bsp_crc8.h"
#include "bsp_useadd.h"

#include "delay.h"
#include "sys.h"
#include "string.h"
//#include "usmart.h"	
#include "includes.h"
#include "led.h"



extern uint8_t FM1702KeyCRC;
extern uint8_t FM1702_Key[7];	//FM1702_Key[0]-[5]为Key;FM1702_Key[6]为块地址；
extern uint8_t g_ACUSN[4];		//区域控制器SN 4位

//extern uint8_t g_SetIPFlag;			//0x00:使用静态IP地址
extern uint8_t g_Setip[5];       	//本机IP地址
extern uint8_t g_Setnetmask[4]; 	//子网掩码
extern uint8_t g_Setgateway[4]; 	//默认网关的IP地址


void SendSerialPort(uint8_t *SerialDat)
{
	uint8_t Runningbuf[20]={0x00};
	uint8_t Sendbuf[20]={0x00};
	uint8_t RFID_Key[7]={0x00};
	uint8_t _lwipADD[4]={0x00};
	uint8_t _ACUAdd[16]={0x00};
	uint8_t _Setip[5];       	//本机IP地址 g_Setip[0] 为使用标志
	uint8_t _Setnetmask[4]; 	//子网掩码
	uint8_t _Setgateway[4]; 	//默认网关的IP地址
	uint16_t _lwipPort=0;
	uint8_t i=0;
	Runningbuf[0] = 0xF4;
	Runningbuf[1] = SerialDat[1];	//长度
	Runningbuf[2] = SerialDat[2]+1;	//命令帧
	switch(SerialDat[2])
	{
		case 0x01:	//通信检测
			Runningbuf[3] = VersionNo;	//数据域
			Runningbuf[4] = g_ACUSN[0];	//数据域 区域控制器SN
			Runningbuf[5] = g_ACUSN[1];	//数据域 区域控制器SN
			Runningbuf[6] = g_ACUSN[2];	//数据域 区域控制器SN
			Runningbuf[7] = g_ACUSN[3];	//数据域 区域控制器SN
			break;
		case 0x03:	//IP地址和端口号
			Runningbuf[3] = SerialDat[3];	//数据域
			Runningbuf[4] = SerialDat[4];	//数据域
			Runningbuf[5] = SerialDat[5];	//数据域
			Runningbuf[6] = SerialDat[6];	//数据域
			Runningbuf[7] = SerialDat[7];	//数据域
			Runningbuf[8] = SerialDat[8];	//数据域
			Sendbuf[0] = Runningbuf[3];
			Sendbuf[1] = Runningbuf[4];
			Sendbuf[2] = Runningbuf[5];
			Sendbuf[3] = Runningbuf[6];			
			Write_IPAdd((uint8_t *)Sendbuf);		//IP地址
			Write_PortAdd((Runningbuf[7]<<8)|Runningbuf[8]);	//端口号		
			break;
		case 0xC3:	//查询  IP地址和端口号
			Read_IPAdd((uint8_t *)_lwipADD);		//printf("IP地址------------%d.%d.%d.%d\r\n",g_lwipADD[0],g_lwipADD[1],g_lwipADD[2],g_lwipADD[3]);
			_lwipPort = Read_PortAdd();				//printf("端口号------------%5d\r\n",g_lwipPort);
			Runningbuf[3] = _lwipADD[0];	//数据域
			Runningbuf[4] = _lwipADD[1];	//数据域
			Runningbuf[5] = _lwipADD[2];	//数据域
			Runningbuf[6] = _lwipADD[3];	//数据域
			Runningbuf[7] = _lwipPort/256;	//数据域
			Runningbuf[8] = _lwipPort%256;	//数据域
			break;
		case 0x05:	//通讯码
			for(i=0;i<16;i++)
			{
				Runningbuf[3+i] = SerialDat[3+i];	//数据域01
				Sendbuf[i] = SerialDat[3+i];	//数据域01
			}
			Write_ACUAdd((uint8_t *)Sendbuf);			
			break;
		case 0xC5:	//查询  通讯码
			Read_ACUAdd((uint8_t *)_ACUAdd);
			for(i=0;i<16;i++)
			{
				Runningbuf[3+i] = _ACUAdd[i];	//数据域
			}
			break;
		case 0x07:	//区域控制器SN
			for(i=0;i<4;i++)
			{
				Runningbuf[3+i] = SerialDat[3+i];	//数据域01
				Sendbuf[i] = SerialDat[3+i];	//数据域01
			}
			Write_ACUSN((uint8_t *)Sendbuf);		//区域控制器SN
			break;
		case 0x09:	//静态IP参数设置
			g_Setip[0] = SerialDat[3];
			g_Setip[1] = SerialDat[4];		g_Setip[2] = SerialDat[5];	
			g_Setip[3] = SerialDat[6];		g_Setip[4] = SerialDat[7];
			g_Setnetmask[0] = SerialDat[8];	g_Setnetmask[1] = SerialDat[9];	
			g_Setnetmask[2] = SerialDat[10];g_Setnetmask[3] = SerialDat[11];
			g_Setgateway[0] = SerialDat[12];g_Setgateway[1] = SerialDat[13];	
			g_Setgateway[2] = SerialDat[14];g_Setgateway[3] = SerialDat[15];
			//Write_SetIPFlag((uint8_t *)g_SetIPFlag);
			Write_localIP((uint8_t *)g_Setip);
			Write_netmask((uint8_t *)g_Setnetmask);
			Write_gateway((uint8_t *)g_Setgateway);
		
			//g_SetIPFlag = Read_SetIPFlag();
			Read_localIP((uint8_t *)g_Setip);
			Read_netmask((uint8_t *)g_Setnetmask);
			Read_gateway((uint8_t *)g_Setgateway);

			Runningbuf[3] = g_Setip[0];
			Runningbuf[4] = g_Setip[1];			Runningbuf[5] = g_Setip[2]; 		Runningbuf[6] = g_Setip[3]; 		Runningbuf[7] = g_Setip[4];
			Runningbuf[8] = g_Setnetmask[0];	Runningbuf[9] = g_Setnetmask[1]; 	Runningbuf[10] = g_Setnetmask[2]; 	Runningbuf[11] = g_Setnetmask[3];
			Runningbuf[12] = g_Setgateway[0];	Runningbuf[13] = g_Setgateway[1];	Runningbuf[14] = g_Setgateway[2];	Runningbuf[15] = g_Setgateway[3];
			printf("---------接收到串口---------\r\n");
			if( g_Setip[0] == 0xAA )	printf("静态IP---开启\r\n");	else printf("静态IP---禁用\r\n");
			printf("静态IP地址......%d.%d.%d.%d\r\n",g_Setip[1],g_Setip[2],g_Setip[3],g_Setip[4]);
			printf("子网掩码........%d.%d.%d.%d\r\n",g_Setnetmask[0],g_Setnetmask[1],g_Setnetmask[2],g_Setnetmask[3]);
			printf("默认网关........%d.%d.%d.%d\r\n",g_Setgateway[0],g_Setgateway[1],g_Setgateway[2],g_Setgateway[3]);
			break;
		case 0xC9:	//查询  静态IP参数设置
			Read_localIP((uint8_t *)_Setip);	//静态IP标志 0xAA为静态IP
			Read_netmask((uint8_t *)_Setnetmask);
			Read_gateway((uint8_t *)_Setgateway);
			Runningbuf[3] = _Setip[0];
			Runningbuf[4] = _Setip[1];			Runningbuf[5] = _Setip[2]; 			Runningbuf[6] = _Setip[3]; 			Runningbuf[7] = _Setip[4];
			Runningbuf[8] = _Setnetmask[0];		Runningbuf[9] = _Setnetmask[1]; 	Runningbuf[10] = _Setnetmask[2]; 	Runningbuf[11] = _Setnetmask[3];
			Runningbuf[12] = _Setgateway[0];	Runningbuf[13] = _Setgateway[1];	Runningbuf[14] = _Setgateway[2];	Runningbuf[15] = _Setgateway[3];
			break;
		case 0x10:	//设置RFID_Key
			FM1702_Key[0] = SerialDat[3];	FM1702_Key[1] = SerialDat[4]; 	FM1702_Key[2] = SerialDat[5];	//FM1702_Key
			FM1702_Key[3] = SerialDat[6];	FM1702_Key[4] = SerialDat[7]; 	FM1702_Key[5] = SerialDat[8];
			FM1702_Key[6] = SerialDat[10];	//块地址
			FM1702KeyCRC = CRC8_Table(FM1702_Key,0x07);	//CRC校验位
			Write_RFID_Key((uint8_t *)FM1702_Key);
			printf("---------接收到串口 RFID_Key %2X%2X%2X%2X%2X%2X,块地址：%d\r\n",FM1702_Key[0],FM1702_Key[1],FM1702_Key[2],FM1702_Key[3],FM1702_Key[4],FM1702_Key[5],FM1702_Key[6]);
		case 0x12:	//查询RFID_Key
			Read_RFID_Key((uint8_t *)RFID_Key);
			Runningbuf[3] = RFID_Key[0];	//数据域 RFID_Key1
			Runningbuf[4] = RFID_Key[1];	//数据域 RFID_Key2
			Runningbuf[5] = RFID_Key[2];	//数据域 RFID_Key3
			Runningbuf[6] = RFID_Key[3];	//数据域 RFID_Key4
			Runningbuf[7] = RFID_Key[4];	//数据域 RFID_Key5
			Runningbuf[8] = RFID_Key[5];	//数据域 RFID_Key6
			Runningbuf[9] = SerialDat[9];	//数据域 A密码 0x60
			Runningbuf[10] = RFID_Key[6];	//数据域 块地址
			break;	
        default:
        	break;
	}
	Runningbuf[SerialDat[1]-1] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRC校验位
	//comSendBuf(SERIALPORT_COM, (uint8_t *)Runningbuf, Runningbuf[1]);
}


