#ifndef __BSP_SERIALPORT_H
#define __BSP_SERIALPORT_H	 
#include "sys.h"

#define VersionNo 	0xC1	//����̼��汾��
#define TestFlag	1		//���Ա�־


#define SERIALPORT_COM	COM3	//USBת����ͨ�ſ�

void SendSerialPort(uint8_t *SerialDat);

#endif
