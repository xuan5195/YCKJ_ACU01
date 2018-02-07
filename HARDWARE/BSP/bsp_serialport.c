#include "bsp_serialport.h"
#include "bsp_uart_fifo.h"
#include "bsp_crc8.h"
#include "bsp_useadd.h"

#include "delay.h"
#include "sys.h"
#include "string.h"
#include "includes.h"
#include "led.h"
#include "bsp_spi_flash.h"
#include "bsp_spi_bus.h"



extern uint8_t FM1702KeyCRC;
extern uint8_t FM1702_Key[7];	//FM1702_Key[0]-[5]ΪKey;FM1702_Key[6]Ϊ���ַ��
extern uint8_t g_ACUSN[4];		//���������SN 4λ

//extern uint8_t g_SetIPFlag;			//0x00:ʹ�þ�̬IP��ַ
extern uint8_t g_Setip[5];       	//����IP��ַ
extern uint8_t g_Setnetmask[4]; 	//��������
extern uint8_t g_Setgateway[4]; 	//Ĭ�����ص�IP��ַ
extern uint8_t g_SPI_Flash_Show,g_IAPFlag;			//���ڲ���ʹ��
uint8_t _hostname[60]={0};	//����

void SendSerialPort(uint8_t *SerialDat)
{
	uint8_t _hostnameLeng=0;
	uint8_t Runningbuf[20]={0x00};
	uint8_t Sendbuf[20]={0x00};
	uint8_t RFID_Key[7]={0x00};
	uint8_t _lwipADD[4]={0x00};
	uint8_t _ACUAdd[16]={0x00};
	uint8_t _Setip[5];       	//����IP��ַ g_Setip[0] Ϊʹ�ñ�־
	uint8_t _Setnetmask[4]; 	//��������
	uint8_t _Setgateway[4]; 	//Ĭ�����ص�IP��ַ
	uint16_t _lwipPort=0;
	uint8_t i=0;
	Runningbuf[0] = 0xF4;
	Runningbuf[1] = SerialDat[1];	//����
	Runningbuf[2] = SerialDat[2]+1;	//����֡
	switch(SerialDat[2])
	{
		case 0x01:	//ͨ�ż��
			if(SerialDat[1]==0x09)
			{
				Runningbuf[3] = VersionNo;	//������
				Runningbuf[4] = g_ACUSN[0];	//������ ���������SN
				Runningbuf[5] = g_ACUSN[1];	//������ ���������SN
				Runningbuf[6] = g_ACUSN[2];	//������ ���������SN
				Runningbuf[7] = g_ACUSN[3];	//������ ���������SN
				printf("ͨ�ż��%02X.%02X.%02X.%02X;\r\n",Runningbuf[4],Runningbuf[5],Runningbuf[6],Runningbuf[7]);
			}
			else if(SerialDat[1]==0x0A)
			{
				Runningbuf[1] = SerialDat[1]+1;	//����
				Runningbuf[3] = VersionNo;	//������
				Runningbuf[4] = g_ACUSN[0];	//������ ���������SN
				Runningbuf[5] = g_ACUSN[1];	//������ ���������SN
				Runningbuf[6] = g_ACUSN[2];	//������ ���������SN
				Runningbuf[7] = g_ACUSN[3];	//������ ���������SN
				Runningbuf[8] = 0xAA;		//������ ʹ���������
				printf("ͨ�ż��%02X.%02X.%02X.%02X;������ǣ�%02X;\r\n",Runningbuf[4],Runningbuf[5],Runningbuf[6],Runningbuf[7],Runningbuf[8]);
			}
			break;
		case 0x03:	//IP��ַ�Ͷ˿ں�
//		#if LWIP_DNS	//DNS
			if(SerialDat[1]==0x06)
			{
				Runningbuf[1] = 0x06;			//֡���� 
				Runningbuf[3] = SerialDat[3];	//������
				Runningbuf[4] = SerialDat[4];	//������
				Write_PortAdd((Runningbuf[3]<<8)|Runningbuf[4]);	//�˿ں�		
			}
//			else
//			{
//				Runningbuf[1] = 0x06;	//֡���� 
//				Runningbuf[3] = 0x00;	//������
//				Runningbuf[4] = 0x00;	//������
//			}
			printf("�˿ں�%02X.%02X;\r\n",Runningbuf[3],Runningbuf[4]);
//		#else
//			Runningbuf[3] = SerialDat[3];	//������
//			Runningbuf[4] = SerialDat[4];	//������
//			Runningbuf[5] = SerialDat[5];	//������
//			Runningbuf[6] = SerialDat[6];	//������
//			Runningbuf[7] = SerialDat[7];	//������
//			Runningbuf[8] = SerialDat[8];	//������
//			Sendbuf[0] = Runningbuf[3];
//			Sendbuf[1] = Runningbuf[4];
//			Sendbuf[2] = Runningbuf[5];
//			Sendbuf[3] = Runningbuf[6];			
//			Write_IPAdd((uint8_t *)Sendbuf);		//IP��ַ
//			Write_PortAdd((Runningbuf[7]<<8)|Runningbuf[8]);	//�˿ں�		
//		#endif
			break;
		case 0xC3:	//��ѯ  IP��ַ�Ͷ˿ں�
		#if LWIP_DNS	//DNS
			_lwipPort = Read_PortAdd();				//printf("�˿ں�------------%5d\r\n",_lwipPort);
			Runningbuf[3] = _lwipPort/256;	//������
			Runningbuf[4] = _lwipPort%256;	//������
		#else
			Read_IPAdd((uint8_t *)_lwipADD);		//printf("IP��ַ------------%d.%d.%d.%d\r\n",g_lwipADD[0],g_lwipADD[1],g_lwipADD[2],g_lwipADD[3]);
			_lwipPort = Read_PortAdd();				//printf("�˿ں�------------%5d\r\n",g_lwipPort);
			Runningbuf[3] = _lwipADD[0];	//������
			Runningbuf[4] = _lwipADD[1];	//������
			Runningbuf[5] = _lwipADD[2];	//������
			Runningbuf[6] = _lwipADD[3];	//������
			Runningbuf[7] = _lwipPort/256;	//������
			Runningbuf[8] = _lwipPort%256;	//������
		#endif
			break;
		case 0x05:	//ͨѶ��
			for(i=0;i<16;i++)
			{
				Runningbuf[3+i] = SerialDat[3+i];	//������01
				Sendbuf[i] = SerialDat[3+i];	//������01
			}
			Write_ACUAdd((uint8_t *)Sendbuf);			
			break;
		case 0xC5:	//��ѯ  ͨѶ��
			Read_ACUAdd((uint8_t *)_ACUAdd);
			for(i=0;i<16;i++)
			{
				Runningbuf[3+i] = _ACUAdd[i];	//������
			}
			break;
		case 0x07:	//���������SN
			for(i=0;i<4;i++)
			{
				Runningbuf[3+i] = SerialDat[3+i];	//������01
				Sendbuf[i] = SerialDat[3+i];	//������01
			}
			Write_ACUSN((uint8_t *)Sendbuf);		//���������SN
			break;
		case 0x09:	//��̬IP��������
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
			printf("---------���յ�����---------\r\n");
			if( g_Setip[0] == 0xAA )	printf("��̬IP---����\r\n");	else printf("��̬IP---����\r\n");
			printf("��̬IP��ַ......%d.%d.%d.%d\r\n",g_Setip[1],g_Setip[2],g_Setip[3],g_Setip[4]);
			printf("��������........%d.%d.%d.%d\r\n",g_Setnetmask[0],g_Setnetmask[1],g_Setnetmask[2],g_Setnetmask[3]);
			printf("Ĭ������........%d.%d.%d.%d\r\n",g_Setgateway[0],g_Setgateway[1],g_Setgateway[2],g_Setgateway[3]);
			break;
		case 0xC9:	//��ѯ  ��̬IP��������
			Read_localIP((uint8_t *)_Setip);	//��̬IP��־ 0xAAΪ��̬IP
			Read_netmask((uint8_t *)_Setnetmask);
			Read_gateway((uint8_t *)_Setgateway);
			Runningbuf[3] = _Setip[0];
			Runningbuf[4] = _Setip[1];			Runningbuf[5] = _Setip[2]; 			Runningbuf[6] = _Setip[3]; 			Runningbuf[7] = _Setip[4];
			Runningbuf[8] = _Setnetmask[0];		Runningbuf[9] = _Setnetmask[1]; 	Runningbuf[10] = _Setnetmask[2]; 	Runningbuf[11] = _Setnetmask[3];
			Runningbuf[12] = _Setgateway[0];	Runningbuf[13] = _Setgateway[1];	Runningbuf[14] = _Setgateway[2];	Runningbuf[15] = _Setgateway[3];
			break;
		case 0x10:	//����RFID_Key
			FM1702_Key[0] = SerialDat[3];	FM1702_Key[1] = SerialDat[4]; 	FM1702_Key[2] = SerialDat[5];	//FM1702_Key
			FM1702_Key[3] = SerialDat[6];	FM1702_Key[4] = SerialDat[7]; 	FM1702_Key[5] = SerialDat[8];
			FM1702_Key[6] = SerialDat[10];	//���ַ
			FM1702KeyCRC = CRC8_Table(FM1702_Key,0x07);	//CRCУ��λ
			Write_RFID_Key((uint8_t *)FM1702_Key);
			printf("---------���յ����� RFID_Key %2X%2X%2X%2X%2X%2X,���ַ��%d\r\n",FM1702_Key[0],FM1702_Key[1],FM1702_Key[2],FM1702_Key[3],FM1702_Key[4],FM1702_Key[5],FM1702_Key[6]);
		case 0x12:	//��ѯRFID_Key
			Read_RFID_Key((uint8_t *)RFID_Key);
			Runningbuf[3] = RFID_Key[0];	//������ RFID_Key1
			Runningbuf[4] = RFID_Key[1];	//������ RFID_Key2
			Runningbuf[5] = RFID_Key[2];	//������ RFID_Key3
			Runningbuf[6] = RFID_Key[3];	//������ RFID_Key4
			Runningbuf[7] = RFID_Key[4];	//������ RFID_Key5
			Runningbuf[8] = RFID_Key[5];	//������ RFID_Key6
			Runningbuf[9] = SerialDat[9];	//������ A���� 0x60
			Runningbuf[10] = RFID_Key[6];	//������ ���ַ
			break;	
		case 0x21:	//��������1
			Clear_hostname();
			printf("\r\nSerialDat: ");
			for(i=0;i<(SerialDat[1]-4);i++)
			{
				_hostname[i] = SerialDat[3+i];
				Runningbuf[3+i] = _hostname[i];
//				printf("0x%02X ",SerialDat[3+i]);
			}
//	printf("\r\n _hostname \r\n");
//	for(i=0;i<(_hostname[0]+1);i++)	printf("0x%02X ",_hostname[i]);
//	printf("\r\n");
//	printf("_hostname:%s \r\n",_hostname);
			if(_hostname[0]<=27)	Write_hostname((uint8_t *)_hostname);
			Runningbuf[1] = SerialDat[1];	//����
			Runningbuf[2] = SerialDat[2];	//����֡
			break;	
		case 0x22:	//��������2
			for(i=0;i<(SerialDat[1]-5);i++)
			{
				_hostname[i+28] = SerialDat[4+i];
				Runningbuf[4+i] = _hostname[i+28];
			}
			Runningbuf[1] = SerialDat[1];	//����
			Runningbuf[2] = SerialDat[2];	//����֡
			Runningbuf[3] = SerialDat[3];	//�����ܳ���
			Write_hostname((uint8_t *)_hostname);
			break;	
		case 0x23:	//��������3
			break;	
		case 0x31:	//��ѯ����1
			_hostnameLeng = Read_hostnameLeng();
			Read_hostname((uint8_t *)_hostname);	printf("\r\ng_hostname:%s;\r\n",_hostname);
			if( _hostnameLeng > 28 )
			{
				Runningbuf[1] = 27+5;	//�����ܳ���
				for(i=0;i<28;i++)	Runningbuf[3+i] = _hostname[i];
			}
			else
			{
				Runningbuf[1] = _hostnameLeng+5;	//�����ܳ���
				for(i=0;i<_hostnameLeng;i++) 	Runningbuf[3+i] = _hostname[i];
			}
			Runningbuf[2] = SerialDat[2];	//����֡
			break;	
		case 0x32:	//��ѯ����2
			_hostnameLeng = Read_hostnameLeng();
			Runningbuf[1] = _hostnameLeng-27+4;	//����
			Runningbuf[2] = SerialDat[2];		//����֡
			for(i=0;i<(_hostnameLeng-27);i++)	Runningbuf[3+i] = _hostname[i+28];
			break;	
 		case 0x33:	//��ѯ����3
			break;	
		case 0xD0:	//IAP������
			//g_SPI_Flash_Show = 0xAA;
			g_IAPFlag = 0xD0;
			break;	
		case 0xD1:	//����FLashSPI_Flash����
			g_IAPFlag = 0xD1;
			break;	
		case 0xD2:	//��ʾFLash������
			g_IAPFlag = 0xD2;
			break;	
		case 0xD3:	//��ת����
			g_IAPFlag = 0xD3;
			break;	        
		case 0xD4:	//��λ����
			g_IAPFlag = 0xD4;
			break;	        
//		case 0xEE:	//״̬�ظ�������ģ���ʼ�����
//			Runningbuf[2] = 0xEE;
//			break;	        
//		case 0xE4:	//״̬�ظ���������λ���
//			Runningbuf[2] = 0xE4;
//			break;	        
//		case 0xE1:	//״̬�ظ�����������FLash���
//			Runningbuf[2] = 0xE4;
//			break;	        
//		case 0xE0:	//״̬�ظ��������������
//			Runningbuf[2] = 0xE0;
//			break;	        
//		case 0xE3:	//״̬�ظ�����ת���
//			Runningbuf[2] = 0xE3;
//			break;	        
		default:
        	break;
	}
	if(SerialDat[2]<=0xCF)
	{
		Runningbuf[SerialDat[1]-1] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRCУ��λ
		comSendBuf(SERIALPORT_COM, (uint8_t *)Runningbuf, Runningbuf[1]);
	}
}

void ReceivePacketDat(uint8_t *SerialDat)
{
	uint16_t i,LineNo;
	uint8_t Runningbuf[4]={0xF2,0x00,0x00,0x00};
	if(SerialDat[1]==0x00)	//��0���ݰ������ݰ�����
	{
		Runningbuf[2] = 0xAA;
		printf("���ݰ�����%02d;\r\n", SerialDat[2]);
//		g_PacketNo = SerialDat[2];		//���ݰ�����
		sf_EraseSector(0x010000);		//�������� 16 4K
		sf_EraseSector(0x020000);		//�������� 17 4K
		sf_EraseSector(0x030000);		//�������� 18 4K
		sf_EraseSector(0x040000);		//�������� 19 4K
		sf_EraseSector(0x050000);		//�������� 20 4K
		sf_EraseSector(0x060000);		//�������� 21 4K
		sf_EraseSector(0x070000);		//�������� 22 4K
		sf_EraseSector(0x080000);		//�������� 23 4K
	}
	else
	{//���ݰ��洢
		printf("%02X %02X\r\n", SerialDat[0], SerialDat[1]);
		LineNo = 1;
		printf("%02d: ",LineNo);
		for (i = 2; i < 131; i++)
		{
			printf("%02X", SerialDat[i]);
			if ( i%2 == 1)         		printf(" ");	/* ÿ����ʾ16�ֽ����� */
			if (((i-2) & 15) == 15)		printf("\r\n%02d: ",(++LineNo));
		}
		printf("\r\n----CRC = %02X\r\n", SerialDat[131]);
		if (sf_WriteBuffer((SerialDat+2), 0x010000+128*(SerialDat[1]-1), 512) == 0)
		{			
			printf("д����Flash����\r\n");
			Runningbuf[2] = 0x00;
		}
		else
		{
			printf("д����Flash�ɹ���\r\n");
			Runningbuf[2] = 0xAA;
		}
	}
	Runningbuf[1] = SerialDat[1];
	Runningbuf[3] = CRC8_Table(Runningbuf,3);	//CRCУ��λ
	comSendBuf(SERIALPORT_COM, (uint8_t *)Runningbuf, 4);		
}

void SendSerialAsk(uint8_t _Dat)
{
	uint8_t Runningbuf[4]={0x00};
	Runningbuf[0] = 0xF4;
	Runningbuf[1] = 0x04;	//����
	Runningbuf[2] = _Dat;	//����֡
	Runningbuf[Runningbuf[1]-1] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRCУ��λ
	comSendBuf(SERIALPORT_COM, (uint8_t *)Runningbuf, Runningbuf[1]);
}

