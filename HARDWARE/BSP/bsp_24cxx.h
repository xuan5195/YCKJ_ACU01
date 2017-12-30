/*
* Copyright (c) 2014,国安达股份有限公司电子部
* All right reserved.
*
* 文件名称：24cxx.h
* 文件标识：100PCS批量文件
* 摘    要：
*     适用于AT24CXX外部存储器，IIC通信，建立在myiic文件的接口上。
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


#ifndef __BSP_24CXX_H
#define __BSP_24CXX_H
#include "sys.h"


#define AT24C01		127
#define AT24C02		255
#define AT24C04		511
#define AT24C08		1023
#define AT24C16		2047
#define AT24C32		4095
#define AT24C64	    8191
#define AT24C128	16383
#define AT24C256	32767  
//Mini STM32开发板使用的是24c02，所以定义EE_TYPE为AT24C02
#define EE_TYPE AT24C08
					  
u8 AT24CXX_ReadOneByte(u16 ReadAddr);							//指定地址读取一个字节
void AT24CXX_WriteOneByte(u16 WriteAddr,u8 DataToWrite);		//指定地址写入一个字节
void AT24CXX_WriteLenByte(u16 WriteAddr,u32 DataToWrite,u8 Len);//指定地址开始写入指定长度的数据
u32 AT24CXX_ReadLenByte(u16 ReadAddr,u8 Len);					//指定地址开始读取指定长度数据
void AT24CXX_Write(u16 WriteAddr,u8 *pBuffer,u16 NumToWrite);	//从指定地址开始写入指定长度的数据
void AT24CXX_Read(u16 ReadAddr,u8 *pBuffer,u16 NumToRead);   	//从指定地址开始读出指定长度的数据

u8 AT24CXX_Check(void);  //检查器件
void AT24CXX_Init(void); //初始化IIC
void Delete_allDat(void);   //清空存储数据

#endif
















