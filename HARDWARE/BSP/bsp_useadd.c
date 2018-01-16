#include "bsp_useadd.h"
#include "bsp_24cxx.h"
#include "bsp_myiic.h"
#include "bsp_powerbus.h"
#include "bsp_uart_fifo.h"

#include "delay.h"
#include "sys.h"
#include "string.h"
//#include "usmart.h"	
#include "includes.h"
#include "bsp_canapp.h"

#define ADD_SIZE	BUSNUM_SIZE
extern uint8_t g_RUNDate[BUSNUM_SIZE+1][14];    //运行数据；
extern uint8_t KJ_Versions[BUSNUM_SIZE];	//卡机版本号

/*
*********************************************************************************************************
*	函 数 名: Distribute_LogicADD(uint8_t Logic_no,uint8_t *Physical_no)
*	功能说明: 为物理地址分配逻辑地址并写入Flash
*	形    参: 
*	返 回 值: 
*********************************************************************************************************
*/
uint8_t Distribute_LogicADD(uint8_t *Physical_no)
{
	uint8_t i,Logic_Count;
	uint8_t u8Temp;
	uint16_t Temp;
	uint8_t Physical_Temp[5]={0};
	//1.先判断该物理地址是否在本机只已分配逻辑地址；
	for(i=1;i<(ADD_SIZE+1);i++)
	{
        Temp = (Start_ADD + ( i * 5 ))|Logic_ADD ;	//每组的起始地址
		u8Temp = AT24CXX_ReadOneByte(Temp);			//每组的逻辑地址
		if( u8Temp == i )		//表明本地址使用了；
        {
			AT24CXX_Read( (Temp + 1),(uint8_t *)Physical_Temp, 0x04 );
            if((*(Physical_no+3) == Physical_Temp[3])&&(*(Physical_no+2) == Physical_Temp[2]))
            {
                if((*(Physical_no+0) == Physical_Temp[0])&&(*(Physical_no+1) == Physical_Temp[1]))
                    return	i; 	//返回该物理地址的逻辑地址，退出
            }

        }
	}
    
	//为该物理地址分配逻辑地址
	for(i=1;i<(ADD_SIZE+1);i++)   
	{
		Temp = (Start_ADD + ( i * 5 ))|Logic_ADD ;
		u8Temp = AT24CXX_ReadOneByte( Temp );
		if( u8Temp != i) //表明本地址未使用；
		{
			AT24CXX_WriteOneByte( Temp, i );            
			AT24CXX_WriteOneByte( Temp+1, Physical_no[0] );            
			AT24CXX_WriteOneByte( Temp+2, Physical_no[1] );            
			AT24CXX_WriteOneByte( Temp+3, Physical_no[2] );            
			AT24CXX_WriteOneByte( Temp+4, Physical_no[3] );            
			Logic_Count = AT24CXX_ReadOneByte(COUNT_ADD);   //读取现已录入 数量；
			AT24CXX_WriteOneByte(COUNT_ADD,Logic_Count+1);
			return i;
		}
	}
    
	return 0xF3;//	错误代码
} 

/*
*********************************************************************************************************
*	函 数 名: Delete_LogicADD((uint8_t Logic_no)
*	功能说明: 删除指定的逻辑地址信息
*	形    参: 
*	返 回 值: 
*********************************************************************************************************
*/
uint8_t Delete_LogicADD(uint8_t Logic_no)
{
	uint8_t i,Logic_Count;
	uint8_t Physical_no[5]={0};
    for(i=1;i<(ADD_SIZE+1);i++)
    {
        if( AT24CXX_ReadOneByte( (Start_ADD + ( i * 5 ))|Logic_ADD ) == Logic_no)
        {
            AT24CXX_Write((Start_ADD + ( i * 5 )) | Physical_ADD1, Physical_no, 0x04);
            AT24CXX_WriteOneByte( (Start_ADD + ( i * 5 )) | Logic_ADD, 0x00 );            
        }
    }
	g_RUNDate[Logic_no][0] = 0x00;
	Logic_Count = AT24CXX_ReadOneByte(COUNT_ADD);   //读取现已录入 数量；
	AT24CXX_WriteOneByte(COUNT_ADD,Logic_Count-1);
	return (Logic_Count-1);
}


/*
*********************************************************************************************************
*	函 数 名: PowerUPLogitADDCheck((uint8_t Logic_no)
*	功能说明: 上电检测Flash内数据是否在线并分配逻辑地址
*	形    参: 无
*	返 回 值: 无
*********************************************************************************************************
*/
void PowerUPLogitADDCheck(void)
{
	uint8_t i;
	uint16_t Temp;
	uint8_t Physical_Temp[5]={0};
	for(i=1;i<(ADD_SIZE+1);i++)
	{
        Temp = (Start_ADD + ( i * 5 ))|Logic_ADD ;	//每组的起始地址  逻辑地址数据存储地址
		if( AT24CXX_ReadOneByte(Temp) == i )		//表明本地址使用了；
		{
			AT24CXX_Read( (Temp + 1),(uint8_t *)Physical_Temp, 0x04 );	//读取物理地址
            printf("++起始:%03d; 逻辑:%02d; 物理：%02X%02X%02X%02X; ",Temp,i,Physical_Temp[0],Physical_Temp[1],Physical_Temp[2],Physical_Temp[3]);
			if((Physical_Temp[0]==0x00)&&(Physical_Temp[1]==0x00)&&(Physical_Temp[2]==0x00)&&(Physical_Temp[3]==0x00))
			{
				printf("\r\n物理地址不能为0x00！清逻辑地址:%d;\r\n",i);               
				Delete_LogicADD(i);	//分配不成功，清掉这逻辑地址	
				KJ_Versions[i]=0;				
			}
			else
			{
				KJ_Versions[i] = Can_WriteLogitADD(i,(uint8_t *)Physical_Temp);
				if(KJ_Versions[i] != 0x00)	//分别逻辑地址成功
				{
					++g_RUNDate[0][0]; 		//总数+1；该地址使用；
					g_RUNDate[i][0] = 0x80;	//总数+1；该地址使用；
					g_RUNDate[i][1] = Physical_Temp[0]; g_RUNDate[i][2] = Physical_Temp[1];	//卡机SN
					g_RUNDate[i][3] = Physical_Temp[2]; g_RUNDate[i][4] = Physical_Temp[3];
					printf("Ver[%02d]=%02X\r\n",i,KJ_Versions[i]);               
				}
				else
				{
					printf("------分配失败!\r\n");
					OSTimeDlyHMSM(0, 0, 0, 50);					
					KJ_Versions[i] = Can_WriteLogitADD(i,(uint8_t *)Physical_Temp);
					if(KJ_Versions[i] != 0x00)	//分别逻辑地址成功
					{
						++g_RUNDate[0][0]; 		//总数+1；该地址使用；
						g_RUNDate[i][0] = 0x80;	//总数+1；该地址使用；
						g_RUNDate[i][1] = Physical_Temp[0]; g_RUNDate[i][2] = Physical_Temp[1];	//卡机SN
						g_RUNDate[i][3] = Physical_Temp[2]; g_RUNDate[i][4] = Physical_Temp[3];
						printf("Ver[%02d]=%02X,分配成功！\r\n",i,KJ_Versions[i]);               
					}
					else
					{
						printf("二次分配失败！\r\n----------------------------------清逻辑地址:%d;----------------------\r\n",i);               
						Delete_LogicADD(i);	//分配不成功，清掉这逻辑地址					
					}
				}
			}
		}
		OSTimeDlyHMSM(0,0,0,200);
	}
}

////用于通电过程掉电，重复注册；
//void LogitADDWrite(uint8_t _AddDat)
//{
//	uint8_t i;
//	uint16_t Temp;
//	uint8_t Physical_Temp[5]={0};
//	i = _AddDat;
//	Temp = (Start_ADD + ( i * 5 ))|Logic_ADD ;	//每组的起始地址  逻辑地址数据存储地址
//	if( AT24CXX_ReadOneByte(Temp) == i )		//表明本地址使用了；
//	{
//		AT24CXX_Read( (Temp + 1),(uint8_t *)Physical_Temp, 0x04 );	//读取物理地址
//		printf("++起始地址:%d;  逻辑地址:%d;  物理地址：%02X%02X%02X%02X;  \r\n",Temp,i,Physical_Temp[0],Physical_Temp[1],Physical_Temp[2],Physical_Temp[3]);
//		KJ_Versions[i] = WriteLogitADD(i,(uint8_t *)Physical_Temp);
//		if(KJ_Versions[i] != 0x00)	//分别逻辑地址成功
//		//if(WriteLogitADD(i,(uint8_t *)Physical_Temp) != 0x00)	//分别逻辑地址成功
//		{
//			++g_RUNDate[0][0]; g_RUNDate[i][0] = 0xAA;	//总数+1；该地址使用；
//			g_RUNDate[i][3] = Physical_Temp[0]; g_RUNDate[i][4] = Physical_Temp[1];	//卡机SN
//			g_RUNDate[i][5] = Physical_Temp[2]; g_RUNDate[i][6] = Physical_Temp[3];
//			printf("分配成功！\r\n");               
//		}
//		else
//		{
//			printf("分配失败！\r\n");               
//		}
//	}
//}

void Read_RFID_Key(uint8_t *RFID_Key_Temp)
{
	AT24CXX_Read( 930,(uint8_t *)RFID_Key_Temp, 0x07 );	//读取RFID_Key数据
}

void Write_RFID_Key(uint8_t *RFID_Key_Temp)//RFID_Key数据
{
	AT24CXX_WriteOneByte( 930, RFID_Key_Temp[0] );            
	AT24CXX_WriteOneByte( 931, RFID_Key_Temp[1] );            
	AT24CXX_WriteOneByte( 932, RFID_Key_Temp[2] );            
	AT24CXX_WriteOneByte( 933, RFID_Key_Temp[3] );            
	AT24CXX_WriteOneByte( 934, RFID_Key_Temp[4] );            
	AT24CXX_WriteOneByte( 935, RFID_Key_Temp[5] );            
	AT24CXX_WriteOneByte( 936, RFID_Key_Temp[6] );            
}
void Read_IPAdd(uint8_t *IPAdd_Temp)
{
	AT24CXX_Read( 900,(uint8_t *)IPAdd_Temp, 0x04 );	//读取IP地址
}

void Write_IPAdd(uint8_t *IPAdd_Temp)//IP地址
{
	AT24CXX_WriteOneByte( 900, IPAdd_Temp[0] );            
	AT24CXX_WriteOneByte( 901, IPAdd_Temp[1] );            
	AT24CXX_WriteOneByte( 902, IPAdd_Temp[2] );            
	AT24CXX_WriteOneByte( 903, IPAdd_Temp[3] );            
}
uint16_t Read_PortAdd(void) //端口号
{
	uint8_t _uTemp[2]={0};
	AT24CXX_Read( 904,(uint8_t *)_uTemp, 0x02 );	
	return ((_uTemp[0]<<8)|_uTemp[1]);
}

void Write_PortAdd(uint16_t PortAdd_Temp)	//端口号
{
	uint16_t Temp = 904 ;
	AT24CXX_WriteOneByte( Temp+0, (PortAdd_Temp/256) );            
	AT24CXX_WriteOneByte( Temp+1, (PortAdd_Temp%256) );            
}
void Read_ACUSN(uint8_t *ACUSN_Temp)//区域控制器SN
{
	AT24CXX_Read( 906,(uint8_t *)ACUSN_Temp, 0x04 );	
}

void Write_ACUSN(uint8_t *ACUSN_Temp)	//区域控制器SN
{
	uint16_t Temp = 906 ;
	AT24CXX_WriteOneByte( Temp+0, ACUSN_Temp[0] );            
	AT24CXX_WriteOneByte( Temp+1, ACUSN_Temp[1] );            
	AT24CXX_WriteOneByte( Temp+2, ACUSN_Temp[2] );            
	AT24CXX_WriteOneByte( Temp+3, ACUSN_Temp[3] );            
}
void Read_ACUAdd(uint8_t *ACUAdd_Temp)//通信码 16位
{
	AT24CXX_Read( 910,(uint8_t *)ACUAdd_Temp, 16 );	
}

void Write_ACUAdd(uint8_t *ACUAdd_Temp)//通信码 16位
{
	uint16_t Temp = 910 ;
	AT24CXX_WriteOneByte( Temp+0, ACUAdd_Temp[0] );            
	AT24CXX_WriteOneByte( Temp+1, ACUAdd_Temp[1] );            
	AT24CXX_WriteOneByte( Temp+2, ACUAdd_Temp[2] );            
	AT24CXX_WriteOneByte( Temp+3, ACUAdd_Temp[3] );            
	AT24CXX_WriteOneByte( Temp+4, ACUAdd_Temp[4] );            
	AT24CXX_WriteOneByte( Temp+5, ACUAdd_Temp[5] );            
	AT24CXX_WriteOneByte( Temp+6, ACUAdd_Temp[6] );            
	AT24CXX_WriteOneByte( Temp+7, ACUAdd_Temp[7] );            
	AT24CXX_WriteOneByte( Temp+8, ACUAdd_Temp[8] );            
	AT24CXX_WriteOneByte( Temp+9, ACUAdd_Temp[9] );            
	AT24CXX_WriteOneByte( Temp+10, ACUAdd_Temp[10] );            
	AT24CXX_WriteOneByte( Temp+11, ACUAdd_Temp[11] );            
	AT24CXX_WriteOneByte( Temp+12, ACUAdd_Temp[12] );            
	AT24CXX_WriteOneByte( Temp+13, ACUAdd_Temp[13] );            
	AT24CXX_WriteOneByte( Temp+14, ACUAdd_Temp[14] );            
	AT24CXX_WriteOneByte( Temp+15, ACUAdd_Temp[15] );            
}
void Write_localIP(uint8_t *_Temp)	//本机IP
{
	uint16_t Temp = 939 ;
	AT24CXX_WriteOneByte( Temp+0, _Temp[0] );            
	AT24CXX_WriteOneByte( Temp+1, _Temp[1] );            
	AT24CXX_WriteOneByte( Temp+2, _Temp[2] );            
	AT24CXX_WriteOneByte( Temp+3, _Temp[3] );            
	AT24CXX_WriteOneByte( Temp+4, _Temp[4] );            
}
void Write_netmask(uint8_t *_Temp)	//子网掩码:255.255.255.0
{
	uint16_t Temp = 944 ;
	AT24CXX_WriteOneByte( Temp+0, _Temp[0] );            
	AT24CXX_WriteOneByte( Temp+1, _Temp[1] );            
	AT24CXX_WriteOneByte( Temp+2, _Temp[2] );            
	AT24CXX_WriteOneByte( Temp+3, _Temp[3] );            
}
void Write_gateway(uint8_t *_Temp)	//默认网关:192.168.1.1
{
	uint16_t Temp = 948 ;
	AT24CXX_WriteOneByte( Temp+0, _Temp[0] );            
	AT24CXX_WriteOneByte( Temp+1, _Temp[1] );            
	AT24CXX_WriteOneByte( Temp+2, _Temp[2] );            
	AT24CXX_WriteOneByte( Temp+3, _Temp[3] );            
}
void Read_localIP(uint8_t *_Temp)
{
	AT24CXX_Read( 939,(uint8_t *)_Temp, 0x05 );	//读取本机IP地址
}
void Read_netmask(uint8_t *_Temp)
{
	AT24CXX_Read( 944,(uint8_t *)_Temp, 0x04 );	//子网掩码
}
void Read_gateway(uint8_t *_Temp)
{
	AT24CXX_Read( 948,(uint8_t *)_Temp, 0x04 );	//默认网关
}

