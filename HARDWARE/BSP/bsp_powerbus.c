#include "bsp_powerbus.h"
#include "bsp_uart_fifo.h"
#include "bsp_crc8.h"
#include "bsp_useadd.h"

#include "delay.h"
#include "sys.h"
#include "string.h"
#include "usmart.h"	
#include "includes.h"
#include "led.h"

#include "bsp_can.h"
#include "bsp_canapp.h"

uint8_t Overflow_Flag;	//�ȴ���ʱ��־
extern uint8_t g_RUNDate[32][17];    //�������ݣ�
extern uint8_t PBus_Count;
extern uint8_t PBusDat[16];
extern uint8_t g_LockFlag;
extern uint8_t KJ_Versions[32];				//�����汾��

//δע��㲥 
void UnregisteredBroadcast(void)
{
	uint8_t Runningbuf[4]={0xF3,0x04,0xA1,0x00};
	Runningbuf[3] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRCУ��λ
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);
    OSTimeDlyHMSM(0, 0, 0, 150);
}

//��ȡδע�������ַ 
uint8_t ReadUnregistered(uint8_t *_uBuff)
{
	Overflow_Flag = 10;
	while( ( (PBus_Count&0x80)!=0x80 ) && (Overflow_Flag>1) ) //�ȴ����ݻظ���
	{
		OSTimeDlyHMSM(0, 0, 0, 100);
		Overflow_Flag--;
		ReceivePowerBUSDat(0x01);	//������ģʽ
	}
	if( PBus_Count>0x80 )	//��ʾ���յ�����
	{
		PBus_Count = 0x00;
		_uBuff[0] = PBusDat[3];	//���������ַ1��
		_uBuff[1] = PBusDat[4];	//���������ַ2��
		_uBuff[2] = PBusDat[5];	//���������ַ3��
		_uBuff[3] = PBusDat[6];	//���������ַ4��
		return 0x01;
	}
	else	return 0x00;	//���ݽ��մ���  a.��ʱ��b.���ݴ�
}

//ˮ�ѹ㲥
void SendBroadcast_Com(uint8_t _WaterCost,uint8_t _CostNum)
{
	uint8_t Runningbuf[6]={0xF3,0x06,0x10,0x05,0x1B,0x00};
	Runningbuf[3] = _WaterCost;	//WaterCost=ˮ�� ��С�ۿ��� 0.005Ԫ
	Runningbuf[4] = _CostNum;	//������������ ÿ��ˮ��������
	printf(">>>>>>>>>�㲥 ˮ��=%d; ������=%d\r\n",Runningbuf[3],Runningbuf[4]);
	Runningbuf[Runningbuf[1]-1] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRCУ��λ
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);
}
//�㲥 RFID
void SendBroadcast_Key(uint8_t *_uBuff)
{
	uint8_t Runningbuf[11]={0xF3,0x0B,0xCD,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x01,0x00};
	Runningbuf[3] = *(_uBuff+0);	//Key1
	Runningbuf[4] = *(_uBuff+1);	//
	Runningbuf[5] = *(_uBuff+2);	//
	Runningbuf[6] = *(_uBuff+3);	//
	Runningbuf[7] = *(_uBuff+4);	//
	Runningbuf[8] = *(_uBuff+5);	//
	Runningbuf[9] = *(_uBuff+6);	//��
	printf(">>>>>>>>>�㲥 RFID_Key %2X%2X%2X%2X%2X%2X,���ַ��%d\r\n",Runningbuf[3],Runningbuf[4],Runningbuf[5],Runningbuf[6],Runningbuf[7],Runningbuf[8],Runningbuf[9]);
	Runningbuf[Runningbuf[1]-1] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRCУ��λ
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);
}
//��ѯ�Ƿ�������
uint8_t SendPBus_Com(uint8_t _uDat,uint8_t *_uBuff)
{
    uint8_t ReTemp=0;
	uint8_t Runningbuf[5]={0xF3,0x05,0x01,0x00,0x00};
	Runningbuf[3] = _uDat;	//�β�  �߼���ַ
	Runningbuf[4] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRCУ��λ
	PBus_Count = 0x00;
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);	//������ѭȡ��������
	Overflow_Flag = 25;
    printf("��ѭ���� %d ",Overflow_Flag);
	while( ( (PBus_Count&0x80)!=0x80 ) && (Overflow_Flag>0) ) //�ȴ����ݻظ���
	{
		OSTimeDlyHMSM(0, 0, 0, 8);
		Overflow_Flag--;
		ReceivePowerBUSDat(0x01);	//������ģʽ
	}
    printf("- %d;  ",Overflow_Flag);
	if( ( (PBus_Count&0x80) == 0x80 ))
	{
		PBus_Count = 0x00;
		if( PBusDat[3] == _uDat )
		{
			if( PBusDat[2] == 0x02 )		ReTemp = 0x02;	//������
			else//������
            {
                _uBuff[0] = PBusDat[2];	//05��ʾ�忨��06��ʾ�ο���
                _uBuff[1] = PBusDat[4];	//����1��
                _uBuff[2] = PBusDat[5];	//����2��
                _uBuff[3] = PBusDat[6];	//����3��
                _uBuff[4] = PBusDat[7];	//����4��
                _uBuff[5] = PBusDat[8];	//���1��
                _uBuff[6] = PBusDat[9];	//���2��
                _uBuff[7] = PBusDat[10];//���3��
                _uBuff[8] = PBusDat[11];//��У�飻
                _uBuff[9] = PBusDat[12];//ͨ���룻
                ReTemp = 0xAA;		//������ȷ 
            }
			printf("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n",PBusDat[0],PBusDat[1],PBusDat[2],PBusDat[3],PBusDat[4],PBusDat[5],PBusDat[6],PBusDat[7],PBusDat[8],PBusDat[9],PBusDat[10],PBusDat[11]);
		}
		else	ReTemp = 0x00;	//���ݽ��մ���  a.��ʱ��b.���ݴ�
	}
	else		ReTemp = 0x00;	//���ݽ��մ���  a.��ʱ��b.���ݴ�
	return ReTemp;
}

//�����ƶ�����
uint8_t SendPBus_Dat(uint8_t _uDat,uint8_t *_uBuff)
{
    uint8_t ReTemp=0;
	uint8_t Runningbuf[14]={0xF3,0x0E,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	Runningbuf[3] = _uDat;	//�β�  �߼���ַ
	Runningbuf[4] = _uBuff[0];	//��SN 1
	Runningbuf[5] = _uBuff[1];	//��SN 2
	Runningbuf[6] = _uBuff[2];	//��SN 3
	Runningbuf[7] = _uBuff[3];	//��SN 4
	Runningbuf[8] = _uBuff[4];	//���ڽ��1 ��λ
	Runningbuf[9] = _uBuff[5];	//���ڽ��2 ��λ
	Runningbuf[10] = _uBuff[6];	//���ڽ��3 ��λ
	Runningbuf[11] = _uBuff[7];	//����У������
	Runningbuf[12] = _uBuff[8];	//ͨ����
	Runningbuf[Runningbuf[1]-1] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRCУ��λ
	PBus_Count = 0x00;
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);
	Overflow_Flag = 40;
    printf("Over_F = %d",Overflow_Flag);
	while( ( (PBus_Count&0x80)!=0x80 ) && (Overflow_Flag>0) ) //�ȴ����ݻظ���
	{
		OSTimeDlyHMSM(0, 0, 0, 10);
		Overflow_Flag--;
		ReceivePowerBUSDat(0x01);	//������ģʽ
	}
    printf("- %d;\r\n",Overflow_Flag);
	if( ( (PBus_Count&0x80) == 0x80 ))
	{
		PBus_Count = 0x00;
		if( PBusDat[3] == _uDat )
		{
			if( PBusDat[2] == 0x09 )	ReTemp = 0xAA;	//д����ȷ
			else                		ReTemp = 0x00;	//ʧ��
		}
		else	ReTemp = 0x00;	//ʧ��
	}
	else		ReTemp = 0x00;	//ʧ��
	return ReTemp;
}

//���Ϳ��쳣��������
uint8_t SendPBus_Dat2(uint8_t _uDat,uint8_t *_uBuff)
{
    uint8_t ReTemp=0;
	uint8_t Runningbuf[14]={0xF3,0x0A,0x13,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	Runningbuf[3] = _uDat;	//�β�  �߼���ַ
	Runningbuf[4] = _uBuff[0];	//�������
	Runningbuf[5] = _uBuff[1];	//�������
	Runningbuf[6] = _uBuff[2];	//�������
	Runningbuf[7] = _uBuff[3];	//�������
	Runningbuf[8] = _uBuff[4];	//ͨ����
	Runningbuf[Runningbuf[1]-1] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRCУ��λ
	PBus_Count = 0x00;
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);
	Overflow_Flag = 40;
    printf("�쳣 = %d ",Overflow_Flag);
	while( ( (PBus_Count&0x80)!=0x80 ) && (Overflow_Flag>0) ) //�ȴ����ݻظ���
	{
		OSTimeDlyHMSM(0, 0, 0, 10);
		Overflow_Flag--;
		ReceivePowerBUSDat(0x01);	//������ģʽ
	}
    printf("- %d; ",Overflow_Flag);
	if( ( (PBus_Count&0x80) == 0x80 ))
	{
		PBus_Count = 0x00;
		if( PBusDat[3] == _uDat )
		{
			if( PBusDat[2] == 0x14 )	{ReTemp = 0xAA;printf("success \r\n");}	//д����ȷ
			else                		{ReTemp = 0x00;printf("fault   \r\n");}	//ʧ��
		}
		else	{ReTemp = 0x00;printf("fault   \r\n");}	//ʧ��
	}
	else		{ReTemp = 0x00;printf("fault   \r\n");}	//ʧ��
	return ReTemp;
}

//�����쳣����
uint8_t SendPBus_ErrCom(uint8_t *_uBuff)
{
    uint8_t ReTemp=0;
	uint8_t Runningbuf[10]={0xF3,0x09,0x00,0x00,0x45,0x30,0x30,0x30,0x00,0x00};
	Runningbuf[2] = _uBuff[0];	//�β�  ����֡
	Runningbuf[3] = _uBuff[1];	//�β�  �߼���ַ
	Runningbuf[5] = _uBuff[2];	//�쳣�����λ
	Runningbuf[6] = _uBuff[3];	//�쳣����	
	Runningbuf[7] = _uBuff[4];	//�쳣�����λ
	Runningbuf[8] = _uBuff[5];	//ͨ����	
	if(Runningbuf[2]==0x11)			Runningbuf[1] = 0x09;//������
	else if(Runningbuf[2]==0x13)	Runningbuf[1] = 0x0A;//�忨ȡ��
	Runningbuf[Runningbuf[1]-1] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRCУ��λ
	PBus_Count = 0x00;
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);
	Overflow_Flag = 40;
    printf("ErrCom %d ",Overflow_Flag);
	while( ( (PBus_Count&0x80)!=0x80 ) && (Overflow_Flag>0) ) //�ȴ����ݻظ���
	{
		OSTimeDlyHMSM(0, 0, 0, 10);
		Overflow_Flag--;
		ReceivePowerBUSDat(0x01);	//������ģʽ
	}
    printf("- %d;",Overflow_Flag);
	if( ( (PBus_Count&0x80) == 0x80 ))
	{
		PBus_Count = 0x00;
		if( PBusDat[3] == _uBuff[1] )	ReTemp = 0xAA;		//������ȷ 
		else							ReTemp = 0x00;	//���ݽ��մ���  a.��ʱ��b.���ݴ�
	}
	else		ReTemp = 0x00;	//���ݽ��մ���  a.��ʱ��b.���ݴ�
	return ReTemp;
}

//�۰���ҷ� ����
//_uDat��ģʽ
//_uDat=0xB1	����
//_uDat=0xB2	С����
//*_uBuff:�����ַ �۰���ҵ�ַ
uint8_t SendPBus_SearchDat(uint8_t _uDat,uint8_t *_uBuff)
{
    uint8_t u8Temp=0,ReTemp=0;
	uint8_t Runningbuf[8]={0xF3,0x08,0x00,0x00,0x00,0x00,0x00,0x00};
	uint8_t uCADDDat[4]={0x00,0x00,0x00,0x00};
	Runningbuf[2] = _uDat;		//�β�  ģʽ ����/С����
	Runningbuf[4] = _uBuff[0];	//SN1
	Runningbuf[5] = _uBuff[1];	//SN2
	Runningbuf[6] = _uBuff[2];	//SN3
	Runningbuf[7] = _uBuff[3];	//SN4
	Runningbuf[Runningbuf[1]-1] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRCУ��λ
	PBus_Count = 0x00;
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);
	Overflow_Flag = 2;
    printf("�۰����%2X  %d ",_uDat,Overflow_Flag);
	while( ( (PBus_Count&0x80)!=0x80 ) && (Overflow_Flag>0) ) //�ȴ����ݻظ���
	{
		OSTimeDlyHMSM(0, 0, 0, 300);
		Overflow_Flag--;
		ReceivePowerBUSDat(0x00);	//����У��ģʽ
	}
    printf("- %d; ",Overflow_Flag);
	if( ( (PBus_Count&0x80) == 0x80 ))
	{
		PBus_Count = 0x00;
		if(CRC8_Table(PBusDat,0x07)==PBusDat[0x07])	//CRCУ��ͨ��
		{
			if( PBusDat[2] == 0xB3 )
			{
				printf("\r\n���յ���վ�������  �����ַ��0x%02x 0x%02x 0x%02x 0x%02x  ;",PBusDat[3],PBusDat[4],PBusDat[5],PBusDat[6]);   
				uCADDDat[0]=PBusDat[3];	uCADDDat[1]=PBusDat[4];	uCADDDat[2]=PBusDat[5];	uCADDDat[3]=PBusDat[6];
				u8Temp = Distribute_LogicADD((uint8_t *)uCADDDat);	//�����߼���ַ
				printf("����ɹ��� �߼���ַ:%d;\r\n",u8Temp);  
				if(u8Temp<=32)
				{
					KJ_Versions[u8Temp] = WriteLogitADD(u8Temp,(uint8_t *)uCADDDat);
					if(KJ_Versions[u8Temp] != 0x00)	//�ֱ��߼���ַ�ɹ�
					g_RUNDate[0][0]++; g_RUNDate[u8Temp][0] = 0xAA; //����+1���õ�ַʹ�ã�
					g_RUNDate[u8Temp][3] = uCADDDat[0]; g_RUNDate[u8Temp][4] = uCADDDat[1];//����SN
					g_RUNDate[u8Temp][5] = uCADDDat[2]; g_RUNDate[u8Temp][6] = uCADDDat[3];
				}
				ReTemp = 0x00;	printf("success \r\n");
			}
			else{ReTemp = 0x01;	printf("fault   \r\n");}//CRCУ��ʧ�� �ж�����������	
		}
		else	{ReTemp = 0x02;	printf("fault   \r\n");}//CRCУ��ʧ�� �ж�����������
	}
	else		{ReTemp = 0x03;	printf("fault   \r\n");}	//ʧ��
	return ReTemp;
}
/*
//����ȡ����
//_uDat:�߼���ַ
//_uBuff�����š����\��У�飻
uint8_t ReadPBus_Com(uint8_t _uDat,uint8_t *_uBuff)
{
    uint8_t ReTemp=0;
	uint8_t Runningbuf[5]={0xF3,0x05,0x04,0x00,0x00};
	Runningbuf[3] = _uDat;	//�β�  �߼���ַ
	Runningbuf[4] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRCУ��λ
	PBus_Count = 0x00;
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);	//����ȡ��������
	Overflow_Flag = 2;
	while( ( (PBus_Count&0x80)!=0x80 ) && (Overflow_Flag>0) ) //�ȴ����ݻظ���
	{
		OSTimeDlyHMSM(0, 0, 0, 180);
		Overflow_Flag--;
		ReceivePowerBUSDat(0x01);	//������ģʽ
	}
	if( ( (PBus_Count&0x80) == 0x80 ))
	{
		PBus_Count = 0x00;
		_uBuff[0] = PBusDat[2];	//05��ʾ�忨��06��ʾ�ο���
		_uBuff[1] = PBusDat[4];	//����1��
		_uBuff[2] = PBusDat[5];	//����2��
		_uBuff[3] = PBusDat[6];	//����3��
		_uBuff[4] = PBusDat[7];	//����4��
		_uBuff[5] = PBusDat[8];	//���1��
		_uBuff[6] = PBusDat[9];	//���2��
		_uBuff[7] = PBusDat[10];//��У�飻
		ReTemp = 0xAA;		//������ȷ 
	}
	else	ReTemp = 0x00;	//���ݽ��մ���  a.��ʱ��b.���ݴ�
	return 	ReTemp;
}
*/
//ȡ�سɹ��ظ�
//_uDat:�߼���ַ
void ReadPBus_Succeed(uint8_t _uDat)
{
	uint8_t Runningbuf[5]={0xF3,0x05,0x07,0x00,0x00};
	Runningbuf[3] = _uDat;	//�β�  �߼���ַ
	Runningbuf[4] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRCУ��λ
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);	//��������
    OSTimeDlyHMSM(0, 0, 0, 250);
}

//��ѭ��ʽȡͨ������
//_no:�߼���ַ
void ReadRunningData(uint8_t _no)
{	
	//1.�жϸ��߼���ַ�Ƿ���ʹ�ã�
	//			δʹ��	-->���أ�
	//			ʹ��	-->��ѯ�ȴ��ظ�;
	//							-->������	-->���أ�
	//							-->������	-->����ȡ���ݽ���������ȡ��
	//											���ظ�ȡ�سɹ�
	uint8_t uDat=0,TempBuff[10];
	uint8_t uBuff[8];
    printf("%2d; ",_no); 
	if(g_RUNDate[_no][0] == 0xAA)	//��������
	{	//���߼���ַʹ��
		if((g_RUNDate[_no][3] == 0x0)&&(g_RUNDate[_no][6] == 0x0))	{g_RUNDate[_no][0] = 0x0;}
		LED1 = !LED1;//LED1 = 0;
		uDat = Can_SendPBus_Com(_no,(uint8_t *)TempBuff);		//������ѭȡ��������
		if( uDat == 0x00 )		uDat = Can_SendPBus_Com(_no,(uint8_t *)TempBuff);
		{
			printf("uDat = %2d; ",uDat); 
			if( uDat == 0x02 )		//������	
			{
				g_RUNDate[_no][1] = 0xDD;
				g_RUNDate[_no][2] = 0x00;
			}
			else if( uDat == 0x00 )	//ʧ��
			{
				g_RUNDate[_no][1] = 0x00;			
				if(g_RUNDate[_no][2]<20)//����һ��������������ɾ��
				{	
					if(g_RUNDate[_no][2]%5==0)	LogitADDWrite(_no);	//���¼��ע��
					g_RUNDate[_no][2]++;	
				}
				else 
				{	
					//for(i=0;i<16;i++)	g_RUNDate[_no][i] = 0;	
					//Delete_LogicADD(_no);	//���䲻�ɹ���������߼���ַ
				}
			}	
			else//������
			{
				g_RUNDate[_no][2] = 0x00;
				g_RUNDate[_no][1] = 0xAA;	//������
				g_RUNDate[_no][2] = TempBuff[0];	//��״̬ 05��ʾ�忨��06��ʾ�ο���
				g_RUNDate[_no][7] = TempBuff[1];	//����1��
				g_RUNDate[_no][8] = TempBuff[2];	//����2��
				g_RUNDate[_no][9] = TempBuff[3];	//����3��
				g_RUNDate[_no][10] = TempBuff[4];	//����4��
				g_RUNDate[_no][11] = TempBuff[5];	//���1��
				g_RUNDate[_no][12] = TempBuff[6];	//���2��
				g_RUNDate[_no][13] = TempBuff[7];	//���3��
				g_RUNDate[_no][14] = TempBuff[8];	//��У�飻
				g_RUNDate[_no][15] = TempBuff[9];	//ͨ���룻
				printf("�߼���ַ��%d;��״̬%d;  ",_no,g_RUNDate[_no][2]);
				printf("���ţ�%02X%02X%02X%02X;  ",g_RUNDate[_no][7],g_RUNDate[_no][8],g_RUNDate[_no][9],g_RUNDate[_no][10]);
				printf("��%6d;   ", (g_RUNDate[_no][11]<<16)|(g_RUNDate[_no][12]<<8)|g_RUNDate[_no][13]);
				printf("��У�飺%2d;   ͨ���룺%02X; \r\n", g_RUNDate[_no][14],g_RUNDate[_no][15]);
				//ReadPBus_Succeed(_no);//�ظ�ȡ�سɹ�
				uBuff[0] = _no;	//�߼���ַ
				uBuff[1] = TempBuff[1];	//����1��
				uBuff[2] = TempBuff[2];	//����2��
				uBuff[3] = TempBuff[3];	//����3��
				uBuff[4] = TempBuff[4];	//����4��
				if(g_RUNDate[_no][2]==0x05)		uBuff[5] = 0xAA;	//��״̬ 05��ʾ�忨
				else if(g_RUNDate[_no][2]==0x06)uBuff[5] = 0x55;	//��״̬ 06��ʾ�ο�
				OSTimeDlyHMSM(0, 0, 0, 5);
				printf("%02X %02X %02X %02X %02X %02X %02X %02X\r\n",uBuff[0],uBuff[1],uBuff[2],uBuff[3],uBuff[4],uBuff[5],uBuff[6],uBuff[7]);
				Can_ReadPBus_Succeed((uint8_t *)uBuff);//�ظ�ȡ�سɹ�
				//OSTimeDlyHMSM(0, 0, 0, 50);
			}
		}
	}
	else if(g_RUNDate[_no][0] == 0x55)	//�쳣
	{
		LED1 = !LED1;//LED1 = 0;
		TempBuff[0] = _no;	//�߼���ַ
		TempBuff[1] = g_RUNDate[_no][7];	//�쳣����"E"
		TempBuff[2] = g_RUNDate[_no][8];	//�쳣�����λ
		TempBuff[3] = g_RUNDate[_no][9];	//�쳣����
		TempBuff[4] = g_RUNDate[_no][10];	//�쳣�����λ
		TempBuff[5] = g_RUNDate[_no][15];	//ͨ����
		Can_SendPBus_ErrCom((uint8_t *)TempBuff);
		//uDat = SendPBus_ErrCom((uint8_t *)TempBuff);
		//if( uDat == 0x00 )		uDat = SendPBus_ErrCom((uint8_t *)TempBuff);
	}
}

//��ѭ��ʽȡͨ������
//_no:�߼���ַ
void WriteRFIDData(uint8_t *_WriteDat)
{	
	uint8_t TempBuff[10]={0x00};
	uint8_t Runningbuf[8]={0x00},Recebuf[8]={0};
	uint8_t _no=0,res=0,Overflow_Flag=10;
    LED1 = !LED1;//LED1 = 0;
	_no	= _WriteDat[9];	//�߼���ַ
	printf("\r\n������ %2d; ",_no); 
	if(_WriteDat[0] == 0xAA)	//��������
	{
		TempBuff[0] = _WriteDat[1];	//��SN 1
		TempBuff[1] = _WriteDat[2];	//��SN 2
		TempBuff[2] = _WriteDat[3];	//��SN 3
		TempBuff[3] = _WriteDat[4];	//��SN 4
		TempBuff[4] = _WriteDat[10];//ͨ����
		TempBuff[5] = _WriteDat[6];	//���ڽ��1 ��λ
		TempBuff[6] = _WriteDat[7];	//���ڽ��2 ��λ
		TempBuff[7] = _WriteDat[8];	//���ڽ��3 ��λ
		TempBuff[8] = _WriteDat[5];	//����У������
		TempBuff[9] = _WriteDat[10];	//ͨ����
		
		///////�������ݰ�1
		Runningbuf[0] = _WriteDat[9];	//�߼���ַ
		Runningbuf[1] = TempBuff[0];
		Runningbuf[2] = TempBuff[1];
		Runningbuf[3] = TempBuff[2];
		Runningbuf[4] = TempBuff[3];
		Runningbuf[5] = TempBuff[4];
		printf("%02X %02X %02X %02X %02X %02X %02X %02X\r\n",Runningbuf[0],Runningbuf[1],\
		Runningbuf[2],Runningbuf[3],Runningbuf[4],Runningbuf[5],Runningbuf[6],Runningbuf[7]);
		Package_Send(0x08,(u8 *)Runningbuf);
		OSTimeDlyHMSM(0,0,0,100);  //��ʱ
		
		///////�������ݰ�2
		Runningbuf[0] = _WriteDat[9];	//�߼���ַ
		Runningbuf[1] = TempBuff[5];
		Runningbuf[2] = TempBuff[6];
		Runningbuf[3] = TempBuff[7];
		Runningbuf[4] = TempBuff[8];
		Runningbuf[5] = TempBuff[9];
		Runningbuf[6] = CRC8_Table(TempBuff,10);	//CRC
		printf("%02X %02X %02X %02X %02X %02X %02X %02X\r\n",Runningbuf[0],Runningbuf[1],\
		Runningbuf[2],Runningbuf[3],Runningbuf[4],Runningbuf[5],Runningbuf[6],Runningbuf[7]);
		Package_Send(0x09,(u8 *)Runningbuf);
	}
	else if(_WriteDat[0] == 0xBB)	//�쳣����
	{
		Runningbuf[0] = _no;	//�߼���ַ
		Runningbuf[1] = _WriteDat[2];	//�������"E"
		Runningbuf[2] = _WriteDat[3];	//��������λ
		Runningbuf[3] = _WriteDat[4];	//���������λ
		Runningbuf[4] = _WriteDat[5];	//��������λ
		Runningbuf[5] = _WriteDat[6];	//ͨ����
		Package_Send(0x11,(u8 *)Runningbuf);
	}
	
	Overflow_Flag=10;
	while((res==0)&& (Overflow_Flag>0) )//���յ�������
	{
		res=Can_Receive_Msg(Recebuf);
		OSTimeDlyHMSM(0, 0, 0, 5);
		Overflow_Flag--;
	}
	if(res)
	{
		printf("%02X %02X %02X %02X %02X %02X %02X %02X\r\n",Recebuf[0],Recebuf[1],\
		Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
	}
}


//д���߼���ַ
//_LogitADD���߼���ַ
uint8_t WriteLogitADD(uint8_t _LogitADD,uint8_t *_PhysicalADD)
{
    uint8_t ReTemp=0;
	uint8_t Runningbuf[9]={0xF3,0x09,0xC1,0x00,0x00,0x00,0x00,0x00,0x00};
	Runningbuf[3] = *(_PhysicalADD+0);	//�����ַ1
	Runningbuf[4] = *(_PhysicalADD+1);	//�����ַ2
	Runningbuf[5] = *(_PhysicalADD+2);	//�����ַ3
	Runningbuf[6] = *(_PhysicalADD+3);	//�����ַ4
	Runningbuf[7] = _LogitADD;			//�߼���ַ
	Runningbuf[Runningbuf[1]-1] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRCУ��λ
	PBus_Count = 0x00;
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);	//����ȡ��������
	Overflow_Flag = 30;
    printf("Over_Flag = %d",Overflow_Flag);  
	while( ( (PBus_Count&0x80)!=0x80 ) && (Overflow_Flag>0) ) //�ȴ����ݻظ���
	{
		OSTimeDlyHMSM(0, 0, 0, 10);
		Overflow_Flag--;
		ReceivePowerBUSDat(0x01);	//������ģʽ
	}
    printf("- %d;",Overflow_Flag);  
	if( ( (PBus_Count&0x80) == 0x80 ) &&(PBusDat[2] == 0xC2) )
	{
		PBus_Count = 0x00;
		g_RUNDate[_LogitADD][0] = 0xAA;
		ReTemp = PBusDat[8];		//������ȷ 
	}
	else	ReTemp = 0x00;	//���ݽ��մ���  a.��ʱ��b.���ݴ�
	return 	ReTemp;
}
//�۰���ҷ� ÿ��ֻ����һ����
//_Dat:���ұ��
uint8_t Binary_searchSN(void)
{	
	uint8_t uDat=0;
	uint8_t ucADDDat[4]={0x16,0x01,0x01,0x01};
	LED1 = 0;
	uDat = SendPBus_SearchDat(0xB1,(uint8_t *)ucADDDat);
	LED1 = 1;
	return uDat;
}

void PrintfDat(void)
{
	uint8_t i,j;
	printf("\r\n");
	for(i=0;i<26;i++)
	{
		printf("%02d--",i);
		for(j=0;j<16;j++)	printf("%02X,",g_RUNDate[i][j]);
		printf("\r\n");
	}
}
//void PrintfDat2(void)
//{
//	uint8_t i,j;
//	printf("\r\n");
//	for(i=0;i<30;i++)
//	{
//		printf("%02d--",i);
//		//for(j=0;j<16;j++)	printf("%02X,",g_SendDate[i][j]);
//		printf("\r\n");
//	}
//}


