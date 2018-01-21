#include "bsp_powerbus.h"
#include "bsp_uart_fifo.h"
#include "bsp_crc8.h"
#include "bsp_useadd.h"

#include "delay.h"
#include "sys.h"
#include "string.h"
//#include "usmart.h"	
#include "includes.h"
#include "led.h"

#include "bsp_can.h"
#include "bsp_canapp.h"

uint8_t Overflow_Flag;	//等待超时标志
extern uint8_t g_RUNDate[BUSNUM_SIZE+1][14];    //运行数据；
extern uint8_t KJ_Versions[BUSNUM_SIZE+1];		//卡机版本号
extern uint8_t KJ_NonResponse[BUSNUM_SIZE+1];	//卡机无回复标志 当数值大于20时，清卡机；在数值为5，10，15时发起一次写逻辑地址操作

extern uint8_t g_CostNum;		//流量计脉冲数 每升水计量周期
extern uint8_t g_WaterCost;		//WaterCost=水费 最小扣款金额 0.005元


//未注册广播 
void UnregisteredBroadcast(void)
{
	uint8_t Runningbuf[4]={0xF3,0x04,0xA1,0x00};
	Runningbuf[3] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRC校验位
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);
    OSTimeDlyHMSM(0, 0, 0, 150);
}
/*
//读取未注册物理地址 
uint8_t ReadUnregistered(uint8_t *_uBuff)
{
	Overflow_Flag = 10;
	while( ( (PBus_Count&0x80)!=0x80 ) && (Overflow_Flag>1) ) //等待数据回复；
	{
		OSTimeDlyHMSM(0, 0, 0, 100);
		Overflow_Flag--;
		ReceivePowerBUSDat(0x01);	//带核验模式
	}
	if( PBus_Count>0x80 )	//表示接收到数据
	{
		PBus_Count = 0x00;
		_uBuff[0] = PBusDat[3];	//卡机物理地址1；
		_uBuff[1] = PBusDat[4];	//卡机物理地址2；
		_uBuff[2] = PBusDat[5];	//卡机物理地址3；
		_uBuff[3] = PBusDat[6];	//卡机物理地址4；
		return 0x01;
	}
	else	return 0x00;	//数据接收错误  a.超时；b.数据错
}
*/
/*
//水费广播
void SendBroadcast_Com(uint8_t _WaterCost,uint8_t _CostNum)
{
	uint8_t Runningbuf[6]={0xF3,0x06,0x10,0x05,0x1B,0x00};
	Runningbuf[3] = _WaterCost;	//WaterCost=水费 最小扣款金额 0.005元
	Runningbuf[4] = _CostNum;	//流量计脉冲数 每升水计量周期
	printf(">>>>>>>>>广播 水费=%d; 脉冲数=%d\r\n",Runningbuf[3],Runningbuf[4]);
	Runningbuf[Runningbuf[1]-1] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRC校验位
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);
}*/

/*
//广播 RFID
void SendBroadcast_Key(uint8_t *_uBuff)
{
	uint8_t Runningbuf[11]={0xF3,0x0B,0xCD,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x01,0x00};
	Runningbuf[3] = *(_uBuff+0);	//Key1
	Runningbuf[4] = *(_uBuff+1);	//
	Runningbuf[5] = *(_uBuff+2);	//
	Runningbuf[6] = *(_uBuff+3);	//
	Runningbuf[7] = *(_uBuff+4);	//
	Runningbuf[8] = *(_uBuff+5);	//
	Runningbuf[9] = *(_uBuff+6);	//块
	printf(">>>>>>>>>广播 RFID_Key %2X%2X%2X%2X%2X%2X,块地址：%d\r\n",Runningbuf[3],Runningbuf[4],Runningbuf[5],Runningbuf[6],Runningbuf[7],Runningbuf[8],Runningbuf[9]);
	Runningbuf[Runningbuf[1]-1] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRC校验位
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);
}
*/

/*
//查询是否有数据
uint8_t SendPBus_Com(uint8_t _uDat,uint8_t *_uBuff)
{
    uint8_t ReTemp=0;
	uint8_t Runningbuf[5]={0xF3,0x05,0x01,0x00,0x00};
	Runningbuf[3] = _uDat;	//形参  逻辑地址
	Runningbuf[4] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRC校验位
	PBus_Count = 0x00;
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);	//发送轮循取数据命令
	Overflow_Flag = 25;
    printf("轮循命令 %d ",Overflow_Flag);
	while( ( (PBus_Count&0x80)!=0x80 ) && (Overflow_Flag>0) ) //等待数据回复；
	{
		OSTimeDlyHMSM(0, 0, 0, 8);
		Overflow_Flag--;
		ReceivePowerBUSDat(0x01);	//带核验模式
	}
    printf("- %d;  ",Overflow_Flag);
	if( ( (PBus_Count&0x80) == 0x80 ))
	{
		PBus_Count = 0x00;
		if( PBusDat[3] == _uDat )
		{
			if( PBusDat[2] == 0x02 )		ReTemp = 0x02;	//无数据
			else//有数据
            {
                _uBuff[0] = PBusDat[2];	//05表示插卡；06表示拔卡；
                _uBuff[1] = PBusDat[4];	//卡号1；
                _uBuff[2] = PBusDat[5];	//卡号2；
                _uBuff[3] = PBusDat[6];	//卡号3；
                _uBuff[4] = PBusDat[7];	//卡号4；
                _uBuff[5] = PBusDat[8];	//金额1；
                _uBuff[6] = PBusDat[9];	//金额2；
                _uBuff[7] = PBusDat[10];//金额3；
                _uBuff[8] = PBusDat[11];//卡校验；
                _uBuff[9] = PBusDat[12];//通信码；
                ReTemp = 0xAA;		//数据正确 
            }
			printf("%02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n",PBusDat[0],PBusDat[1],PBusDat[2],PBusDat[3],PBusDat[4],PBusDat[5],PBusDat[6],PBusDat[7],PBusDat[8],PBusDat[9],PBusDat[10],PBusDat[11]);
		}
		else	ReTemp = 0x00;	//数据接收错误  a.超时；b.数据错
	}
	else		ReTemp = 0x00;	//数据接收错误  a.超时；b.数据错
	return ReTemp;
}
*/

/*
//发送云端数据
uint8_t SendPBus_Dat(uint8_t _uDat,uint8_t *_uBuff)
{
    uint8_t ReTemp=0;
	uint8_t Runningbuf[14]={0xF3,0x0E,0x08,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	Runningbuf[3] = _uDat;	//形参  逻辑地址
	Runningbuf[4] = _uBuff[0];	//卡SN 1
	Runningbuf[5] = _uBuff[1];	//卡SN 2
	Runningbuf[6] = _uBuff[2];	//卡SN 3
	Runningbuf[7] = _uBuff[3];	//卡SN 4
	Runningbuf[8] = _uBuff[4];	//卡内金额1 高位
	Runningbuf[9] = _uBuff[5];	//卡内金额2 高位
	Runningbuf[10] = _uBuff[6];	//卡内金额3 高位
	Runningbuf[11] = _uBuff[7];	//卡内校验数据
	Runningbuf[12] = _uBuff[8];	//通信码
	Runningbuf[Runningbuf[1]-1] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRC校验位
	PBus_Count = 0x00;
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);
	Overflow_Flag = 40;
    printf("Over_F = %d",Overflow_Flag);
	while( ( (PBus_Count&0x80)!=0x80 ) && (Overflow_Flag>0) ) //等待数据回复；
	{
		OSTimeDlyHMSM(0, 0, 0, 10);
		Overflow_Flag--;
		ReceivePowerBUSDat(0x01);	//带核验模式
	}
    printf("- %d;\r\n",Overflow_Flag);
	if( ( (PBus_Count&0x80) == 0x80 ))
	{
		PBus_Count = 0x00;
		if( PBusDat[3] == _uDat )
		{
			if( PBusDat[2] == 0x09 )	ReTemp = 0xAA;	//写入正确
			else                		ReTemp = 0x00;	//失败
		}
		else	ReTemp = 0x00;	//失败
	}
	else		ReTemp = 0x00;	//失败
	return ReTemp;
}*/

/*
//发送卡异常代码数据
uint8_t SendPBus_Dat2(uint8_t _uDat,uint8_t *_uBuff)
{
    uint8_t ReTemp=0;
	uint8_t Runningbuf[14]={0xF3,0x0A,0x13,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
	Runningbuf[3] = _uDat;	//形参  逻辑地址
	Runningbuf[4] = _uBuff[0];	//错误代码
	Runningbuf[5] = _uBuff[1];	//错误代码
	Runningbuf[6] = _uBuff[2];	//错误代码
	Runningbuf[7] = _uBuff[3];	//错误代码
	Runningbuf[8] = _uBuff[4];	//通信码
	Runningbuf[Runningbuf[1]-1] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRC校验位
	PBus_Count = 0x00;
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);
	Overflow_Flag = 40;
    printf("异常 = %d ",Overflow_Flag);
	while( ( (PBus_Count&0x80)!=0x80 ) && (Overflow_Flag>0) ) //等待数据回复；
	{
		OSTimeDlyHMSM(0, 0, 0, 10);
		Overflow_Flag--;
		ReceivePowerBUSDat(0x01);	//带核验模式
	}
    printf("- %d; ",Overflow_Flag);
	if( ( (PBus_Count&0x80) == 0x80 ))
	{
		PBus_Count = 0x00;
		if( PBusDat[3] == _uDat )
		{
			if( PBusDat[2] == 0x14 )	{ReTemp = 0xAA;printf("success \r\n");}	//写入正确
			else                		{ReTemp = 0x00;printf("fault   \r\n");}	//失败
		}
		else	{ReTemp = 0x00;printf("fault   \r\n");}	//失败
	}
	else		{ReTemp = 0x00;printf("fault   \r\n");}	//失败
	return ReTemp;
}
*/

/*
//发送异常数据
uint8_t SendPBus_ErrCom(uint8_t *_uBuff)
{
    uint8_t ReTemp=0;
	uint8_t Runningbuf[10]={0xF3,0x09,0x00,0x00,0x45,0x30,0x30,0x30,0x00,0x00};
	Runningbuf[2] = _uBuff[0];	//形参  命令帧
	Runningbuf[3] = _uBuff[1];	//形参  逻辑地址
	Runningbuf[5] = _uBuff[2];	//异常代码高位
	Runningbuf[6] = _uBuff[3];	//异常代码	
	Runningbuf[7] = _uBuff[4];	//异常代码低位
	Runningbuf[8] = _uBuff[5];	//通信码	
	if(Runningbuf[2]==0x11)			Runningbuf[1] = 0x09;//心跳包
	else if(Runningbuf[2]==0x13)	Runningbuf[1] = 0x0A;//插卡取卡
	Runningbuf[Runningbuf[1]-1] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRC校验位
	PBus_Count = 0x00;
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);
	Overflow_Flag = 40;
    printf("ErrCom %d ",Overflow_Flag);
	while( ( (PBus_Count&0x80)!=0x80 ) && (Overflow_Flag>0) ) //等待数据回复；
	{
		OSTimeDlyHMSM(0, 0, 0, 10);
		Overflow_Flag--;
		ReceivePowerBUSDat(0x01);	//带核验模式
	}
    printf("- %d;",Overflow_Flag);
	if( ( (PBus_Count&0x80) == 0x80 ))
	{
		PBus_Count = 0x00;
		if( PBusDat[3] == _uBuff[1] )	ReTemp = 0xAA;		//数据正确 
		else							ReTemp = 0x00;	//数据接收错误  a.超时；b.数据错
	}
	else		ReTemp = 0x00;	//数据接收错误  a.超时；b.数据错
	return ReTemp;
}*/


/*
//取回成功回复
//_uDat:逻辑地址
void ReadPBus_Succeed(uint8_t _uDat)
{
	uint8_t Runningbuf[5]={0xF3,0x05,0x07,0x00,0x00};
	Runningbuf[3] = _uDat;	//形参  逻辑地址
	Runningbuf[4] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRC校验位
	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);	//发送数据
    OSTimeDlyHMSM(0, 0, 0, 250);
}*/


//轮循方式取通信数据
//_no:逻辑地址
void ReadRunningData(uint8_t _no)
{	
	//1.判断该逻辑地址是否有使用；
	//			未使用	-->返回；
	//			使用	-->查询等待回复;
	//							-->无数据	-->返回；
	//							-->有数据	-->发送取数据将卡机数据取回
	//											并回复取回成功
	uint8_t uDat=0,TempBuff[10];
	uint8_t uBuff[8]={0},uTempSN[4]={0};
    printf("%2d; ",_no); 
	if((g_RUNDate[_no][0]&0x80) == 0x80)	//正常数据
	{	//该逻辑地址使用
		if((g_RUNDate[_no][1] == 0x0)&&(g_RUNDate[_no][4] == 0x0))	{g_RUNDate[_no][0] = 0x0;}
		LED1 = !LED1;//LED1 = 0;
		uDat = Can_SendPBus_Com(_no,(uint8_t *)TempBuff);		//发送轮循取数据命令
		//if( uDat == 0x00 )		uDat = Can_SendPBus_Com(_no,(uint8_t *)TempBuff);
		{
			//printf("uDat = %2d; ",uDat); 
			if( uDat == 0x02 )		//无数据	
			{
				g_RUNDate[_no][0] = g_RUNDate[_no][0]&(~0x23);//清失联标记、低三位
				KJ_NonResponse[_no]=0;
			}
			else if( uDat == 0x00 )	//失联
			{
				g_RUNDate[_no][0] = g_RUNDate[_no][0]|0x20;//写入失联标记
				if(KJ_NonResponse[_no]<20)
				{
					if(KJ_NonResponse[_no]%5==1)	
					{
						uTempSN[0] = g_RUNDate[_no][1];	uTempSN[1] = g_RUNDate[_no][2];	
						uTempSN[2] = g_RUNDate[_no][3];	uTempSN[3] = g_RUNDate[_no][4];	//卡机SN	
						Can_WriteLogitADD(_no,(uint8_t *)uTempSN);	//写入逻辑地址
					}
					KJ_NonResponse[_no] = KJ_NonResponse[_no]+1;
				}
				else	//超过次数，直接清该卡机
				{
					Delete_LogicADD(_no);	//分配不成功，清掉这逻辑地址
					KJ_Versions[_no] = 0;	KJ_NonResponse[_no]=0;
				}
			}	
			else//有数据
			{
				KJ_NonResponse[_no]=0;
				if(TempBuff[0]==0x05)		g_RUNDate[_no][0] = g_RUNDate[_no][0]|0x01;	//写入插卡标记
				else if(TempBuff[0]==0x06)	g_RUNDate[_no][0] = g_RUNDate[_no][0]|0x02;	//写入拔卡标记
				g_RUNDate[_no][5] = TempBuff[1];	//卡号1；
				g_RUNDate[_no][6] = TempBuff[2];	//卡号2；
				g_RUNDate[_no][7] = TempBuff[3];	//卡号3；
				g_RUNDate[_no][8] = TempBuff[4];	//卡号4；
				g_RUNDate[_no][9] = TempBuff[5];	//金额1；
				g_RUNDate[_no][10] = TempBuff[6];	//金额2；
				g_RUNDate[_no][11] = TempBuff[7];	//金额3；
				g_RUNDate[_no][12] = TempBuff[8];	//卡校验；
				g_RUNDate[_no][13] = TempBuff[9];	//通信码；
				printf("逻辑地址：%d;卡状态%d;  ",_no,g_RUNDate[_no][0]&0x03);
				printf("卡号：%02X%02X%02X%02X;  ",g_RUNDate[_no][5],g_RUNDate[_no][6],g_RUNDate[_no][7],g_RUNDate[_no][8]);
				printf("金额：%6d;   ", (g_RUNDate[_no][9]<<16)|(g_RUNDate[_no][10]<<8)|g_RUNDate[_no][11]);
				printf("卡校验：%2d;   通信码：%02X; \r\n", g_RUNDate[_no][12],g_RUNDate[_no][13]);
				//ReadPBus_Succeed(_no);//回复取回成功
				uBuff[0] = _no;	//逻辑地址
				uBuff[1] = TempBuff[1];	//卡号1；
				uBuff[2] = TempBuff[2];	//卡号2；
				uBuff[3] = TempBuff[3];	//卡号3；
				uBuff[4] = TempBuff[4];	//卡号4；
				if((g_RUNDate[_no][0]&0x03)==0x01)		uBuff[5] = 0xAA;	//卡状态 01表示插卡
				else if((g_RUNDate[_no][0]&0x03)==0x02)	uBuff[5] = 0x55;	//卡状态 02表示拔卡
				OSTimeDlyHMSM(0, 0, 0, 5);
				//printf("%02X %02X %02X %02X %02X %02X %02X %02X\r\n",uBuff[0],uBuff[1],uBuff[2],uBuff[3],uBuff[4],uBuff[5],uBuff[6],uBuff[7]);
				Can_ReadPBus_Succeed((uint8_t *)uBuff);//回复取回成功
				//OSTimeDlyHMSM(0, 0, 0, 50);
			}
		}
	}
}

//轮循方式取通信数据
void WriteRFIDData(uint8_t *_WriteDat)
{	
	uint8_t TempBuff[10]={0x00};
	uint8_t Runningbuf[8]={0x00},Recebuf[8]={0};
	uint8_t res=0,Overflow_Flag=10;
    LED1 = !LED1;//LED1 = 0;
	if(_WriteDat[0] == 0xAA)	//正常数据
	{
		g_RUNDate[_WriteDat[9]][0] = (g_RUNDate[_WriteDat[9]][0]&0x3F)|0x80;
		printf("\r\n服务器 %2d; ",_WriteDat[9]); 
		TempBuff[0] = _WriteDat[1];	//卡SN 1
		TempBuff[1] = _WriteDat[2];	//卡SN 2
		TempBuff[2] = _WriteDat[3];	//卡SN 3
		TempBuff[3] = _WriteDat[4];	//卡SN 4
		TempBuff[4] = _WriteDat[10];//通信码
		TempBuff[5] = _WriteDat[6];	//卡内金额1 高位
		TempBuff[6] = _WriteDat[7];	//卡内金额2 高位
		TempBuff[7] = _WriteDat[8];	//卡内金额3 高位
		TempBuff[8] = _WriteDat[5];	//卡内校验数据
		TempBuff[9] = _WriteDat[10];	//通信码
		
		///////发送数据包1
		Runningbuf[0] = _WriteDat[9];	//逻辑地址
		Runningbuf[1] = TempBuff[0];
		Runningbuf[2] = TempBuff[1];
		Runningbuf[3] = TempBuff[2];
		Runningbuf[4] = TempBuff[3];
		Runningbuf[5] = TempBuff[4];
		printf("%02X %02X %02X %02X %02X %02X %02X %02X\r\n",Runningbuf[0],Runningbuf[1],\
		Runningbuf[2],Runningbuf[3],Runningbuf[4],Runningbuf[5],Runningbuf[6],Runningbuf[7]);
		Package_Send(0x08,(u8 *)Runningbuf);
		OSTimeDlyHMSM(0,0,0,100);  //延时
		
		///////发送数据包2
		Runningbuf[0] = _WriteDat[9];	//逻辑地址
		Runningbuf[1] = TempBuff[5];
		Runningbuf[2] = TempBuff[6];
		Runningbuf[3] = TempBuff[7];
		Runningbuf[4] = TempBuff[8];
		Runningbuf[5] = TempBuff[9];
		Runningbuf[6] = CRC8_Table(TempBuff,10);	//CRC
		printf("%02X %02X %02X %02X %02X %02X %02X %02X\r\n",Runningbuf[0],Runningbuf[1],\
		Runningbuf[2],Runningbuf[3],Runningbuf[4],Runningbuf[5],Runningbuf[6],Runningbuf[7]);
		Package_Send(0x09,(u8 *)Runningbuf);
	}
	else if(_WriteDat[0] == 0xBB)	////插卡数据包 异常数据 与卡相关
	{
		printf("\r\n服务器 插卡数据包 异常 %2d; ",_WriteDat[1]); 
		Runningbuf[0] = _WriteDat[1];	//逻辑地址
		Runningbuf[1] = _WriteDat[2];	//卡号1
		Runningbuf[2] = _WriteDat[3];	//卡号2
		Runningbuf[3] = _WriteDat[4];	//卡号3
		Runningbuf[4] = _WriteDat[5];	//卡号4
		Runningbuf[5] = _WriteDat[6];	//错误代码
		Runningbuf[6] = _WriteDat[7];	//通信码
		Package_Send(0x13,(u8 *)Runningbuf);	//加入卡号识别，防止写错卡
	}
	else if(_WriteDat[0] == 0x55)	////心跳包 异常数据 与卡机相关
	{
		g_RUNDate[_WriteDat[1]][0] = (g_RUNDate[_WriteDat[1]][0]&0x3F)|0x40;
		printf("\r\n服务器 心跳包 异常 %2d; ",_WriteDat[1]); 
		Runningbuf[0] = _WriteDat[1];	//逻辑地址
		Runningbuf[1] = _WriteDat[2];	//错误代码"E"
		Runningbuf[2] = _WriteDat[3];	//错误代码高位
		Runningbuf[3] = _WriteDat[4];	//错误代码中位
		Runningbuf[4] = _WriteDat[5];	//错误代码低位
		Runningbuf[5] = 0;	//通信码
		Package_Send(0x11,(u8 *)Runningbuf);
	}
	else if(_WriteDat[0] == 0xCC)	////心跳包 正常数据
	{
		g_RUNDate[_WriteDat[7]][0] = 0x80;	//将卡机写为正常,写为正常后，插卡、拔卡数据包才有效。
		printf("\r\n服务器 心跳包 正常:%2d,水费:%2d,脉冲数:%2d;\r\n",_WriteDat[1],_WriteDat[5],_WriteDat[6]); 
		Runningbuf[0] = _WriteDat[7];	//逻辑地址
		g_WaterCost = _WriteDat[5];	//WaterCost=水费 最小扣款金额 0.005元
		g_CostNum = _WriteDat[6];	//流量计脉冲数 每升水计量周期
	}
	
	Overflow_Flag=10;
	while((res==0)&& (Overflow_Flag>0) )//接收到有数据
	{
		res=Can_Receive_Msg(Recebuf);
		OSTimeDlyHMSM(0, 0, 0, 5);
		Overflow_Flag--;
	}
//	if(res)
//	{
//		printf("%02X %02X %02X %02X %02X %02X %02X %02X\r\n",Recebuf[0],Recebuf[1],\
//		Recebuf[2],Recebuf[3],Recebuf[4],Recebuf[5],Recebuf[6],Recebuf[7]);
//	}
}


////写入逻辑地址
////_LogitADD：逻辑地址
//uint8_t WriteLogitADD(uint8_t _LogitADD,uint8_t *_PhysicalADD)
//{
//    uint8_t ReTemp=0;
//	uint8_t Runningbuf[9]={0xF3,0x09,0xC1,0x00,0x00,0x00,0x00,0x00,0x00};
//	Runningbuf[3] = *(_PhysicalADD+0);	//物理地址1
//	Runningbuf[4] = *(_PhysicalADD+1);	//物理地址2
//	Runningbuf[5] = *(_PhysicalADD+2);	//物理地址3
//	Runningbuf[6] = *(_PhysicalADD+3);	//物理地址4
//	Runningbuf[7] = _LogitADD;			//逻辑地址
//	Runningbuf[Runningbuf[1]-1] = CRC8_Table(Runningbuf,Runningbuf[1]-1);	//CRC校验位
//	PBus_Count = 0x00;
//	comSendBuf(COM2, (uint8_t *)Runningbuf, Runningbuf[1]);	//发送取数据命令
//	Overflow_Flag = 30;
//    printf("Over_Flag = %d",Overflow_Flag);  
//	while( ( (PBus_Count&0x80)!=0x80 ) && (Overflow_Flag>0) ) //等待数据回复；
//	{
//		OSTimeDlyHMSM(0, 0, 0, 10);
//		Overflow_Flag--;
//		ReceivePowerBUSDat(0x01);	//带核验模式
//	}
//    printf("- %d;",Overflow_Flag);  
//	if( ( (PBus_Count&0x80) == 0x80 ) &&(PBusDat[2] == 0xC2) )
//	{
//		PBus_Count = 0x00;
//		g_RUNDate[_LogitADD][0] = 0x80;	//表示卡机使用
//		ReTemp = PBusDat[8];			//数据正确 
//	}
//	else	ReTemp = 0x00;	//数据接收错误  a.超时；b.数据错
//	return 	ReTemp;
//}
//折半查找法 a.广播->等待数据接收->接收注册->等待超时并退出。没有接收到计时1秒超时机制
//_Dat:查找标记
uint8_t Binary_searchSN(void)
{	
	uint8_t u8Temp=0,uDat=0,uTCount=10,uTempSN[4]={0};
	printf("\r\n广播查找!\r\n");      
	Can_UnregisteredBroadcast();   //未注册广播
	while(uTCount--)
	{
		printf("%02d. ",uTCount);
		if(Can_ReadUnregistered((uint8_t *)uTempSN)!=0x00)  //有数据 检测一次100ms
		{
			uDat++;	uTCount = 10;	//读取到数据，重新计时1秒
            printf("物理：%02X%02X%02X%02X;",uTempSN[0],uTempSN[1],uTempSN[2],uTempSN[3]);   
            u8Temp = Distribute_LogicADD((uint8_t *)uTempSN);	//分配物理地址
            printf("逻辑:%0d;",u8Temp); 
            if( u8Temp <= ( BUSNUM_SIZE+1 ) )
			{
				KJ_Versions[u8Temp] = Can_WriteLogitADD(u8Temp,(uint8_t *)uTempSN);
				KJ_NonResponse[u8Temp]=0;
				if(KJ_Versions[u8Temp]==0x00)	printf("Ver[%02d]=%02X,分配失败！\r\n",u8Temp,KJ_Versions[u8Temp]);
				else
				{
					printf("Ver[%02d]=%02X,分配成功！\r\n",u8Temp,KJ_Versions[u8Temp]);
					++g_RUNDate[0][0]; 					g_RUNDate[u8Temp][0] = 0x80;		//总数+1；该地址使用；
					g_RUNDate[u8Temp][1] = uTempSN[0]; 	g_RUNDate[u8Temp][2] = uTempSN[1];	//卡机SN
					g_RUNDate[u8Temp][3] = uTempSN[2]; 	g_RUNDate[u8Temp][4] = uTempSN[3];
				}
			}
		}
	}
	printf("查询结束-----------\r\n");      
	return uDat;
}

void PrintfDat(void)
{
	uint8_t i,j;
	printf("\r\n");
	for(i=0;i<26;i++)
	{
		printf("%02d--",i);
		for(j=0;j<14;j++)	printf("%02X,",g_RUNDate[i][j]);
		printf("\r\n");
	}
}
//void PrintfDat2(void)
//{
//	uint8_t i,j;
//	printf("\r\n");
//	for(i=0;i<30;i++)
//	{
//		printf("%02d--",i);
//		//for(j=0;j<16;j++)	printf("%02X,",g_SendDate[i][j]);
//		printf("\r\n");
//	}
//}


