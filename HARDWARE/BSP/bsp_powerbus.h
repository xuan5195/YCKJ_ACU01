#ifndef __BSP_POWERBUS_H
#define __BSP_POWERBUS_H	 
#include "sys.h"

#define POWERBUS_COM	COM2	//���ߴ���
#define	BUSNUM_SIZE		80		//���ߴ�վ����
void UnregisteredBroadcast(void);   //δע��㲥
uint8_t ReadUnregistered(uint8_t *_uBuff);
void ReadRunningData(uint8_t _no);
void WriteRFIDData(uint8_t *_WriteDat);
//uint8_t WriteLogitADD(uint8_t _LogitADD,uint8_t *_PhysicalADD);
void SendBroadcast_Com(uint8_t _WaterCost,uint8_t _CostNum);
void SendBroadcast_Key(uint8_t *_uBuff);
//uint8_t SendPBus_Dat(uint8_t _uDat,uint8_t *_uBuff);
uint8_t SendPBus_Dat2(uint8_t _uDat,uint8_t *_uBuff);
//uint8_t SendPBus_SearchDat(uint8_t _uDat,uint8_t *_uBuff);
uint8_t Binary_searchSN(void);
void PrintfDat(void);
//void PrintfDat2(void);


#endif
