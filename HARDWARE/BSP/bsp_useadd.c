#include "bsp_useadd.h"
#include "bsp_24cxx.h"
#include "bsp_myiic.h"
#include "bsp_powerbus.h"
#include "bsp_uart_fifo.h"

#include "delay.h"
#include "sys.h"
#include "string.h"
//#include "usmart.h"	
#include "includes.h"
#include "bsp_canapp.h"

#define ADD_SIZE	BUSNUM_SIZE
extern uint8_t g_RUNDate[BUSNUM_SIZE+1][14];    //�������ݣ�
extern uint8_t KJ_Versions[BUSNUM_SIZE];	//�����汾��

/*
*********************************************************************************************************
*	�� �� ��: Distribute_LogicADD(uint8_t Logic_no,uint8_t *Physical_no)
*	����˵��: Ϊ�����ַ�����߼���ַ��д��Flash
*	��    ��: 
*	�� �� ֵ: 
*********************************************************************************************************
*/
uint8_t Distribute_LogicADD(uint8_t *Physical_no)
{
	uint8_t i,Logic_Count;
	uint8_t u8Temp;
	uint16_t Temp;
	uint8_t Physical_Temp[5]={0};
	//1.���жϸ������ַ�Ƿ��ڱ���ֻ�ѷ����߼���ַ��
	for(i=1;i<(ADD_SIZE+1);i++)
	{
        Temp = (Start_ADD + ( i * 5 ))|Logic_ADD ;	//ÿ�����ʼ��ַ
		u8Temp = AT24CXX_ReadOneByte(Temp);			//ÿ����߼���ַ
		if( u8Temp == i )		//��������ַʹ���ˣ�
        {
			AT24CXX_Read( (Temp + 1),(uint8_t *)Physical_Temp, 0x04 );
            if((*(Physical_no+3) == Physical_Temp[3])&&(*(Physical_no+2) == Physical_Temp[2]))
            {
                if((*(Physical_no+0) == Physical_Temp[0])&&(*(Physical_no+1) == Physical_Temp[1]))
                    return	i; 	//���ظ������ַ���߼���ַ���˳�
            }

        }
	}
    
	//Ϊ�������ַ�����߼���ַ
	for(i=1;i<(ADD_SIZE+1);i++)   
	{
		Temp = (Start_ADD + ( i * 5 ))|Logic_ADD ;
		u8Temp = AT24CXX_ReadOneByte( Temp );
		if( u8Temp != i) //��������ַδʹ�ã�
		{
			AT24CXX_WriteOneByte( Temp, i );            
			AT24CXX_WriteOneByte( Temp+1, Physical_no[0] );            
			AT24CXX_WriteOneByte( Temp+2, Physical_no[1] );            
			AT24CXX_WriteOneByte( Temp+3, Physical_no[2] );            
			AT24CXX_WriteOneByte( Temp+4, Physical_no[3] );            
			Logic_Count = AT24CXX_ReadOneByte(COUNT_ADD);   //��ȡ����¼�� ������
			AT24CXX_WriteOneByte(COUNT_ADD,Logic_Count+1);
			return i;
		}
	}
    
	return 0xF3;//	�������
} 

/*
*********************************************************************************************************
*	�� �� ��: Delete_LogicADD((uint8_t Logic_no)
*	����˵��: ɾ��ָ�����߼���ַ��Ϣ
*	��    ��: 
*	�� �� ֵ: 
*********************************************************************************************************
*/
uint8_t Delete_LogicADD(uint8_t Logic_no)
{
	uint8_t i,Logic_Count;
	uint8_t Physical_no[5]={0};
    for(i=1;i<(ADD_SIZE+1);i++)
    {
        if( AT24CXX_ReadOneByte( (Start_ADD + ( i * 5 ))|Logic_ADD ) == Logic_no)
        {
            AT24CXX_Write((Start_ADD + ( i * 5 )) | Physical_ADD1, Physical_no, 0x04);
            AT24CXX_WriteOneByte( (Start_ADD + ( i * 5 )) | Logic_ADD, 0x00 );            
        }
    }
	g_RUNDate[Logic_no][0] = 0x00;
	Logic_Count = AT24CXX_ReadOneByte(COUNT_ADD);   //��ȡ����¼�� ������
	AT24CXX_WriteOneByte(COUNT_ADD,Logic_Count-1);
	return (Logic_Count-1);
}


/*
*********************************************************************************************************
*	�� �� ��: PowerUPLogitADDCheck((uint8_t Logic_no)
*	����˵��: �ϵ���Flash�������Ƿ����߲������߼���ַ
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void PowerUPLogitADDCheck(void)
{
	uint8_t i;
	uint16_t Temp;
	uint8_t Physical_Temp[5]={0};
	for(i=1;i<(ADD_SIZE+1);i++)
	{
        Temp = (Start_ADD + ( i * 5 ))|Logic_ADD ;	//ÿ�����ʼ��ַ  �߼���ַ���ݴ洢��ַ
		if( AT24CXX_ReadOneByte(Temp) == i )		//��������ַʹ���ˣ�
		{
			AT24CXX_Read( (Temp + 1),(uint8_t *)Physical_Temp, 0x04 );	//��ȡ�����ַ
            printf("++��ʼ:%03d; �߼�:%02d; ����%02X%02X%02X%02X; ",Temp,i,Physical_Temp[0],Physical_Temp[1],Physical_Temp[2],Physical_Temp[3]);
			if((Physical_Temp[0]==0x00)&&(Physical_Temp[1]==0x00)&&(Physical_Temp[2]==0x00)&&(Physical_Temp[3]==0x00))
			{
				printf("\r\n�����ַ����Ϊ0x00�����߼���ַ:%d;\r\n",i);               
				Delete_LogicADD(i);	//���䲻�ɹ���������߼���ַ	
				KJ_Versions[i]=0;				
			}
			else
			{
				KJ_Versions[i] = Can_WriteLogitADD(i,(uint8_t *)Physical_Temp);
				if(KJ_Versions[i] != 0x00)	//�ֱ��߼���ַ�ɹ�
				{
					++g_RUNDate[0][0]; 		//����+1���õ�ַʹ�ã�
					g_RUNDate[i][0] = 0x80;	//����+1���õ�ַʹ�ã�
					g_RUNDate[i][1] = Physical_Temp[0]; g_RUNDate[i][2] = Physical_Temp[1];	//����SN
					g_RUNDate[i][3] = Physical_Temp[2]; g_RUNDate[i][4] = Physical_Temp[3];
					printf("Ver[%02d]=%02X\r\n",i,KJ_Versions[i]);               
				}
				else
				{
					printf("------����ʧ��!\r\n");
					OSTimeDlyHMSM(0, 0, 0, 50);					
					KJ_Versions[i] = Can_WriteLogitADD(i,(uint8_t *)Physical_Temp);
					if(KJ_Versions[i] != 0x00)	//�ֱ��߼���ַ�ɹ�
					{
						++g_RUNDate[0][0]; 		//����+1���õ�ַʹ�ã�
						g_RUNDate[i][0] = 0x80;	//����+1���õ�ַʹ�ã�
						g_RUNDate[i][1] = Physical_Temp[0]; g_RUNDate[i][2] = Physical_Temp[1];	//����SN
						g_RUNDate[i][3] = Physical_Temp[2]; g_RUNDate[i][4] = Physical_Temp[3];
						printf("Ver[%02d]=%02X,����ɹ���\r\n",i,KJ_Versions[i]);               
					}
					else
					{
						printf("���η���ʧ�ܣ�\r\n----------------------------------���߼���ַ:%d;----------------------\r\n",i);               
						Delete_LogicADD(i);	//���䲻�ɹ���������߼���ַ					
					}
				}
			}
		}
		OSTimeDlyHMSM(0,0,0,200);
	}
}

////����ͨ����̵��磬�ظ�ע�᣻
//void LogitADDWrite(uint8_t _AddDat)
//{
//	uint8_t i;
//	uint16_t Temp;
//	uint8_t Physical_Temp[5]={0};
//	i = _AddDat;
//	Temp = (Start_ADD + ( i * 5 ))|Logic_ADD ;	//ÿ�����ʼ��ַ  �߼���ַ���ݴ洢��ַ
//	if( AT24CXX_ReadOneByte(Temp) == i )		//��������ַʹ���ˣ�
//	{
//		AT24CXX_Read( (Temp + 1),(uint8_t *)Physical_Temp, 0x04 );	//��ȡ�����ַ
//		printf("++��ʼ��ַ:%d;  �߼���ַ:%d;  �����ַ��%02X%02X%02X%02X;  \r\n",Temp,i,Physical_Temp[0],Physical_Temp[1],Physical_Temp[2],Physical_Temp[3]);
//		KJ_Versions[i] = WriteLogitADD(i,(uint8_t *)Physical_Temp);
//		if(KJ_Versions[i] != 0x00)	//�ֱ��߼���ַ�ɹ�
//		//if(WriteLogitADD(i,(uint8_t *)Physical_Temp) != 0x00)	//�ֱ��߼���ַ�ɹ�
//		{
//			++g_RUNDate[0][0]; g_RUNDate[i][0] = 0xAA;	//����+1���õ�ַʹ�ã�
//			g_RUNDate[i][3] = Physical_Temp[0]; g_RUNDate[i][4] = Physical_Temp[1];	//����SN
//			g_RUNDate[i][5] = Physical_Temp[2]; g_RUNDate[i][6] = Physical_Temp[3];
//			printf("����ɹ���\r\n");               
//		}
//		else
//		{
//			printf("����ʧ�ܣ�\r\n");               
//		}
//	}
//}

void Read_RFID_Key(uint8_t *RFID_Key_Temp)
{
	AT24CXX_Read( 930,(uint8_t *)RFID_Key_Temp, 0x07 );	//��ȡRFID_Key����
}

void Write_RFID_Key(uint8_t *RFID_Key_Temp)//RFID_Key����
{
	AT24CXX_WriteOneByte( 930, RFID_Key_Temp[0] );            
	AT24CXX_WriteOneByte( 931, RFID_Key_Temp[1] );            
	AT24CXX_WriteOneByte( 932, RFID_Key_Temp[2] );            
	AT24CXX_WriteOneByte( 933, RFID_Key_Temp[3] );            
	AT24CXX_WriteOneByte( 934, RFID_Key_Temp[4] );            
	AT24CXX_WriteOneByte( 935, RFID_Key_Temp[5] );            
	AT24CXX_WriteOneByte( 936, RFID_Key_Temp[6] );            
}
void Read_IPAdd(uint8_t *IPAdd_Temp)
{
	AT24CXX_Read( 900,(uint8_t *)IPAdd_Temp, 0x04 );	//��ȡIP��ַ
}

void Write_IPAdd(uint8_t *IPAdd_Temp)//IP��ַ
{
	AT24CXX_WriteOneByte( 900, IPAdd_Temp[0] );            
	AT24CXX_WriteOneByte( 901, IPAdd_Temp[1] );            
	AT24CXX_WriteOneByte( 902, IPAdd_Temp[2] );            
	AT24CXX_WriteOneByte( 903, IPAdd_Temp[3] );            
}
uint16_t Read_PortAdd(void) //�˿ں�
{
	uint8_t _uTemp[2]={0};
	AT24CXX_Read( 904,(uint8_t *)_uTemp, 0x02 );	
	return ((_uTemp[0]<<8)|_uTemp[1]);
}

void Write_PortAdd(uint16_t PortAdd_Temp)	//�˿ں�
{
	uint16_t Temp = 904 ;
	AT24CXX_WriteOneByte( Temp+0, (PortAdd_Temp/256) );            
	AT24CXX_WriteOneByte( Temp+1, (PortAdd_Temp%256) );            
}
void Read_ACUSN(uint8_t *ACUSN_Temp)//���������SN
{
	AT24CXX_Read( 906,(uint8_t *)ACUSN_Temp, 0x04 );	
}

void Write_ACUSN(uint8_t *ACUSN_Temp)	//���������SN
{
	uint16_t Temp = 906 ;
	AT24CXX_WriteOneByte( Temp+0, ACUSN_Temp[0] );            
	AT24CXX_WriteOneByte( Temp+1, ACUSN_Temp[1] );            
	AT24CXX_WriteOneByte( Temp+2, ACUSN_Temp[2] );            
	AT24CXX_WriteOneByte( Temp+3, ACUSN_Temp[3] );            
}
void Read_ACUAdd(uint8_t *ACUAdd_Temp)//ͨ���� 16λ
{
	AT24CXX_Read( 910,(uint8_t *)ACUAdd_Temp, 16 );	
}

void Write_ACUAdd(uint8_t *ACUAdd_Temp)//ͨ���� 16λ
{
	uint16_t Temp = 910 ;
	AT24CXX_WriteOneByte( Temp+0, ACUAdd_Temp[0] );            
	AT24CXX_WriteOneByte( Temp+1, ACUAdd_Temp[1] );            
	AT24CXX_WriteOneByte( Temp+2, ACUAdd_Temp[2] );            
	AT24CXX_WriteOneByte( Temp+3, ACUAdd_Temp[3] );            
	AT24CXX_WriteOneByte( Temp+4, ACUAdd_Temp[4] );            
	AT24CXX_WriteOneByte( Temp+5, ACUAdd_Temp[5] );            
	AT24CXX_WriteOneByte( Temp+6, ACUAdd_Temp[6] );            
	AT24CXX_WriteOneByte( Temp+7, ACUAdd_Temp[7] );            
	AT24CXX_WriteOneByte( Temp+8, ACUAdd_Temp[8] );            
	AT24CXX_WriteOneByte( Temp+9, ACUAdd_Temp[9] );            
	AT24CXX_WriteOneByte( Temp+10, ACUAdd_Temp[10] );            
	AT24CXX_WriteOneByte( Temp+11, ACUAdd_Temp[11] );            
	AT24CXX_WriteOneByte( Temp+12, ACUAdd_Temp[12] );            
	AT24CXX_WriteOneByte( Temp+13, ACUAdd_Temp[13] );            
	AT24CXX_WriteOneByte( Temp+14, ACUAdd_Temp[14] );            
	AT24CXX_WriteOneByte( Temp+15, ACUAdd_Temp[15] );            
}
void Write_localIP(uint8_t *_Temp)	//����IP
{
	uint16_t Temp = 939 ;
	AT24CXX_WriteOneByte( Temp+0, _Temp[0] );            
	AT24CXX_WriteOneByte( Temp+1, _Temp[1] );            
	AT24CXX_WriteOneByte( Temp+2, _Temp[2] );            
	AT24CXX_WriteOneByte( Temp+3, _Temp[3] );            
	AT24CXX_WriteOneByte( Temp+4, _Temp[4] );            
}
void Write_netmask(uint8_t *_Temp)	//��������:255.255.255.0
{
	uint16_t Temp = 944 ;
	AT24CXX_WriteOneByte( Temp+0, _Temp[0] );            
	AT24CXX_WriteOneByte( Temp+1, _Temp[1] );            
	AT24CXX_WriteOneByte( Temp+2, _Temp[2] );            
	AT24CXX_WriteOneByte( Temp+3, _Temp[3] );            
}
void Write_gateway(uint8_t *_Temp)	//Ĭ������:192.168.1.1
{
	uint16_t Temp = 948 ;
	AT24CXX_WriteOneByte( Temp+0, _Temp[0] );            
	AT24CXX_WriteOneByte( Temp+1, _Temp[1] );            
	AT24CXX_WriteOneByte( Temp+2, _Temp[2] );            
	AT24CXX_WriteOneByte( Temp+3, _Temp[3] );            
}
void Read_localIP(uint8_t *_Temp)
{
	AT24CXX_Read( 939,(uint8_t *)_Temp, 0x05 );	//��ȡ����IP��ַ
}
void Read_netmask(uint8_t *_Temp)
{
	AT24CXX_Read( 944,(uint8_t *)_Temp, 0x04 );	//��������
}
void Read_gateway(uint8_t *_Temp)
{
	AT24CXX_Read( 948,(uint8_t *)_Temp, 0x04 );	//Ĭ������
}

