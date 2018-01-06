/*
* Copyright (c) 2014,������ɷ����޹�˾���Ӳ�
* All right reserved.
*
* �ļ����ƣ�led.c
* �ļ���ʶ��100PCS�����ļ�
* ժ    Ҫ��
*       	���ļ���Ҫ������AT24CXX�ⲿ�洢����IICͨ�ţ�������myiic�ļ��Ľӿ��ϵȹ��ܡ�
*		������
		u8 		AT24CXX_ReadOneByte(u16 ReadAddr);							//ָ����ַ��ȡһ���ֽ�
		void 	AT24CXX_WriteOneByte(u16 WriteAddr,u8 DataToWrite);			//ָ����ַд��һ���ֽ�
		void 	AT24CXX_WriteLenByte(u16 WriteAddr,u32 DataToWrite,u8 Len);	//ָ����ַ��ʼд��ָ�����ȵ�����
		u32 	AT24CXX_ReadLenByte(u16 ReadAddr,u8 Len);					//ָ����ַ��ʼ��ȡָ����������
		void 	AT24CXX_Write(u16 WriteAddr,u8 *pBuffer,u16 NumToWrite);	//��ָ����ַ��ʼд��ָ�����ȵ�����
		void 	AT24CXX_Read(u16 ReadAddr,u8 *pBuffer,u16 NumToRead);   	//��ָ����ַ��ʼ����ָ�����ȵ�����

*
* �ļ���ʷ:
* �汾��  	����       	����    	˵��
* V0.1    	20120909 	����ԭ��  	�������ļ�
* V1.1   	20140826 	���  		ʹ����GAD-DL02���� ������
*
*/

#include "stm32f10x.h"
#include "sys.h" 
#include "delay.h"
#include "bsp_24cxx.h"
#include "bsp_myiic.h"
#include "bsp_uart_fifo.h"
#include "bsp_useadd.h"
extern uint8_t g_lwipADD[4];	//Զ��IP��ַ
extern uint16_t g_lwipPort;		//Զ�˶˿ں�
extern uint8_t g_ACUSN[4];		//���������SN 4λ
extern uint8_t g_ACUAdd[16];	//ͨ����16λ
extern uint8_t FM1702_Key[7];	//FM1702_Key[0]-[5]ΪKey;FM1702_Key[6]Ϊ���ַ��
extern uint8_t g_Setip[5];       	//����IP��ַ
extern uint8_t g_Setnetmask[4]; 	//��������
extern uint8_t g_Setgateway[4]; 	//Ĭ�����ص�IP��ַ

/*
*********************************************************************************************************
*                                            AT24CXX_Init()
*
* Description : AT24CXX initialize.
*
* Argument : none.
*
* Return   : none.
*
* Caller   : none.
*
* Note     : none.
*********************************************************************************************************
*/
void AT24CXX_Init(void)
{
	IIC_Init();
}
/*
*********************************************************************************************************
*                                            AT24CXX_ReadOneByte()
*
* Description : AT24CXX read a one byte.
*
* Argument : read a byte.
*
* Return   : form the readAddr to read  Data .
*
* Caller   : ReadAddr.
*
* Note     : none.
*********************************************************************************************************
*/
u8 AT24CXX_ReadOneByte(u16 ReadAddr)
{				  
	u8 temp=0;		  	    																 
    IIC_Start();  
	if(EE_TYPE>AT24C16)
	{
		IIC_Send_Byte(0XA0);	   //����д����
		IIC_Wait_Ack();
		IIC_Send_Byte(ReadAddr>>8);//���͸ߵ�ַ
		IIC_Wait_Ack();		 
	}else IIC_Send_Byte(0XA0+((ReadAddr/256)<<1));   //����������ַ0XA0,д���� 	 

	IIC_Wait_Ack(); 
    IIC_Send_Byte(ReadAddr%256);   //���͵͵�ַ
	IIC_Wait_Ack();	    
	IIC_Start();  	 	   
	IIC_Send_Byte(0XA1);           //�������ģʽ			   
	IIC_Wait_Ack();	 
    temp=IIC_Read_Byte(0);		   
    IIC_Stop();//����һ��ֹͣ����	    
	return temp;
}
/*
*********************************************************************************************************
*                            void AT24CXX_WriteOneByte(u16 WriteAddr,u8 DataToWrite)
*
* Description : .
*
* Argument : none.
*
* Return   : none.
*
* Caller   : none.
*
* Note     : none.
*********************************************************************************************************
*/
void AT24CXX_WriteOneByte(u16 WriteAddr,u8 DataToWrite)
{				   	  	    																 
    IIC_Start();  
	if(EE_TYPE>AT24C16)
	{
		IIC_Send_Byte(0XA0);	    //����д����
		IIC_Wait_Ack();
		IIC_Send_Byte(WriteAddr>>8);//���͸ߵ�ַ
 	}else
	{
		IIC_Send_Byte(0XA0+((WriteAddr/256)<<1));   //����������ַ0XA0,д���� 
	}	 
	IIC_Wait_Ack();	   
    IIC_Send_Byte(WriteAddr%256);   //���͵͵�ַ
	IIC_Wait_Ack(); 	 										  		   
	IIC_Send_Byte(DataToWrite);     //�����ֽ�							   
	IIC_Wait_Ack();  		    	   
    IIC_Stop();//����һ��ֹͣ���� 
	//OSTimeDlyHMSM(0, 0, 0, 10);
    delay_ms(10);
    
}
//��AT24CXX�����ָ����ַ��ʼд�볤��ΪLen������
//�ú�������д��16bit����32bit������.
//WriteAddr  :��ʼд��ĵ�ַ  
//DataToWrite:���������׵�ַ
//Len        :Ҫд�����ݵĳ���2,4
void AT24CXX_WriteLenByte(u16 WriteAddr,u32 DataToWrite,u8 Len)
{  	
	u8 t;
	for(t=0;t<Len;t++)
	{
		AT24CXX_WriteOneByte(WriteAddr+t,(DataToWrite>>(8*t))&0xff);
	}												    
}

//��AT24CXX�����ָ����ַ��ʼ��������ΪLen������
//�ú������ڶ���16bit����32bit������.
//ReadAddr   :��ʼ�����ĵ�ַ 
//����ֵ     :����
//Len        :Ҫ�������ݵĳ���2,4
u32 AT24CXX_ReadLenByte(u16 ReadAddr,u8 Len)
{  	
	u8 t;
	u32 temp=0;
	for(t=0;t<Len;t++)
	{
		temp<<=8;
		temp+=AT24CXX_ReadOneByte(ReadAddr+Len-t-1); 	 				   
	}
	return temp;												    
}
/*
24C08���ݴ洢	
	>>255	�洢ʹ�ñ�ʶ��						
	>>200	201	202	203	IP��ַ ����192.168.1.1			
	>>204	205	�˿ںţ�20000(4E20)�� 205���4E��206���20					
	>>206	207	208	209	������������SN			
	>>210	-225	���ͨ����					
	>>230	231	232	233	234	235	���RFID_KEY	
	>>236	���RFID_Key���Կ�ţ�						
								
	>>10	14	����߼���ַΪ1�������ַ					
	>>15	19	����߼���ַΪ2�������ַ					
	>>20	24	����߼���ַΪ3�������ַ					
	>>����	����						
	>>135	139	����߼���ַΪ25�������ַ					
*/

//���AT24CXX�Ƿ�����
//��������24XX�����һ����ַ(255)���洢��־��.
//���������24Cϵ��,�����ַҪ�޸�
//����1:���ʧ��
//����0:���ɹ�
u8 AT24CXX_Check(void)
{
	u8 temp;
	uint8_t ucRFID_Key_Temp[7]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x01};	//RFID_Key ��ʼֵ
	uint8_t ACUSN_Temp[4]={0x20,0x20,0x01,0x01};	//���������SN
	uint8_t IPAdd_Temp[4]={39,106,113,254};	//���������SN
	uint16_t PortAdd_Temp=20000;	//�˿ں�
	uint8_t ACUAdd_Temp[16]={0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x2D,0x62,0x37,0x36,0x31,0x63,0x32,0x62,0x30};	//���������SN

	Read_RFID_Key((uint8_t *)FM1702_Key);	printf("RFID_Key:%02X%02X%02X%02X%02X%02X.���Կ��:%d\r\n",FM1702_Key[0],FM1702_Key[1],FM1702_Key[2],FM1702_Key[3],FM1702_Key[4],FM1702_Key[5],FM1702_Key[6]);
	Read_IPAdd((uint8_t *)g_lwipADD);		printf("IP��ַ------------%d.%d.%d.%d\r\n",g_lwipADD[0],g_lwipADD[1],g_lwipADD[2],g_lwipADD[3]);
	g_lwipPort = Read_PortAdd();			printf("�˿ں�------------%5d\r\n",g_lwipPort);
	Read_ACUSN((uint8_t *)g_ACUSN);			printf("���������SN------%02X%02X%02X%02X\r\n",g_ACUSN[0],g_ACUSN[1],g_ACUSN[2],g_ACUSN[3]);
	Read_ACUAdd((uint8_t *)g_ACUAdd);		printf("���������ͨ����--");	
	printf("%2X%2X%2X%2X%2X%2X%2X%2X",		g_ACUAdd[0],g_ACUAdd[1],g_ACUAdd[2],g_ACUAdd[3],g_ACUAdd[4],g_ACUAdd[5],g_ACUAdd[6],g_ACUAdd[7]);
	printf("%2X%2X%2X%2X%2X%2X%2X%2X\r\n",	g_ACUAdd[8],g_ACUAdd[9],g_ACUAdd[10],g_ACUAdd[11],g_ACUAdd[12],g_ACUAdd[13],g_ACUAdd[14],g_ACUAdd[15]);

	Read_localIP((uint8_t *)g_Setip);	//��̬IP��־ 0xAAΪ��̬IP
	if(g_Setip[0]==0xAA)	
	{
		printf("��̬IP---����\r\n");	
		Read_netmask((uint8_t *)g_Setnetmask);
		Read_gateway((uint8_t *)g_Setgateway);
	}
	else printf("��̬IP---����\r\n");
	temp=AT24CXX_ReadOneByte(255);//����ÿ�ο�����дAT24C02			   
	if(temp==0x55)return 0;		   
	else//�ų���һ�γ�ʼ�������
	{
        Delete_allDat();    //�������
        printf("AT24CXX_Delete_allDat()!\r\n");
		Write_RFID_Key((uint8_t *)ucRFID_Key_Temp);	//RFID_Key ��ʼֵ
		Write_IPAdd((uint8_t *)IPAdd_Temp);		//IP��ַ
		Write_PortAdd(PortAdd_Temp);			//�˿ں�
		Write_ACUSN((uint8_t *)ACUSN_Temp);		//���������SN
		Write_ACUAdd((uint8_t *)ACUAdd_Temp);
		AT24CXX_WriteOneByte(255,0x55);
	    temp=AT24CXX_ReadOneByte(255);	  
		if(temp==0x55)return 0;
	}
	return 1;											  
}

//��AT24CXX�����ָ����ַ��ʼ����ָ������������
//ReadAddr :��ʼ�����ĵ�ַ ��24c02Ϊ0~255
//pBuffer  :���������׵�ַ
//NumToRead:Ҫ�������ݵĸ���
void AT24CXX_Read(u16 ReadAddr,u8 *pBuffer,u16 NumToRead)
{
	while(NumToRead)
	{
		*pBuffer++=AT24CXX_ReadOneByte(ReadAddr++);	
		NumToRead--;
	}
}  
//��AT24CXX�����ָ����ַ��ʼд��ָ������������
//WriteAddr :��ʼд��ĵ�ַ ��24c02Ϊ0~255
//pBuffer   :���������׵�ַ
//NumToWrite:Ҫд�����ݵĸ���
void AT24CXX_Write(u16 WriteAddr,u8 *pBuffer,u16 NumToWrite)
{
	while(NumToWrite--)
	{
		AT24CXX_WriteOneByte(WriteAddr,*pBuffer);
		WriteAddr++;
		pBuffer++;
	}
}
 
/*
*********************************************************************************************************
*	�� �� ��: Delete_allDat()
*	����˵��: ��Flash����
*	��    ��: 
*	�� �� ֵ: 
*********************************************************************************************************
*/
void Delete_allDat(void)
{
	uint16_t i;
    for(i=0;i<(EE_TYPE+1);i++)
    {
        AT24CXX_WriteOneByte( i, 0x00 );            
    }	
}










