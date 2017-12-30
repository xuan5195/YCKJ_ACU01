#ifndef __BSP_USEADD_H
#define __BSP_USEADD_H	 
#include "sys.h"

#define	COUNT_ADD	0x0001	//����������ַ
#define	Logic_ADD		0	//�߼���ַ	
#define	Physical_ADD1	1	//�����ַ1
#define	Physical_ADD2	2	//�����ַ2
#define	Physical_ADD3	3	//�����ַ3
#define	Physical_ADD4	4	//�����ַ4
#define	Start_ADD	0x000A	//��ʼ��ַ

uint8_t Distribute_LogicADD(uint8_t *Physical_no);
uint8_t Delete_LogicADD(uint8_t Logic_no);
void PowerUPLogitADDCheck(void);
void Read_RFID_Key(uint8_t *RFID_Key_Temp);
void Write_RFID_Key(uint8_t *RFID_Key_Temp);//RFID_Key
void Read_IPAdd(uint8_t *IPAdd_Temp);
void Write_IPAdd(uint8_t *IPAdd_Temp);//IP��ַ
uint16_t Read_PortAdd(void); //�˿ں�
void Write_PortAdd(uint16_t PortAdd_Temp);	//�˿ں�
void Read_ACUSN(uint8_t *ACUSN_Temp);//���������SN
void Write_ACUSN(uint8_t *ACUSN_Temp);	//���������SN
void Read_ACUAdd(uint8_t *ACUAdd_Temp);//ͨ���� 16λ
void Write_ACUAdd(uint8_t *ACUAdd_Temp);//ͨ���� 16λ
void LogitADDWrite(uint8_t _AddDat);

void Write_localIP(uint8_t *_Temp);	//����IP
void Write_netmask(uint8_t *_Temp);	//��������:255.255.255.0
void Write_gateway(uint8_t *_Temp);	//Ĭ������:192.168.1.1
void Read_localIP(uint8_t *_Temp);
void Read_netmask(uint8_t *_Temp);
void Read_gateway(uint8_t *_Temp);
//void Write_SetIPFlag(uint8_t *_Temp);
//uint8_t Read_SetIPFlag(void);	//g_SetIPFlag


#endif
