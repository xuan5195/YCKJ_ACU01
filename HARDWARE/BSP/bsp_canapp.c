#include "stm32f10x.h"
#include <stdio.h>

#include "bsp_can.h"
#include "bsp_canapp.h"
#include "bsp_crc8.h"

#include "includes.h"

extern uint8_t g_RxMessage[8];	//CAN接收数据
extern uint8_t g_RxMessFlag;	//CAN接收数据 标志

//写入逻辑地址
//_LogitADD：逻辑地址
uint8_t Can_WriteLogitADD(uint8_t _LogitADD,uint8_t *_PhysicalADD)
{
    uint8_t i,ReTemp=0,Overflow_Flag=0,res=0;
	uint8_t Runningbuf[8]={0x00},Recebuf[8]={0x00};
	Runningbuf[0] = *(_PhysicalADD+0);	//物理地址1
	Runningbuf[1] = *(_PhysicalADD+1);	//物理地址2
	Runningbuf[2] = *(_PhysicalADD+2);	//物理地址3
	Runningbuf[3] = *(_PhysicalADD+3);	//物理地址4
	Runningbuf[4] = _LogitADD;			//逻辑地址

	g_RxMessFlag = 0x00;
	for(i=0;i<8;i++){	g_RxMessage[i] = 0x00;	}
	Package_Send(0x02,(uint8_t *)Runningbuf);	//写入逻辑地址

	Overflow_Flag = 20;
    printf("Over=%d",Overflow_Flag);  
	while((res==0)&& (Overflow_Flag>0) )//接收到有数据
	{
//		res=Can_Receive_Msg(Recebuf);
//		if(Recebuf[5]!=_LogitADD)	res = 0;
//		else	continue;
		if(g_RxMessFlag == 0xAA)
		{
			for(i=0;i<8;i++){	Recebuf[i] = g_RxMessage[i];	g_RxMessage[i] = 0x00;	}
			printf("Rece<<%02X%02X%02X%02X%02X%02X%02X%02X;\r\n",Recebuf[0],Recebuf[1],Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
			g_RxMessFlag = 0x00;
			if((Recebuf[5]!=0xC2)&&(Recebuf[5]!=_LogitADD))	res = 0;
			else {	res = 1;	continue;	}
		}			
		OSTimeDlyHMSM(0, 0, 0, 5);
		Overflow_Flag--;
	}
    printf("-%d; ",Overflow_Flag); 
	if(res!=0)
	{
		printf("Re<<%02X%02X%02X%02X%02X%02X%02X%02X ",Recebuf[0],Recebuf[1],Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
		if( (Recebuf[0] == 0xC2)&&(Recebuf[1] == Runningbuf[0])&&(Recebuf[2] == Runningbuf[1])\
		&&(Recebuf[3] == Runningbuf[2])&&(Recebuf[4] == Runningbuf[3])&&(Recebuf[5] == Runningbuf[4]))
		{
			ReTemp = Recebuf[6];		//数据正确 返回版本号
			res = 0;
		}
		else	ReTemp = 0x00;	//数据错误  a.超时；b.数据错
	}
	else	ReTemp = 0x00;	//数据错误  a.超时；b.数据错
	return 	ReTemp;
}
//广播 RFID
void Can_SendBroadcast_Key(uint8_t *_uBuff)
{
	uint8_t Runningbuf[8]={0x00};
	Runningbuf[0] = *(_uBuff+0);	//Key1
	Runningbuf[1] = *(_uBuff+1);	//
	Runningbuf[2] = *(_uBuff+2);	//
	Runningbuf[3] = *(_uBuff+3);	//
	Runningbuf[4] = *(_uBuff+4);	//
	Runningbuf[5] = *(_uBuff+5);	//
	Runningbuf[6] = *(_uBuff+6);	//块
	//printf(">>>>>>>>>广播 RFID_Key %2X%2X%2X%2X%2X%2X,块地址：%d\r\n",Runningbuf[0],Runningbuf[1],Runningbuf[2],Runningbuf[3],Runningbuf[4],Runningbuf[5],Runningbuf[6]);
	Package_Send(0x03,(uint8_t *)Runningbuf);
}
//未注册广播 
void Can_UnregisteredBroadcast(void)
{
	uint8_t Runningbuf[8]={0x00};
	Package_Send(0x01,(uint8_t *)Runningbuf);
}

//水费广播
void Can_SendBroadcast_Com(uint8_t _WaterCost,uint8_t _CostNum)
{
	uint8_t Runningbuf[8]={0x00};
	Runningbuf[0] = _WaterCost;	//WaterCost=水费 最小扣款金额 0.005元
	Runningbuf[1] = _CostNum;	//流量计脉冲数 每升水计量周期
	printf(">>>>>>>>>广播 水费=%d; 脉冲数=%d\r\n",Runningbuf[0],Runningbuf[1]);
	Package_Send(0x04,(uint8_t *)Runningbuf);
}


//读取未注册物理地址 
uint8_t Can_ReadUnregistered(uint8_t *_uBuff)
{
	uint8_t i,Overflow_Flag=20;
	uint8_t CRC_Dat=0,res=0;
	uint8_t Recebuf[8]={0x00};
	while((res==0)&& (Overflow_Flag>0) )//接收到有数据
	{
//		res = Can_Receive_Msg(Recebuf);
//		if(res != 0)	continue;
		if(g_RxMessFlag == 0xAA)
		{
			for(i=0;i<8;i++){	Recebuf[i] = g_RxMessage[i];	g_RxMessage[i] = 0x00;	}
			printf("Rece<<%02X%02X%02X%02X%02X%02X%02X%02X;\r\n",Recebuf[0],Recebuf[1],Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
			g_RxMessFlag = 0x00;
			res = 1;
			continue;
		}			
		OSTimeDlyHMSM(0, 0, 0, 5);
		Overflow_Flag--;
	}
	//printf("- %d;",Overflow_Flag);  
	if( res != 0 )
	{
		if( Recebuf[0] == 0xB3 )	
		{
			_uBuff[0] = Recebuf[1];	//卡机物理地址1；
			_uBuff[1] = Recebuf[2];	//卡机物理地址2；
			_uBuff[2] = Recebuf[3];	//卡机物理地址3；
			_uBuff[3] = Recebuf[4];	//卡机物理地址4；
			CRC_Dat = CRC8_Table(_uBuff,4);
			if( CRC_Dat == Recebuf[5] )	return 0x01;
			else						return 0x00;	//数据错误  a.超时；b.数据错
		}
		else	return 0x00;	//数据错误  a.超时；b.数据错		
	}
	else 	return 0x00;	//数据错误  a.超时；b.数据错
}
//查询是否有数据
uint8_t Can_SendPBus_Com(uint8_t _uDat,uint8_t *_uBuff)
{
	uint8_t Recebuf[8]={0x00};
	uint8_t Runningbuf[8]={0x00};
	uint8_t Overflow_Flag=10;
    uint8_t ReTemp=0;
	uint8_t i,res=0;
	Runningbuf[0] = _uDat;	//形参  逻辑地址
	Package_Send(0x05,(uint8_t *)Runningbuf);
	Overflow_Flag=10;
    printf("轮循 %d ",Overflow_Flag);
	while((res==0)&& (Overflow_Flag>0) )//接收到有数据
	{
//		res=Can_Receive_Msg(Recebuf);
//		if(Recebuf[1]!=_uDat)	res = 0;
//		else	continue;
		if(g_RxMessFlag == 0xAA)
		{
			for(i=0;i<8;i++){	Recebuf[i] = g_RxMessage[i];	g_RxMessage[i] = 0x00;	}
			//printf("Rece<<%02X%02X%02X%02X%02X%02X%02X%02X;\r\n",Recebuf[0],Recebuf[1],Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
			g_RxMessFlag = 0x00;
			if(Recebuf[1]!=_uDat)	res = 0;
			else	{	res = 1;	continue;	}		
		}
		OSTimeDlyHMSM(0, 0, 0, 5);
		Overflow_Flag--;
	}
    printf("- %d;  \r\n",Overflow_Flag);
	if(res)
	{
		printf("Rece<< %02X%02X%02X%02X%02X%02X%02X%02X;\r\n",Recebuf[0],Recebuf[1],Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
		if( ( Recebuf[1] == _uDat )&&(Recebuf[0] == 0x02 ))		ReTemp = 0x02;	//无数据
		else if( ( Recebuf[1] == _uDat )&&(Recebuf[0] == 0x03 ))	//插卡数据包1
		{
			_uBuff[0] = 0x05;	//05表示插卡；06表示拔卡；
			_uBuff[1] = Recebuf[2];	//卡号1；
			_uBuff[2] = Recebuf[3];	//卡号2；
			_uBuff[3] = Recebuf[4];	//卡号3；
			_uBuff[4] = Recebuf[5];	//卡号4；
			Overflow_Flag=10;res=0;
			while((res==0)&& (Overflow_Flag>0) )//接收到有数据
			{
//				res=Can_Receive_Msg(Recebuf);
//				if(Recebuf[1]!=_uDat)	res = 0;
//				else	continue;
				if(g_RxMessFlag == 0xAA)
				{
					for(i=0;i<8;i++){	Recebuf[i] = g_RxMessage[i];	g_RxMessage[i] = 0x00;	}
					//printf("Rece<<%02X%02X%02X%02X%02X%02X%02X%02X;\r\n",Recebuf[0],Recebuf[1],Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
					g_RxMessFlag = 0x00;
					if(Recebuf[1]!=_uDat)	res = 0;
					else	{	res = 1;	continue;	}		
				}
				OSTimeDlyHMSM(0, 0, 0, 5);
				Overflow_Flag--;
			}
			if(res)
			{
				printf("Rece<< %02X%02X%02X%02X%02X%02X%02X%02X;\r\n",Recebuf[0],Recebuf[1],Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
				if(( Recebuf[1] == _uDat )&&( Recebuf[0] == 0x04 ))	//判断是否是本机数据
				{
					_uBuff[5] = Recebuf[2];	//金额1；
					_uBuff[6] = Recebuf[3];	//金额2；
					_uBuff[7] = Recebuf[4];	//金额3；
					_uBuff[8] = Recebuf[5];	//卡校验；
					_uBuff[9] = Recebuf[6];	//通信码；
					ReTemp = 0xAA;	//数据接收成功
					return ReTemp;
				}
				else	ReTemp = 0x00;	//数据接收错误  a.超时；b.数据错
				res = 0;
			}
			else	ReTemp = 0x00;	//数据接收错误  a.超时；b.数据错
		}
		else if( ( Recebuf[1] == _uDat )&&(Recebuf[0] == 0x05 ))	//拔卡数据包1
		{
			_uBuff[0] = 0x06;	//05表示插卡；06表示拔卡；
			_uBuff[1] = Recebuf[2];	//卡号1；
			_uBuff[2] = Recebuf[3];	//卡号2；
			_uBuff[3] = Recebuf[4];	//卡号3；
			_uBuff[4] = Recebuf[5];	//卡号4；
			Overflow_Flag=10;
			while((res==0)&& (Overflow_Flag>0) )//接收到有数据
			{
//				res=Can_Receive_Msg(Recebuf);
//				if(Recebuf[1]!=_uDat)	res = 0;
//				else	continue;
				if(g_RxMessFlag == 0xAA)
				{
					for(i=0;i<8;i++){	Recebuf[i] = g_RxMessage[i];	g_RxMessage[i] = 0x00;	}
					//printf("Rece<<%02X%02X%02X%02X%02X%02X%02X%02X;\r\n",Recebuf[0],Recebuf[1],Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
					g_RxMessFlag = 0x00;
					if(Recebuf[1]!=_uDat)	res = 0;
					else	{	res = 1;	continue;	}		
				}
				OSTimeDlyHMSM(0, 0, 0, 5);
				Overflow_Flag--;
			}
			if(res)
			{
				printf("Rece<< %02X%02X%02X%02X%02X%02X%02X%02X;\r\n",Recebuf[0],Recebuf[1],Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
				if(( Recebuf[1] == _uDat )&&( Recebuf[0] == 0x06 ))	//判断是否是本机数据
				{
					_uBuff[5] = Recebuf[2];	//金额1；
					_uBuff[6] = Recebuf[3];	//金额2；
					_uBuff[7] = Recebuf[4];	//金额3；
					_uBuff[8] = Recebuf[5];	//卡校验；
					_uBuff[9] = Recebuf[6];	//通信码；
					ReTemp = 0xAA;	//数据接收成功
					return ReTemp;
				}
				else	ReTemp = 0x00;	//数据接收错误  a.超时；b.数据错
				res = 0;
			}
			else	ReTemp = 0x00;	//数据接收错误  a.超时；b.数据错
		}
		else if( ( Recebuf[1] == _uDat )&&(Recebuf[0] == 0x04 ))	//插卡数据包2
		{
			_uBuff[5] = Recebuf[2];	//金额1；
			_uBuff[6] = Recebuf[3];	//金额2；
			_uBuff[7] = Recebuf[4];	//金额3；
			_uBuff[8] = Recebuf[5];	//卡校验；
			_uBuff[9] = Recebuf[6];	//通信码；
			ReTemp = 0xAA;	//数据接收成功
			return ReTemp;
		}
		else if( ( Recebuf[1] == _uDat )&&(Recebuf[0] == 0x06 ))	//拔卡数据包2
		{
			_uBuff[5] = Recebuf[2];	//金额1；
			_uBuff[6] = Recebuf[3];	//金额2；
			_uBuff[7] = Recebuf[4];	//金额3；
			_uBuff[8] = Recebuf[5];	//卡校验；
			_uBuff[9] = Recebuf[6];	//通信码；
			ReTemp = 0xAA;	//数据接收成功
			return ReTemp;
		}
		else	ReTemp = 0x00;	//数据接收错误  a.超时；b.数据错
		
	}
	else	ReTemp = 0x00;	//数据接收错误  a.超时；b.数据错

	return ReTemp;
}

//取回成功回复
void Can_ReadPBus_Succeed(uint8_t *_uBuff)
{
	uint8_t Runningbuf[8]={0x00};
	Runningbuf[0] = _uBuff[0];	//逻辑地址
	Runningbuf[1] = _uBuff[1];	//卡号1
	Runningbuf[2] = _uBuff[2];	//卡号2
	Runningbuf[3] = _uBuff[3];	//卡号3
	Runningbuf[4] = _uBuff[4];	//卡号4
	Runningbuf[5] = _uBuff[5];	//插卡/拔卡
	Package_Send(0x06,(uint8_t *)Runningbuf);
}
//取回成功回复
void Can_SendPBus_ErrCom(uint8_t *_uBuff)
{
	uint8_t Runningbuf[8]={0x00};
	Runningbuf[0] = _uBuff[0];	//逻辑地址
	Runningbuf[1] = _uBuff[1];	//"E"
	Runningbuf[2] = _uBuff[2];	//异常代码高位
	Runningbuf[3] = _uBuff[3];	//异常代码
	Runningbuf[4] = _uBuff[4];	//异常代码低位
	Runningbuf[6] = _uBuff[5];	//通信码
	Package_Send(0x11,(uint8_t *)Runningbuf);
}
