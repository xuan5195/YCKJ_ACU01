#include "led.h"

//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEKս��STM32������
//LED��������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/9/2
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	   

//��ʼ��PC.6-8 Ϊ�����.��ʹ������ڵ�ʱ��		    
//LED IO��ʼ��
void LED_Init(void)
{ 
    GPIO_InitTypeDef  GPIO_InitStructure;
 	
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);	 //ʹ��PC�˿�ʱ��
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8;	//LED0-->PC.6-8 �˿�����
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		        //�������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		        //IO���ٶ�Ϊ50MHz
    GPIO_Init(GPIOC, &GPIO_InitStructure);					        //�����趨������ʼ��GPIOB.5
    GPIO_SetBits(GPIOC,GPIO_Pin_6); 						        //PC.6 ����� 
    GPIO_SetBits(GPIOC,GPIO_Pin_7); 						        //PC.7 ����� 
    GPIO_SetBits(GPIOC,GPIO_Pin_8); 						        //PC.8 ����� 
}
 
