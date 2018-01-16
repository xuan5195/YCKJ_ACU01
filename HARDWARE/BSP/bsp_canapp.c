#include "stm32f10x.h"
#include <stdio.h>

#include "bsp_can.h"
#include "bsp_canapp.h"
#include "bsp_crc8.h"

#include "includes.h"

extern uint8_t g_RxMessage[8];	//CAN��������
extern uint8_t g_RxMessFlag;	//CAN�������� ��־

//д���߼���ַ
//_LogitADD���߼���ַ
uint8_t Can_WriteLogitADD(uint8_t _LogitADD,uint8_t *_PhysicalADD)
{
    uint8_t i,ReTemp=0,Overflow_Flag=0,res=0;
	uint8_t Runningbuf[8]={0x00},Recebuf[8]={0x00};
	Runningbuf[0] = *(_PhysicalADD+0);	//�����ַ1
	Runningbuf[1] = *(_PhysicalADD+1);	//�����ַ2
	Runningbuf[2] = *(_PhysicalADD+2);	//�����ַ3
	Runningbuf[3] = *(_PhysicalADD+3);	//�����ַ4
	Runningbuf[4] = _LogitADD;			//�߼���ַ

	g_RxMessFlag = 0x00;
	for(i=0;i<8;i++){	g_RxMessage[i] = 0x00;	}
	Package_Send(0x02,(uint8_t *)Runningbuf);	//д���߼���ַ

	Overflow_Flag = 20;
    printf("Over=%d",Overflow_Flag);  
	while((res==0)&& (Overflow_Flag>0) )//���յ�������
	{
//		res=Can_Receive_Msg(Recebuf);
//		if(Recebuf[5]!=_LogitADD)	res = 0;
//		else	continue;
		if(g_RxMessFlag == 0xAA)
		{
			for(i=0;i<8;i++){	Recebuf[i] = g_RxMessage[i];	g_RxMessage[i] = 0x00;	}
			printf("Rece<<%02X%02X%02X%02X%02X%02X%02X%02X;\r\n",Recebuf[0],Recebuf[1],Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
			g_RxMessFlag = 0x00;
			if((Recebuf[5]!=0xC2)&&(Recebuf[5]!=_LogitADD))	res = 0;
			else {	res = 1;	continue;	}
		}			
		OSTimeDlyHMSM(0, 0, 0, 5);
		Overflow_Flag--;
	}
    printf("-%d; ",Overflow_Flag); 
	if(res!=0)
	{
		printf("Re<<%02X%02X%02X%02X%02X%02X%02X%02X ",Recebuf[0],Recebuf[1],Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
		if( (Recebuf[0] == 0xC2)&&(Recebuf[1] == Runningbuf[0])&&(Recebuf[2] == Runningbuf[1])\
		&&(Recebuf[3] == Runningbuf[2])&&(Recebuf[4] == Runningbuf[3])&&(Recebuf[5] == Runningbuf[4]))
		{
			ReTemp = Recebuf[6];		//������ȷ ���ذ汾��
			res = 0;
		}
		else	ReTemp = 0x00;	//���ݴ���  a.��ʱ��b.���ݴ�
	}
	else	ReTemp = 0x00;	//���ݴ���  a.��ʱ��b.���ݴ�
	return 	ReTemp;
}
//�㲥 RFID
void Can_SendBroadcast_Key(uint8_t *_uBuff)
{
	uint8_t Runningbuf[8]={0x00};
	Runningbuf[0] = *(_uBuff+0);	//Key1
	Runningbuf[1] = *(_uBuff+1);	//
	Runningbuf[2] = *(_uBuff+2);	//
	Runningbuf[3] = *(_uBuff+3);	//
	Runningbuf[4] = *(_uBuff+4);	//
	Runningbuf[5] = *(_uBuff+5);	//
	Runningbuf[6] = *(_uBuff+6);	//��
	//printf(">>>>>>>>>�㲥 RFID_Key %2X%2X%2X%2X%2X%2X,���ַ��%d\r\n",Runningbuf[0],Runningbuf[1],Runningbuf[2],Runningbuf[3],Runningbuf[4],Runningbuf[5],Runningbuf[6]);
	Package_Send(0x03,(uint8_t *)Runningbuf);
}
//δע��㲥 
void Can_UnregisteredBroadcast(void)
{
	uint8_t Runningbuf[8]={0x00};
	Package_Send(0x01,(uint8_t *)Runningbuf);
}

//ˮ�ѹ㲥
void Can_SendBroadcast_Com(uint8_t _WaterCost,uint8_t _CostNum)
{
	uint8_t Runningbuf[8]={0x00};
	Runningbuf[0] = _WaterCost;	//WaterCost=ˮ�� ��С�ۿ��� 0.005Ԫ
	Runningbuf[1] = _CostNum;	//������������ ÿ��ˮ��������
	printf(">>>>>>>>>�㲥 ˮ��=%d; ������=%d\r\n",Runningbuf[0],Runningbuf[1]);
	Package_Send(0x04,(uint8_t *)Runningbuf);
}


//��ȡδע�������ַ 
uint8_t Can_ReadUnregistered(uint8_t *_uBuff)
{
	uint8_t i,Overflow_Flag=20;
	uint8_t CRC_Dat=0,res=0;
	uint8_t Recebuf[8]={0x00};
	while((res==0)&& (Overflow_Flag>0) )//���յ�������
	{
//		res = Can_Receive_Msg(Recebuf);
//		if(res != 0)	continue;
		if(g_RxMessFlag == 0xAA)
		{
			for(i=0;i<8;i++){	Recebuf[i] = g_RxMessage[i];	g_RxMessage[i] = 0x00;	}
			printf("Rece<<%02X%02X%02X%02X%02X%02X%02X%02X;\r\n",Recebuf[0],Recebuf[1],Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
			g_RxMessFlag = 0x00;
			res = 1;
			continue;
		}			
		OSTimeDlyHMSM(0, 0, 0, 5);
		Overflow_Flag--;
	}
	//printf("- %d;",Overflow_Flag);  
	if( res != 0 )
	{
		if( Recebuf[0] == 0xB3 )	
		{
			_uBuff[0] = Recebuf[1];	//���������ַ1��
			_uBuff[1] = Recebuf[2];	//���������ַ2��
			_uBuff[2] = Recebuf[3];	//���������ַ3��
			_uBuff[3] = Recebuf[4];	//���������ַ4��
			CRC_Dat = CRC8_Table(_uBuff,4);
			if( CRC_Dat == Recebuf[5] )	return 0x01;
			else						return 0x00;	//���ݴ���  a.��ʱ��b.���ݴ�
		}
		else	return 0x00;	//���ݴ���  a.��ʱ��b.���ݴ�		
	}
	else 	return 0x00;	//���ݴ���  a.��ʱ��b.���ݴ�
}
//��ѯ�Ƿ�������
uint8_t Can_SendPBus_Com(uint8_t _uDat,uint8_t *_uBuff)
{
	uint8_t Recebuf[8]={0x00};
	uint8_t Runningbuf[8]={0x00};
	uint8_t Overflow_Flag=10;
    uint8_t ReTemp=0;
	uint8_t i,res=0;
	Runningbuf[0] = _uDat;	//�β�  �߼���ַ
	Package_Send(0x05,(uint8_t *)Runningbuf);
	Overflow_Flag=10;
    printf("��ѭ %d ",Overflow_Flag);
	while((res==0)&& (Overflow_Flag>0) )//���յ�������
	{
//		res=Can_Receive_Msg(Recebuf);
//		if(Recebuf[1]!=_uDat)	res = 0;
//		else	continue;
		if(g_RxMessFlag == 0xAA)
		{
			for(i=0;i<8;i++){	Recebuf[i] = g_RxMessage[i];	g_RxMessage[i] = 0x00;	}
			//printf("Rece<<%02X%02X%02X%02X%02X%02X%02X%02X;\r\n",Recebuf[0],Recebuf[1],Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
			g_RxMessFlag = 0x00;
			if(Recebuf[1]!=_uDat)	res = 0;
			else	{	res = 1;	continue;	}		
		}
		OSTimeDlyHMSM(0, 0, 0, 5);
		Overflow_Flag--;
	}
    printf("- %d;  \r\n",Overflow_Flag);
	if(res)
	{
		printf("Rece<< %02X%02X%02X%02X%02X%02X%02X%02X;\r\n",Recebuf[0],Recebuf[1],Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
		if( ( Recebuf[1] == _uDat )&&(Recebuf[0] == 0x02 ))		ReTemp = 0x02;	//������
		else if( ( Recebuf[1] == _uDat )&&(Recebuf[0] == 0x03 ))	//�忨���ݰ�1
		{
			_uBuff[0] = 0x05;	//05��ʾ�忨��06��ʾ�ο���
			_uBuff[1] = Recebuf[2];	//����1��
			_uBuff[2] = Recebuf[3];	//����2��
			_uBuff[3] = Recebuf[4];	//����3��
			_uBuff[4] = Recebuf[5];	//����4��
			Overflow_Flag=10;res=0;
			while((res==0)&& (Overflow_Flag>0) )//���յ�������
			{
//				res=Can_Receive_Msg(Recebuf);
//				if(Recebuf[1]!=_uDat)	res = 0;
//				else	continue;
				if(g_RxMessFlag == 0xAA)
				{
					for(i=0;i<8;i++){	Recebuf[i] = g_RxMessage[i];	g_RxMessage[i] = 0x00;	}
					//printf("Rece<<%02X%02X%02X%02X%02X%02X%02X%02X;\r\n",Recebuf[0],Recebuf[1],Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
					g_RxMessFlag = 0x00;
					if(Recebuf[1]!=_uDat)	res = 0;
					else	{	res = 1;	continue;	}		
				}
				OSTimeDlyHMSM(0, 0, 0, 5);
				Overflow_Flag--;
			}
			if(res)
			{
				printf("Rece<< %02X%02X%02X%02X%02X%02X%02X%02X;\r\n",Recebuf[0],Recebuf[1],Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
				if(( Recebuf[1] == _uDat )&&( Recebuf[0] == 0x04 ))	//�ж��Ƿ��Ǳ�������
				{
					_uBuff[5] = Recebuf[2];	//���1��
					_uBuff[6] = Recebuf[3];	//���2��
					_uBuff[7] = Recebuf[4];	//���3��
					_uBuff[8] = Recebuf[5];	//��У�飻
					_uBuff[9] = Recebuf[6];	//ͨ���룻
					ReTemp = 0xAA;	//���ݽ��ճɹ�
					return ReTemp;
				}
				else	ReTemp = 0x00;	//���ݽ��մ���  a.��ʱ��b.���ݴ�
				res = 0;
			}
			else	ReTemp = 0x00;	//���ݽ��մ���  a.��ʱ��b.���ݴ�
		}
		else if( ( Recebuf[1] == _uDat )&&(Recebuf[0] == 0x05 ))	//�ο����ݰ�1
		{
			_uBuff[0] = 0x06;	//05��ʾ�忨��06��ʾ�ο���
			_uBuff[1] = Recebuf[2];	//����1��
			_uBuff[2] = Recebuf[3];	//����2��
			_uBuff[3] = Recebuf[4];	//����3��
			_uBuff[4] = Recebuf[5];	//����4��
			Overflow_Flag=10;
			while((res==0)&& (Overflow_Flag>0) )//���յ�������
			{
//				res=Can_Receive_Msg(Recebuf);
//				if(Recebuf[1]!=_uDat)	res = 0;
//				else	continue;
				if(g_RxMessFlag == 0xAA)
				{
					for(i=0;i<8;i++){	Recebuf[i] = g_RxMessage[i];	g_RxMessage[i] = 0x00;	}
					//printf("Rece<<%02X%02X%02X%02X%02X%02X%02X%02X;\r\n",Recebuf[0],Recebuf[1],Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
					g_RxMessFlag = 0x00;
					if(Recebuf[1]!=_uDat)	res = 0;
					else	{	res = 1;	continue;	}		
				}
				OSTimeDlyHMSM(0, 0, 0, 5);
				Overflow_Flag--;
			}
			if(res)
			{
				printf("Rece<< %02X%02X%02X%02X%02X%02X%02X%02X;\r\n",Recebuf[0],Recebuf[1],Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
				if(( Recebuf[1] == _uDat )&&( Recebuf[0] == 0x06 ))	//�ж��Ƿ��Ǳ�������
				{
					_uBuff[5] = Recebuf[2];	//���1��
					_uBuff[6] = Recebuf[3];	//���2��
					_uBuff[7] = Recebuf[4];	//���3��
					_uBuff[8] = Recebuf[5];	//��У�飻
					_uBuff[9] = Recebuf[6];	//ͨ���룻
					ReTemp = 0xAA;	//���ݽ��ճɹ�
					return ReTemp;
				}
				else	ReTemp = 0x00;	//���ݽ��մ���  a.��ʱ��b.���ݴ�
				res = 0;
			}
			else	ReTemp = 0x00;	//���ݽ��մ���  a.��ʱ��b.���ݴ�
		}
		else if( ( Recebuf[1] == _uDat )&&(Recebuf[0] == 0x04 ))	//�忨���ݰ�2
		{
			_uBuff[5] = Recebuf[2];	//���1��
			_uBuff[6] = Recebuf[3];	//���2��
			_uBuff[7] = Recebuf[4];	//���3��
			_uBuff[8] = Recebuf[5];	//��У�飻
			_uBuff[9] = Recebuf[6];	//ͨ���룻
			ReTemp = 0xAA;	//���ݽ��ճɹ�
			return ReTemp;
		}
		else if( ( Recebuf[1] == _uDat )&&(Recebuf[0] == 0x06 ))	//�ο����ݰ�2
		{
			_uBuff[5] = Recebuf[2];	//���1��
			_uBuff[6] = Recebuf[3];	//���2��
			_uBuff[7] = Recebuf[4];	//���3��
			_uBuff[8] = Recebuf[5];	//��У�飻
			_uBuff[9] = Recebuf[6];	//ͨ���룻
			ReTemp = 0xAA;	//���ݽ��ճɹ�
			return ReTemp;
		}
		else	ReTemp = 0x00;	//���ݽ��մ���  a.��ʱ��b.���ݴ�
		
	}
	else	ReTemp = 0x00;	//���ݽ��մ���  a.��ʱ��b.���ݴ�

	return ReTemp;
}

//ȡ�سɹ��ظ�
void Can_ReadPBus_Succeed(uint8_t *_uBuff)
{
	uint8_t Runningbuf[8]={0x00};
	Runningbuf[0] = _uBuff[0];	//�߼���ַ
	Runningbuf[1] = _uBuff[1];	//����1
	Runningbuf[2] = _uBuff[2];	//����2
	Runningbuf[3] = _uBuff[3];	//����3
	Runningbuf[4] = _uBuff[4];	//����4
	Runningbuf[5] = _uBuff[5];	//�忨/�ο�
	Package_Send(0x06,(uint8_t *)Runningbuf);
}
//ȡ�سɹ��ظ�
void Can_SendPBus_ErrCom(uint8_t *_uBuff)
{
	uint8_t Runningbuf[8]={0x00};
	Runningbuf[0] = _uBuff[0];	//�߼���ַ
	Runningbuf[1] = _uBuff[1];	//"E"
	Runningbuf[2] = _uBuff[2];	//�쳣�����λ
	Runningbuf[3] = _uBuff[3];	//�쳣����
	Runningbuf[4] = _uBuff[4];	//�쳣�����λ
	Runningbuf[6] = _uBuff[5];	//ͨ����
	Package_Send(0x11,(uint8_t *)Runningbuf);
}
