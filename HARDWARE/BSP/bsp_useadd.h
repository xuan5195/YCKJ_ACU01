#ifndef __BSP_USEADD_H
#define __BSP_USEADD_H	 
#include "sys.h"

#define	COUNT_ADD	0x0001	//计数总量地址
#define	Logic_ADD		0	//逻辑地址	
#define	Physical_ADD1	1	//物理地址1
#define	Physical_ADD2	2	//物理地址2
#define	Physical_ADD3	3	//物理地址3
#define	Physical_ADD4	4	//物理地址4
#define	Start_ADD	0x000A	//起始地址

uint8_t Distribute_LogicADD(uint8_t *Physical_no);
uint8_t Delete_LogicADD(uint8_t Logic_no);
void PowerUPLogitADDCheck(void);
void Read_RFID_Key(uint8_t *RFID_Key_Temp);
void Write_RFID_Key(uint8_t *RFID_Key_Temp);//RFID_Key
void Read_IPAdd(uint8_t *IPAdd_Temp);
void Write_IPAdd(uint8_t *IPAdd_Temp);//IP地址
uint16_t Read_PortAdd(void); //端口号
void Write_PortAdd(uint16_t PortAdd_Temp);	//端口号
void Read_ACUSN(uint8_t *ACUSN_Temp);//区域控制器SN
void Write_ACUSN(uint8_t *ACUSN_Temp);	//区域控制器SN
void Read_ACUAdd(uint8_t *ACUAdd_Temp);//通信码 16位
void Write_ACUAdd(uint8_t *ACUAdd_Temp);//通信码 16位
void LogitADDWrite(uint8_t _AddDat);

void Write_localIP(uint8_t *_Temp);	//本机IP
void Write_netmask(uint8_t *_Temp);	//子网掩码:255.255.255.0
void Write_gateway(uint8_t *_Temp);	//默认网关:192.168.1.1
void Read_localIP(uint8_t *_Temp);
void Read_netmask(uint8_t *_Temp);
void Read_gateway(uint8_t *_Temp);
//void Write_SetIPFlag(uint8_t *_Temp);
//uint8_t Read_SetIPFlag(void);	//g_SetIPFlag


#endif
