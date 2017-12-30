/*
* Copyright (c) 2014,������ɷ����޹�˾���Ӳ�
* All right reserved.
*
* �ļ����ƣ�common.c
* �ļ���ʶ��100PCS�����ļ�
* ժ    Ҫ��
*       	���ļ���ҪIIC��ʼ�������ݶ�д��Ӧ������ݺ�����
*
* �ļ���ʷ:
* �汾��  	����       	����    	˵��
* V0.1    2012.09.09 ����ԭ��  	�������ļ�
* V1.1   	20140826 	���  		ʹ����GAD-DL02���� ������
*
*/

#ifndef __BSP_MYIIC_H
#define __BSP_MYIIC_H
#include "sys.h"



////λ������,ʵ��51���Ƶ�GPIO���ƹ���
////����ʵ��˼��,�ο�<<CM3Ȩ��ָ��>>������(87ҳ~92ҳ).
////IO�ڲ����궨��
//#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2)) 
//#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr)) 
//#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum)) 
////IO�ڵ�ַӳ��
//#define GPIOA_ODR_Addr    (GPIOA_BASE+12) //0x4001080C 
//#define GPIOB_ODR_Addr    (GPIOB_BASE+12) //0x40010C0C 
//#define GPIOC_ODR_Addr    (GPIOC_BASE+12) //0x4001100C 
//#define GPIOD_ODR_Addr    (GPIOD_BASE+12) //0x4001140C 
//#define GPIOE_ODR_Addr    (GPIOE_BASE+12) //0x4001180C 
//#define GPIOF_ODR_Addr    (GPIOF_BASE+12) //0x40011A0C    
//#define GPIOG_ODR_Addr    (GPIOG_BASE+12) //0x40011E0C    

//#define GPIOA_IDR_Addr    (GPIOA_BASE+8) //0x40010808 
//#define GPIOB_IDR_Addr    (GPIOB_BASE+8) //0x40010C08 
//#define GPIOC_IDR_Addr    (GPIOC_BASE+8) //0x40011008 
//#define GPIOD_IDR_Addr    (GPIOD_BASE+8) //0x40011408 
//#define GPIOE_IDR_Addr    (GPIOE_BASE+8) //0x40011808 
//#define GPIOF_IDR_Addr    (GPIOF_BASE+8) //0x40011A08 
//#define GPIOG_IDR_Addr    (GPIOG_BASE+8) //0x40011E08 
// 
////IO�ڲ���,ֻ�Ե�һ��IO��!
////ȷ��n��ֵС��16!
//#define PAout(n)   BIT_ADDR(GPIOA_ODR_Addr,n)  //��� 
//#define PAin(n)    BIT_ADDR(GPIOA_IDR_Addr,n)  //���� 

//#define PBout(n)   BIT_ADDR(GPIOB_ODR_Addr,n)  //��� 
//#define PBin(n)    BIT_ADDR(GPIOB_IDR_Addr,n)  //���� 

//#define PCout(n)   BIT_ADDR(GPIOC_ODR_Addr,n)  //��� 
//#define PCin(n)    BIT_ADDR(GPIOC_IDR_Addr,n)  //���� 

//#define PDout(n)   BIT_ADDR(GPIOD_ODR_Addr,n)  //��� 
//#define PDin(n)    BIT_ADDR(GPIOD_IDR_Addr,n)  //���� 

//#define PEout(n)   BIT_ADDR(GPIOE_ODR_Addr,n)  //��� 
//#define PEin(n)    BIT_ADDR(GPIOE_IDR_Addr,n)  //����

//#define PFout(n)   BIT_ADDR(GPIOF_ODR_Addr,n)  //��� 
//#define PFin(n)    BIT_ADDR(GPIOF_IDR_Addr,n)  //����

//#define PGout(n)   BIT_ADDR(GPIOG_ODR_Addr,n)  //��� 
//#define PGin(n)    BIT_ADDR(GPIOG_IDR_Addr,n)  //����


//IO��������
#define SDA_IN()  {GPIOB->CRL&=0X0FFFFFFF;GPIOB->CRL|=(u32)8<<28;}
#define SDA_OUT() {GPIOB->CRL&=0X0FFFFFFF;GPIOB->CRL|=(u32)3<<28;}

//IO��������	 
#define IIC_SCL    PBout(6) //SCL
#define IIC_SDA    PBout(7) //SDA	 
#define READ_SDA   PBin(7)  //����SDA 

//IIC���в�������
void IIC_Init(void);                //��ʼ��IIC��IO��				 
void IIC_Start(void);				//����IIC��ʼ�ź�
void IIC_Stop(void);	  			//����IICֹͣ�ź�
void IIC_Send_Byte(u8 txd);			//IIC����һ���ֽ�
u8 IIC_Read_Byte(unsigned char ack);//IIC��ȡһ���ֽ�
u8 IIC_Wait_Ack(void); 				//IIC�ȴ�ACK�ź�
void IIC_Ack(void);					//IIC����ACK�ź�
void IIC_NAck(void);				//IIC������ACK�ź�

void IIC_Write_One_Byte(u8 daddr,u8 addr,u8 data);
u8 IIC_Read_One_Byte(u8 daddr,u8 addr);	  
#endif
















