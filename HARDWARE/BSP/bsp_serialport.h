#ifndef __BSP_SERIALPORT_H
#define __BSP_SERIALPORT_H	 
#include "sys.h"

#define VersionNo 	0x11	//����̼��汾��
#define TestFlag	0		//���Ա�־
//#define lwipTestFlag	1	//���Ա�־


#define SERIALPORT_COM	COM3	//USBת����ͨ�ſ�

void SendSerialPort(uint8_t *SerialDat);

#endif
