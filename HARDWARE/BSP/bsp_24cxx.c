/*
* Copyright (c) 2014,国安达股份有限公司电子部
* All right reserved.
*
* 文件名称：led.c
* 文件标识：100PCS批量文件
* 摘    要：
*       	本文件主要适用于AT24CXX外部存储器，IIC通信，建立在myiic文件的接口上等功能。
*		包含：
		u8 		AT24CXX_ReadOneByte(u16 ReadAddr);							//指定地址读取一个字节
		void 	AT24CXX_WriteOneByte(u16 WriteAddr,u8 DataToWrite);			//指定地址写入一个字节
		void 	AT24CXX_WriteLenByte(u16 WriteAddr,u32 DataToWrite,u8 Len);	//指定地址开始写入指定长度的数据
		u32 	AT24CXX_ReadLenByte(u16 ReadAddr,u8 Len);					//指定地址开始读取指定长度数据
		void 	AT24CXX_Write(u16 WriteAddr,u8 *pBuffer,u16 NumToWrite);	//从指定地址开始写入指定长度的数据
		void 	AT24CXX_Read(u16 ReadAddr,u8 *pBuffer,u16 NumToRead);   	//从指定地址开始读出指定长度的数据

*
* 文件历史:
* 版本号  	日期       	作者    	说明
* V0.1    	20120909 	正点原子  	创建该文件
* V1.1   	20140826 	北宸  		使用于GAD-DL02主机 批量产
*
*/

#include "stm32f10x.h"
#include "sys.h" 
#include "delay.h"
#include "bsp_24cxx.h"
#include "bsp_myiic.h"
#include "bsp_uart_fifo.h"
#include "bsp_useadd.h"
extern uint8_t g_lwipADD[4];	//远端IP地址
extern uint16_t g_lwipPort;		//远端端口号
extern uint8_t g_ACUSN[4];		//区域控制器SN 4位
extern uint8_t g_ACUAdd[16];	//通信码16位
extern uint8_t FM1702_Key[7];	//FM1702_Key[0]-[5]为Key;FM1702_Key[6]为块地址；
extern uint8_t g_Setip[5];       	//本机IP地址
extern uint8_t g_Setnetmask[4]; 	//子网掩码
extern uint8_t g_Setgateway[4]; 	//默认网关的IP地址

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
		IIC_Send_Byte(0XA0);	   //发送写命令
		IIC_Wait_Ack();
		IIC_Send_Byte(ReadAddr>>8);//发送高地址
		IIC_Wait_Ack();		 
	}else IIC_Send_Byte(0XA0+((ReadAddr/256)<<1));   //发送器件地址0XA0,写数据 	 

	IIC_Wait_Ack(); 
    IIC_Send_Byte(ReadAddr%256);   //发送低地址
	IIC_Wait_Ack();	    
	IIC_Start();  	 	   
	IIC_Send_Byte(0XA1);           //进入接收模式			   
	IIC_Wait_Ack();	 
    temp=IIC_Read_Byte(0);		   
    IIC_Stop();//产生一个停止条件	    
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
		IIC_Send_Byte(0XA0);	    //发送写命令
		IIC_Wait_Ack();
		IIC_Send_Byte(WriteAddr>>8);//发送高地址
 	}else
	{
		IIC_Send_Byte(0XA0+((WriteAddr/256)<<1));   //发送器件地址0XA0,写数据 
	}	 
	IIC_Wait_Ack();	   
    IIC_Send_Byte(WriteAddr%256);   //发送低地址
	IIC_Wait_Ack(); 	 										  		   
	IIC_Send_Byte(DataToWrite);     //发送字节							   
	IIC_Wait_Ack();  		    	   
    IIC_Stop();//产生一个停止条件 
	//OSTimeDlyHMSM(0, 0, 0, 10);
    delay_ms(10);
    
}
//在AT24CXX里面的指定地址开始写入长度为Len的数据
//该函数用于写入16bit或者32bit的数据.
//WriteAddr  :开始写入的地址  
//DataToWrite:数据数组首地址
//Len        :要写入数据的长度2,4
void AT24CXX_WriteLenByte(u16 WriteAddr,u32 DataToWrite,u8 Len)
{  	
	u8 t;
	for(t=0;t<Len;t++)
	{
		AT24CXX_WriteOneByte(WriteAddr+t,(DataToWrite>>(8*t))&0xff);
	}												    
}

//在AT24CXX里面的指定地址开始读出长度为Len的数据
//该函数用于读出16bit或者32bit的数据.
//ReadAddr   :开始读出的地址 
//返回值     :数据
//Len        :要读出数据的长度2,4
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
24C08数据存储	
	>>255	存储使用标识；						
	>>200	201	202	203	IP地址 例：192.168.1.1			
	>>204	205	端口号：20000(4E20)； 205存放4E；206存放20					
	>>206	207	208	209	存放区域控制器SN			
	>>210	-225	存放通信码					
	>>230	231	232	233	234	235	存放RFID_KEY	
	>>236	存放RFID_Key绝对块号；						
								
	>>10	14	存放逻辑地址为1的物理地址					
	>>15	19	存放逻辑地址为2的物理地址					
	>>20	24	存放逻辑地址为3的物理地址					
	>>……	……						
	>>135	139	存放逻辑地址为25的物理地址					
*/

//检查AT24CXX是否正常
//这里用了24XX的最后一个地址(255)来存储标志字.
//如果用其他24C系列,这个地址要修改
//返回1:检测失败
//返回0:检测成功
u8 AT24CXX_Check(void)
{
	u8 temp;
	uint8_t ucRFID_Key_Temp[7]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x01};	//RFID_Key 初始值
	uint8_t ACUSN_Temp[4]={0x20,0x20,0x01,0x01};	//区域控制器SN
	uint8_t IPAdd_Temp[4]={39,106,113,254};	//区域控制器SN
	uint16_t PortAdd_Temp=20000;	//端口号
	uint8_t ACUAdd_Temp[16]={0x31,0x30,0x30,0x30,0x30,0x30,0x30,0x2D,0x62,0x37,0x36,0x31,0x63,0x32,0x62,0x30};	//区域控制器SN

	Read_RFID_Key((uint8_t *)FM1702_Key);	printf("RFID_Key:%02X%02X%02X%02X%02X%02X.绝对块号:%d\r\n",FM1702_Key[0],FM1702_Key[1],FM1702_Key[2],FM1702_Key[3],FM1702_Key[4],FM1702_Key[5],FM1702_Key[6]);
	Read_IPAdd((uint8_t *)g_lwipADD);		printf("IP地址------------%d.%d.%d.%d\r\n",g_lwipADD[0],g_lwipADD[1],g_lwipADD[2],g_lwipADD[3]);
	g_lwipPort = Read_PortAdd();			printf("端口号------------%5d\r\n",g_lwipPort);
	Read_ACUSN((uint8_t *)g_ACUSN);			printf("区域控制器SN------%02X%02X%02X%02X\r\n",g_ACUSN[0],g_ACUSN[1],g_ACUSN[2],g_ACUSN[3]);
	Read_ACUAdd((uint8_t *)g_ACUAdd);		printf("区域控制器通信码--");	
	printf("%2X%2X%2X%2X%2X%2X%2X%2X",		g_ACUAdd[0],g_ACUAdd[1],g_ACUAdd[2],g_ACUAdd[3],g_ACUAdd[4],g_ACUAdd[5],g_ACUAdd[6],g_ACUAdd[7]);
	printf("%2X%2X%2X%2X%2X%2X%2X%2X\r\n",	g_ACUAdd[8],g_ACUAdd[9],g_ACUAdd[10],g_ACUAdd[11],g_ACUAdd[12],g_ACUAdd[13],g_ACUAdd[14],g_ACUAdd[15]);

	Read_localIP((uint8_t *)g_Setip);	//静态IP标志 0xAA为静态IP
	if(g_Setip[0]==0xAA)	
	{
		printf("静态IP---开启\r\n");	
		Read_netmask((uint8_t *)g_Setnetmask);
		Read_gateway((uint8_t *)g_Setgateway);
	}
	else printf("静态IP---禁用\r\n");
	temp=AT24CXX_ReadOneByte(255);//避免每次开机都写AT24C02			   
	if(temp==0x55)return 0;		   
	else//排除第一次初始化的情况
	{
        Delete_allDat();    //清空数据
        printf("AT24CXX_Delete_allDat()!\r\n");
		Write_RFID_Key((uint8_t *)ucRFID_Key_Temp);	//RFID_Key 初始值
		Write_IPAdd((uint8_t *)IPAdd_Temp);		//IP地址
		Write_PortAdd(PortAdd_Temp);			//端口号
		Write_ACUSN((uint8_t *)ACUSN_Temp);		//区域控制器SN
		Write_ACUAdd((uint8_t *)ACUAdd_Temp);
		AT24CXX_WriteOneByte(255,0x55);
	    temp=AT24CXX_ReadOneByte(255);	  
		if(temp==0x55)return 0;
	}
	return 1;											  
}

//在AT24CXX里面的指定地址开始读出指定个数的数据
//ReadAddr :开始读出的地址 对24c02为0~255
//pBuffer  :数据数组首地址
//NumToRead:要读出数据的个数
void AT24CXX_Read(u16 ReadAddr,u8 *pBuffer,u16 NumToRead)
{
	while(NumToRead)
	{
		*pBuffer++=AT24CXX_ReadOneByte(ReadAddr++);	
		NumToRead--;
	}
}  
//在AT24CXX里面的指定地址开始写入指定个数的数据
//WriteAddr :开始写入的地址 对24c02为0~255
//pBuffer   :数据数组首地址
//NumToWrite:要写入数据的个数
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
*	函 数 名: Delete_allDat()
*	功能说明: 清Flash数量
*	形    参: 
*	返 回 值: 
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










