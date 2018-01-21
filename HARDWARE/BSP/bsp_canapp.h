#ifndef __BSP_CANAPP_H
#define __BSP_CANAPP_H	 

#include "stm32f10x.h"

uint8_t Can_WriteLogitADD(uint8_t _LogitADD,uint8_t *_PhysicalADD);
void Can_SendBroadcast_Key(uint8_t *_uBuff);
void Can_UnregisteredBroadcast(void);
uint8_t Can_ReadUnregistered(uint8_t *_uBuff);
void Can_SendBroadcast_Com(uint8_t _WaterCost,uint8_t _CostNum);
uint8_t Can_SendPBus_Com(uint8_t _uDat,uint8_t *_uBuff);
void Can_ReadPBus_Succeed(uint8_t *_uBuff);
void Can_SendPBus_ErrCom(uint8_t *_uBuff);
void Send_IAPDate(uint8_t _PackNo);
void Send_IAPDate0(uint8_t _PackNo);

#endif

















