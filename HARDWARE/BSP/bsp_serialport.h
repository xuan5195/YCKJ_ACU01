#ifndef __BSP_SERIALPORT_H
#define __BSP_SERIALPORT_H	 
#include "sys.h"

#define VersionNo 	0x11	//区域固件版本号
#define TestFlag	0		//测试标志
//#define lwipTestFlag	1	//测试标志


#define SERIALPORT_COM	COM3	//USB转串口通信口

void SendSerialPort(uint8_t *SerialDat);

#endif
