#include "stm32f10x.h"
#include <stdio.h>

#include "bsp_can.h"

extern uint8_t Physical_ADD[4];//�����ַ


//CAN��ʼ��
//tsjw:����ͬ����Ծʱ�䵥Ԫ.��Χ:1~3; CAN_SJW_1tq	 CAN_SJW_2tq CAN_SJW_3tq CAN_SJW_4tq
//tbs2:ʱ���2��ʱ�䵥Ԫ.��Χ:1~8;
//tbs1:ʱ���1��ʱ�䵥Ԫ.��Χ:1~16;	  CAN_BS1_1tq ~CAN_BS1_16tq
//brp :�����ʷ�Ƶ��.��Χ:1~1024;(ʵ��Ҫ��1,Ҳ����1~1024) tq=(brp)*tpclk1
//ע�����ϲ����κ�һ����������Ϊ0,�������.
//������=Fpclk1/((tsjw+tbs1+tbs2)*brp);
//mode:0,��ͨģʽ;1,�ػ�ģʽ;
//Fpclk1��ʱ���ڳ�ʼ����ʱ������Ϊ36M,�������CAN_Normal_Init(1,8,7,5,1);
//������Ϊ:36M/((1+8+7)*5)=450Kbps
//����ֵ:0,��ʼ��OK;
//    ����,��ʼ��ʧ��;


u8 CAN_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode)
{

	GPIO_InitTypeDef GPIO_InitStructure; 
	CAN_InitTypeDef        CAN_InitStructure;
 	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
#if CAN_RX0_INT_ENABLE 
   	NVIC_InitTypeDef  NVIC_InitStructure;
#endif

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);//ʹ��PORTAʱ��	                   											 

  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//ʹ��CAN1ʱ��	

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//��������
    GPIO_Init(GPIOA, &GPIO_InitStructure);		//��ʼ��IO
   
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;//��������
    GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��IO
	  
 	//CAN��Ԫ����
 	CAN_InitStructure.CAN_TTCM=DISABLE;						 //��ʱ�䴥��ͨ��ģʽ  //
 	CAN_InitStructure.CAN_ABOM=DISABLE;						 //����Զ����߹���	 //
  	CAN_InitStructure.CAN_AWUM=DISABLE;						 //˯��ģʽͨ���������(���CAN->MCR��SLEEPλ)//
  	CAN_InitStructure.CAN_NART=ENABLE;						 	//��ֹ�����Զ����� //
  	CAN_InitStructure.CAN_RFLM=DISABLE;						 //���Ĳ�����,�µĸ��Ǿɵ� // 
  	CAN_InitStructure.CAN_TXFP=DISABLE;						 //���ȼ��ɱ��ı�ʶ������ //
  	CAN_InitStructure.CAN_Mode= mode;	         //ģʽ���ã� mode:0,��ͨģʽ;1,�ػ�ģʽ; //
  	//���ò�����
  	CAN_InitStructure.CAN_SJW=tsjw;				//����ͬ����Ծ���(Tsjw)Ϊtsjw+1��ʱ�䵥λ  CAN_SJW_1tq	 CAN_SJW_2tq CAN_SJW_3tq CAN_SJW_4tq
  	CAN_InitStructure.CAN_BS1=tbs1; //Tbs1=tbs1+1��ʱ�䵥λCAN_BS1_1tq ~CAN_BS1_16tq
  	CAN_InitStructure.CAN_BS2=tbs2;//Tbs2=tbs2+1��ʱ�䵥λCAN_BS2_1tq ~	CAN_BS2_8tq
  	CAN_InitStructure.CAN_Prescaler=brp;            //��Ƶϵ��(Fdiv)Ϊbrp+1	//
  	CAN_Init(CAN1, &CAN_InitStructure);            // ��ʼ��CAN1 

 	CAN_FilterInitStructure.CAN_FilterNumber=0;	  //������0
 	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask; 
  	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit; //32λ 
  	CAN_FilterInitStructure.CAN_FilterIdHigh=0x0000;////32λID
  	CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
  	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;//32λMASK
  	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;
  	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO0;//������0������FIFO0
 	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE; //���������0

  	CAN_FilterInit(&CAN_FilterInitStructure);//�˲�����ʼ��
#if CAN_RX0_INT_ENABLE
	
	CAN_ITConfig(CAN1,CAN_IT_FMP0,ENABLE);//FIFO0��Ϣ�Һ��ж�����.		    
  
  	NVIC_InitStructure.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;     // �����ȼ�Ϊ1
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;            // �����ȼ�Ϊ0
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);
#endif
	return 0;
}   
 
#if CAN_RX0_INT_ENABLE	//ʹ��RX0�ж�
//�жϷ�����			    
void USB_LP_CAN1_RX0_IRQHandler(void)
{
  	CanRxMsg RxMessage;
	int i=0;
    CAN_Receive(CAN1, 0, &RxMessage);
	for(i=0;i<8;i++)
	printf("rxbuf[%d]:%d\r\n",i,RxMessage.Data[i]);
}
#endif

//can����һ������(�̶���ʽ:IDΪ0X12,��׼֡,����֡)	
//len:���ݳ���(���Ϊ16)				     
//msg:����ָ��,���Ϊ8���ֽ�.
//����ֵ:0,�ɹ�;	����,ʧ��;
u8 Can_Send_Msg(u8* msg,u8 len)
{	
	u8 mbox;
	u16 i=0;
	CanTxMsg TxMessage;
	if((msg[0]==0x01)||(msg[0]==0x07)||(msg[0]==0x08)||(msg[0]==0x09))	
		TxMessage.StdId=0x200|msg[1];	// ��׼��ʶ��Ϊ0
	else				
		TxMessage.StdId=0x200;			// ��׼��ʶ��Ϊ0
	TxMessage.ExtId=0x1800F001;			// ������չ��ʾ����29λ��
	TxMessage.IDE=0;		// ��ʹ����չ��ʶ��
	TxMessage.RTR=0;		// ��Ϣ����Ϊ����֡��һ֡8λ
	TxMessage.DLC=len;		// ������֡��Ϣ
	for(i=0;i<8;i++)
	TxMessage.Data[i]=msg[i];	// ֡��Ϣ          
	mbox= CAN_Transmit(CAN1, &TxMessage);   
	i=0;
	while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//�ȴ����ͽ���
	if(i>=0XFFF)return 1;
	return 0;		

}
//can�ڽ������ݲ�ѯ
//buf:���ݻ�����;	 
//����ֵ:0,�����ݱ��յ�;	 ����,���յ����ݳ���;
u8 Can_Receive_Msg(u8 *buf)
{		   		   
 	u32 i;
	CanRxMsg RxMessage;
    if( CAN_MessagePending(CAN1,CAN_FIFO0)==0)return 0;		//û�н��յ�����,ֱ���˳� 
    CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);				//��ȡ����	
    for(i=0;i<8;i++)    buf[i]=RxMessage.Data[i];  
	return RxMessage.DLC;	
}

void Package_Send(u8 _mode,u8 *Package_Dat)
{
	//uint8_t res;
	uint8_t Package_SendBuf[8]={0x00};//���ͻ�����
	switch (_mode)
    {
    	case 0x01:	//δע��㲥
			Package_SendBuf[0] = 0xA1;
			Package_SendBuf[1] = 0xA1;
			break;
    	case 0x02:	//ע�� (д���߼���ַ)
			Package_SendBuf[0] = 0xC1;
			Package_SendBuf[1] = Package_Dat[0];	//�����ַ
 			Package_SendBuf[2] = Package_Dat[1];	//�����ַ
			Package_SendBuf[3] = Package_Dat[2];	//�����ַ
			Package_SendBuf[4] = Package_Dat[3];	//�����ַ
			Package_SendBuf[5] = Package_Dat[4];	//�߼���ַ
    		break;
    	case 0x03:	//�㲥RFID����Ϣ
 			Package_SendBuf[0] = 0xA2;
			Package_SendBuf[1] = Package_Dat[0];	//FM1702_Key[0]
 			Package_SendBuf[2] = Package_Dat[1];	//FM1702_Key[1]
 			Package_SendBuf[3] = Package_Dat[2];	//FM1702_Key[2]
			Package_SendBuf[4] = Package_Dat[3];	//FM1702_Key[3]
			Package_SendBuf[5] = Package_Dat[4];	//FM1702_Key[4]
			Package_SendBuf[6] = Package_Dat[5];	//FM1702_Key[5]
			Package_SendBuf[7] = Package_Dat[6];	//���ַ
			break;
     	case 0x04:	//�㲥ˮ��
 			Package_SendBuf[0] = 0xA3;
			Package_SendBuf[1] = Package_Dat[0];	//��С�ۿ���
 			Package_SendBuf[2] = Package_Dat[1];	//�Ʒ�����
			break;
		case 0x05:	//��ѭ����
 			Package_SendBuf[0] = 0x01;
			Package_SendBuf[1] = Package_Dat[0];	//�߼���ַ
			break;
		case 0x06:	//ȡ�����ݳɹ�,�ظ�
 			Package_SendBuf[0] = 0x07;
			Package_SendBuf[1] = Package_Dat[0];	//�߼���ַ
 			Package_SendBuf[2] = Package_Dat[1];	//����1
 			Package_SendBuf[3] = Package_Dat[2];	//����2
			Package_SendBuf[4] = Package_Dat[3];	//����3
			Package_SendBuf[5] = Package_Dat[4];	//����4
			Package_SendBuf[6] = Package_Dat[5];	//�忨/�ο�
			break;
		case 0x08:	//����������д��1
 			Package_SendBuf[0] = 0x08;
			Package_SendBuf[1] = Package_Dat[0];	//�߼���ַ
 			Package_SendBuf[2] = Package_Dat[1];	//����1
 			Package_SendBuf[3] = Package_Dat[2];	//����2
			Package_SendBuf[4] = Package_Dat[3];	//����3
			Package_SendBuf[5] = Package_Dat[4];	//����4
			Package_SendBuf[6] = Package_Dat[5];	//ͨ����
			break;
		case 0x09:	//����������д��2
 			Package_SendBuf[0] = 0x09;
			Package_SendBuf[1] = Package_Dat[0];	//�߼���ַ
 			Package_SendBuf[2] = Package_Dat[1];	//���1
 			Package_SendBuf[3] = Package_Dat[2];	//���2
			Package_SendBuf[4] = Package_Dat[3];	//���3
			Package_SendBuf[5] = Package_Dat[4];	//��У��
			Package_SendBuf[6] = Package_Dat[5];	//ͨ����
			Package_SendBuf[7] = Package_Dat[6];	//CRC
			break;
		case 0x11:	//�������
 			Package_SendBuf[0] = 0x11;
			Package_SendBuf[1] = Package_Dat[0];	//�߼���ַ
 			Package_SendBuf[2] = Package_Dat[1];	//�������
 			Package_SendBuf[3] = Package_Dat[2];	//�������
			Package_SendBuf[4] = Package_Dat[3];	//�������
			Package_SendBuf[5] = Package_Dat[4];	//�������
			Package_SendBuf[6] = Package_Dat[5];	//ͨ����
			break;
    	default:
    		break;
    }
	//printf("CAN_Send:%02X %02X %02X %02X %02X %02X %02X %02X\r\n",Package_SendBuf[0],Package_SendBuf[1],\
	Package_SendBuf[2],Package_SendBuf[3],Package_SendBuf[4],Package_SendBuf[5],Package_SendBuf[6],Package_SendBuf[7]);
	//res = Can_Send_Msg(Package_SendBuf,8);//����8���ֽ�
	Can_Send_Msg(Package_SendBuf,8);//����8���ֽ�
	//if(res)	printf(" F, ");	
	//else 	printf(" S, ");	
}



