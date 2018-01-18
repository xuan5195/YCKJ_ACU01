/*******************************************************************************
** �ļ���: 		ymodem.c
** �汾��  		1.0
** ��������: 	RealView MDK-ARM 4.14
** ����: 		wuguoyana
** ��������: 	2011-04-29
** ����:		��Ymodem.c����ص�Э���ļ�
                ����ӳ����ն˽�������(ʹ��YmodemЭ��)���������ݼ��ص��ڲ�RAM�С�
                ����������������������ݱ�̵�Flash�У����������������ʾ����
** ����ļ�:	stm32f10x.h
** �޸���־��	2011-04-29   �����ĵ�
*******************************************************************************/

/* ����ͷ�ļ� *****************************************************************/

#include "includes.h"
#include "stm32f10x_flash.h"
#include "ymodem.h"
#include "common.h"
#include "bsp_spi_flash.h"
#include "bsp_spi_bus.h"
#include "bsp_serialport.h"
#include "bsp_uart_fifo.h"

/* �������� -----------------------------------------------------------------*/
uint8_t file_name[FILE_NAME_LENGTH];
//�û�����Flashƫ��
//uint32_t FlashDestination = ApplicationAddress;
uint16_t PageSize = PAGE_SIZE;
uint32_t EraseCounter = 0x0;
uint32_t NbrOfPage = 0;
FLASH_Status FLASHStatus = FLASH_COMPLETE;
uint32_t RamSource;
uint8_t tab_1024[1024] = {0};
uint8_t g_Head_flag;
uint16_t g_Packet_Count;
/*******************************************************************************
  * @��������	Receive_Byte
  * @����˵��   �ӷ��Ͷ˽���һ���ֽ�
  * @�������   c: �����ַ�
                timeout: ��ʱʱ��
  * @�������   ��
  * @���ز���   ���յĽ��
                0���ɹ�����
                1��ʱ�䳬ʱ
*******************************************************************************/
static  int32_t Receive_Byte (uint8_t *c, uint32_t timeout)
{
    while (timeout-- > 0)
    {
        if (SerialKeyPressed(c) == 1)    return 0;
    }
    return -1;
}

/*******************************************************************************
  * @��������	Send_Byte
  * @����˵��   ����һ���ַ�
  * @�������   c: ���͵��ַ�
  * @�������   ��
  * @���ز���   ���͵Ľ��
                0���ɹ�����
*******************************************************************************/
static uint32_t Send_Byte (uint8_t c)
{
    SerialPutChar(c);
    return 0;
}

/*******************************************************************************
  * @��������	Receive_Packet
  * @����˵��   �ӷ��Ͷ˽���һ�����ݰ�
  * @�������   data ������ָ��
                length������
                timeout ����ʱʱ��
  * @�������   ��
  * @���ز���   ���յĽ��
                0: ��������
                -1: ��ʱ�������ݰ�����
                1: �û�ȡ��
*******************************************************************************/
static int32_t Receive_Packet (uint8_t *data, int32_t *length, uint32_t timeout)
{
    uint8_t read;
    uint16_t i, packet_size;
	if (comGetChar(SERIALPORT_COM, &read))    //���յ���������
	{
		if(g_Head_flag == 0)	//����ͷ����Ϣ
		{
			switch (read)	//c: �����ַ�
			{
				case SOH:	//YModemЭ��ͷ������Ϊ128Byte
					packet_size = PACKET_SIZE;
					break;
				case STX:	//YModemЭ��ͷ������Ϊ1024Byte
					packet_size = PACKET_1K_SIZE;
					break;
				case EOT:	//�������
					return 0;
				case CA:
//					if ((Receive_Byte(&c, timeout) == 0) && (c == CA))
//					{
//						*length = -1;
//						return 0;
//					}
//					else
					{
						return -1;
					}
				case ABORT1:
				case ABORT2:
					return 1;
				default:
					return -1;
			}
			g_Head_flag = 0xAA;g_Packet_Count=1;*data = read;
		}
		else
		{
			if( g_Packet_Count < (packet_size + PACKET_OVERHEAD))
			{	data++;	*data = read;	}
			else
			{
				if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff))	return -1;
				*length = packet_size;
				return 0;
			}
		}
	}
}

int32_t Ymodem_Receive (uint8_t *buf)
{
	uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD], file_size[FILE_SIZE_LENGTH], *file_ptr, *buf_ptr;
	int32_t i, packet_length, session_done, file_done, packets_received, errors, session_begin, size = 0;
	uint32_t FlashDestination = 0x010000;//ApplicationAddress;	//���APP������ʼ��ַ
	for (session_done = 0, errors = 0, session_begin = 0; ;)//��ѭ����һ��ymodem����
	{
		for (packets_received = 0, file_done = 0, buf_ptr = buf; ;)//��ѭ�������Ͻ�������
		{
			switch (Receive_Packet(packet_data, &packet_length, NAK_TIMEOUT))//�������ݰ�
			{
				case 0:
					errors = 0;
					switch (packet_length)
					{
						/* Abort by sender */
						case - 1: //���Ͷ���ֹ����
							Send_Byte(ACK);//�ظ�ACK
							return 0;
						/* End of transmission */
						case 0://���ս�������մ���
							Send_Byte(ACK);
							file_done = 1;//�������
							break;
						/* Normal packet */
						default://����������
							if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff))
							{
								Send_Byte(NAK);//���մ�������ݣ��ظ�NAK
							}
							else//���յ���ȷ������
							{
								if (packets_received == 0)//���յ�һ֡����
								{
									printf("\r\n���յ�һ֡����\r\n"); 
									/* Filename packet */
									if (packet_data[PACKET_HEADER] != 0)//�����ļ���Ϣ���ļ������ļ����ȵ�
									{
										/* Filename packet has valid data */
										for (i = 0, file_ptr = packet_data + PACKET_HEADER; (*file_ptr != 0) && (i < FILE_NAME_LENGTH);)
										{
											file_name[i++] = *file_ptr++;//�����ļ���
										}
										file_name[i++] = '\0';//�ļ�����'\0'����
										for (i = 0, file_ptr ++; (*file_ptr != ' ') && (i < FILE_SIZE_LENGTH);)
										{
											file_size[i++] = *file_ptr++;//�����ļ���С
										}
										file_size[i++] = '\0';//�ļ���С��'\0'����
										Str2Int(file_size, &size);

										/* Test the size of the image to be sent */
										/* Image size is greater than Flash size */
										if (size > (FLASH_SIZE - 1))//�����̼�����
										{
											/* End session */
											Send_Byte(CA);
											Send_Byte(CA);//��������2����ֹ��CA
											return -1;//����
										}
//										NbrOfPage = FLASH_PagesMask(size);	//�������Ҫ����ҳ����
//
//										/* Erase the FLASH pages */ //������Ӧ��flash�ռ�
//										for (EraseCounter = 0; (EraseCounter < NbrOfPage) && (FLASHStatus == FLASH_COMPLETE); EraseCounter++)
//										{
//											FLASHStatus = sf_EraseSector(FlashDestination + (PageSize * EraseCounter));
//										}
										sf_EraseSector(0x010000);		//�������� 16 4K
										sf_EraseSector(0x020000);		//�������� 17 4K
										sf_EraseSector(0x030000);		//�������� 18 4K
										sf_EraseSector(0x040000);		//�������� 19 4K
										sf_EraseSector(0x050000);		//�������� 20 4K
										sf_EraseSector(0x060000);		//�������� 21 4K
										sf_EraseSector(0x070000);		//�������� 22 4K
										sf_EraseSector(0x080000);		//�������� 23 4K
										Send_Byte(ACK);//�ظ�ACk
										Send_Byte(CRC16);//����'C',ѯ������
										printf("�ظ�ACk������'C',ѯ������\r\n"); 
									}
									else//�ļ������ݰ�Ϊ�գ���������
									{
										Send_Byte(ACK);//�ظ�ACK
										file_done = 1;//ֹͣ����
										session_done = 1;//�����Ի�
										break;
									}
								}
								else//�յ����ݰ�
								{
									memcpy(buf_ptr, packet_data + PACKET_HEADER, packet_length);//��������
//									RamSource = (uint32_t)buf;//8λǿ��ת����32Ϊ����
//									for (j = 0;(j < packet_length) && (FlashDestination <  ApplicationAddress + size);j += 4)
//									{
//										/* Program the data received into STM32F10x Flash */
//										FLASH_ProgramWord(FlashDestination, *(uint32_t*)RamSource);//��д��������
//
//										if (*(uint32_t*)FlashDestination != *(uint32_t*)RamSource)
//										{
//											/* End session */
//											Send_Byte(CA);
//											Send_Byte(CA);//flash��д������������2����ֹ��CA
//											return -2;//��д����
//										}
//										FlashDestination += 4;
//										RamSource += 4;
//									}
									if (sf_WriteBuffer(buf, FlashDestination, packet_length) == 0)
									{
										Send_Byte(CA);
										Send_Byte(CA);//flash��д������������2����ֹ��CA
										return -2;//��д����
									}
									FlashDestination = FlashDestination + packet_length;
									Send_Byte(ACK);//flash��д�ɹ����ظ�ACK
								}
								packets_received ++;//�յ����ݰ��ĸ���
								session_begin = 1;//���ý����б�־
							}
					}
					break;
				case 1://�û���ֹ
					Send_Byte(CA);
					Send_Byte(CA);//��������2����ֹ��CA
					return -3;//��д��ֹ
				default:
					if (session_begin > 0)//��������з�������
					{
						errors ++;
					}
					if (errors > MAX_ERRORS)//���󳬹�����
					{
						Send_Byte(CA);
						Send_Byte(CA);//��������2����ֹ��CA
						return 0;//������̷����������
					}
					Send_Byte(CRC16);//����'C',��������
					break;
			}
			if (file_done != 0)//�ļ�������ϣ��˳�ѭ��
			{
				break;
			}
			OSTimeDlyHMSM(0,0,0,50);  //��ʱ50ms
		}
		if (session_done != 0)//�Ի�����������ѭ��
		{
			break;
		}
	}
	return (int32_t)size;//���ؽ��յ��ļ��Ĵ�С
}

/*******************************************************************************
  * @��������	Ymodem_CheckResponse
  * @����˵��   ͨ�� ymodemЭ������Ӧ
  * @�������   c
  * @�������   ��
  * @���ز���   0
*******************************************************************************/
int32_t Ymodem_CheckResponse(uint8_t c)
{
    return 0;
}

/*******************************************************************************
  * @��������	Ymodem_PrepareIntialPacket
  * @����˵��   ׼����һ�����ݰ�
  * @�������   data�����ݰ�
                fileName ���ļ���
                length ������
  * @�������   ��
  * @���ز���   ��
*******************************************************************************/
void Ymodem_PrepareIntialPacket(uint8_t *data, const uint8_t* fileName, uint32_t *length)
{
    uint16_t i, j;
    uint8_t file_ptr[10];

    //����ͷ3�����ݰ�
    data[0] = SOH;
    data[1] = 0x00;
    data[2] = 0xff;
    //�ļ������ݰ���Ч����
    for (i = 0; (fileName[i] != '\0') && (i < FILE_NAME_LENGTH); i++)
    {
        data[i + PACKET_HEADER] = fileName[i];
    }

    data[i + PACKET_HEADER] = 0x00;

    Int2Str (file_ptr, *length);
    for (j =0, i = i + PACKET_HEADER + 1; file_ptr[j] != '\0' ; )
    {
        data[i++] = file_ptr[j++];
    }

    for (j = i; j < PACKET_SIZE + PACKET_HEADER; j++)
    {
        data[j] = 0;
    }
}

/*******************************************************************************
  * @��������	Ymodem_PreparePacket
  * @����˵��   ׼�����ݰ�
  * @�������   SourceBuf������Դ����
                data�����ݰ�
                pktNo �����ݰ����
                sizeBlk ������
  * @�������   ��
  * @���ز���   ��
*******************************************************************************/
void Ymodem_PreparePacket(uint8_t *SourceBuf, uint8_t *data, uint8_t pktNo, uint32_t sizeBlk)
{
    uint16_t i, size, packetSize;
    uint8_t* file_ptr;

    //����ͷ3�����ݰ�
    packetSize = sizeBlk >= PACKET_1K_SIZE ? PACKET_1K_SIZE : PACKET_SIZE;
    size = sizeBlk < packetSize ? sizeBlk :packetSize;
    if (packetSize == PACKET_1K_SIZE)
    {
        data[0] = STX;
    }
    else
    {
        data[0] = SOH;
    }
    data[1] = pktNo;
    data[2] = (~pktNo);
    file_ptr = SourceBuf;

    //�ļ������ݰ���Ч����
    for (i = PACKET_HEADER; i < size + PACKET_HEADER; i++)
    {
        data[i] = *file_ptr++;
    }
    if ( size  <= packetSize)
    {
        for (i = size + PACKET_HEADER; i < packetSize + PACKET_HEADER; i++)
        {
            data[i] = 0x1A; //����
        }
    }
}

/*******************************************************************************
  * @��������	UpdateCRC16
  * @����˵��   �����������ݵģãң�У��
  * @�������   crcIn
                byte
  * @�������   ��
  * @���ز���   �ãң�У��ֵ
*******************************************************************************/
uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte)
{
    uint32_t crc = crcIn;
    uint32_t in = byte|0x100;
    do
    {
        crc <<= 1;
        in <<= 1;
        if (in&0x100)
            ++crc;
        if (crc&0x10000)
            crc ^= 0x1021;
    }
    while (!(in&0x10000));
    return crc&0xffffu;
}

/*******************************************************************************
  * @��������	UpdateCRC16
  * @����˵��   �����������ݵģãң�У��
  * @�������   data ������
                size ������
  * @�������   ��
  * @���ز���   �ãң�У��ֵ
*******************************************************************************/
uint16_t Cal_CRC16(const uint8_t* data, uint32_t size)
{
    uint32_t crc = 0;
    const uint8_t* dataEnd = data+size;
    while (data<dataEnd)
        crc = UpdateCRC16(crc,*data++);

    crc = UpdateCRC16(crc,0);
    crc = UpdateCRC16(crc,0);
    return crc&0xffffu;
}


/*******************************************************************************
  * @��������	CalChecksum
  * @����˵��   ����YModem���ݰ����ܴ�С
  * @�������   data ������
                size ������
  * @�������   ��
  * @���ز���   ���ݰ����ܴ�С
*******************************************************************************/
uint8_t CalChecksum(const uint8_t* data, uint32_t size)
{
    uint32_t sum = 0;
    const uint8_t* dataEnd = data+size;
    while (data < dataEnd )
        sum += *data++;
    return sum&0xffu;
}

/*******************************************************************************
  * @��������	Ymodem_SendPacket
  * @����˵��   ͨ��ymodemЭ�鴫��һ�����ݰ�
  * @�������   data �����ݵ�ַָ��
                length������
  * @�������   ��
  * @���ز���   ��
*******************************************************************************/
void Ymodem_SendPacket(uint8_t *data, uint16_t length)
{
    uint16_t i;
    i = 0;
    while (i < length)
    {
        Send_Byte(data[i]);
        i++;
    }
}

/*******************************************************************************
  * @��������	Ymodem_Transmit
  * @����˵��   ͨ��ymodemЭ�鴫��һ���ļ�
  * @�������   buf �����ݵ�ַָ��
                sendFileName ���ļ���
                sizeFile���ļ�����
  * @�������   ��
  * @���ز���   �Ƿ�ɹ�
                0���ɹ�
*******************************************************************************/
uint8_t Ymodem_Transmit (uint8_t *buf, const uint8_t* sendFileName, uint32_t sizeFile)
{

    uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
    uint8_t FileName[FILE_NAME_LENGTH];
    uint8_t *buf_ptr, tempCheckSum ;
    uint16_t tempCRC, blkNumber;
    uint8_t receivedC[2], CRC16_F = 0, i;
    uint32_t errors, ackReceived, size = 0, pktSize;

    errors = 0;
    ackReceived = 0;
    for (i = 0; i < (FILE_NAME_LENGTH - 1); i++)
    {
        FileName[i] = sendFileName[i];
    }
    CRC16_F = 1;

    //׼����һ�����ݰ�
    Ymodem_PrepareIntialPacket(&packet_data[0], FileName, &sizeFile);

    do
    {
        //�������ݰ�
        Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);
        //����CRCУ��
        if (CRC16_F)
        {
            tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
            Send_Byte(tempCRC >> 8);
            Send_Byte(tempCRC & 0xFF);
        }
        else
        {
            tempCheckSum = CalChecksum (&packet_data[3], PACKET_SIZE);
            Send_Byte(tempCheckSum);
        }

        //�ȴ���Ӧ
        if (Receive_Byte(&receivedC[0], 10000) == 0)
        {
            if (receivedC[0] == ACK)
            {
                //���ݰ���ȷ����
                ackReceived = 1;
            }
        }
        else
        {
            errors++;
        }
    } while (!ackReceived && (errors < 0x0A));

    if (errors >=  0x0A)
    {
        return errors;
    }
    buf_ptr = buf;
    size = sizeFile;
    blkNumber = 0x01;

    //1024�ֽڵ����ݰ�����
    while (size)
    {
        //׼����һ�����ݰ�
        Ymodem_PreparePacket(buf_ptr, &packet_data[0], blkNumber, size);
        ackReceived = 0;
        receivedC[0]= 0;
        errors = 0;
        do
        {
            //������һ�����ݰ�
            if (size >= PACKET_1K_SIZE)
            {
                pktSize = PACKET_1K_SIZE;

            }
            else
            {
                pktSize = PACKET_SIZE;
            }
            Ymodem_SendPacket(packet_data, pktSize + PACKET_HEADER);
            //����CRCУ��
            if (CRC16_F)
            {
                tempCRC = Cal_CRC16(&packet_data[3], pktSize);
                Send_Byte(tempCRC >> 8);
                Send_Byte(tempCRC & 0xFF);
            }
            else
            {
                tempCheckSum = CalChecksum (&packet_data[3], pktSize);
                Send_Byte(tempCheckSum);
            }

            //�ȴ���Ӧ
            if ((Receive_Byte(&receivedC[0], 100000) == 0)  && (receivedC[0] == ACK))
            {
                ackReceived = 1;
                if (size > pktSize)
                {
                    buf_ptr += pktSize;
                    size -= pktSize;
                    if (blkNumber == (FLASH_IMAGE_SIZE/1024))
                    {
                        return 0xFF; //����
                    }
                    else
                    {
                        blkNumber++;
                    }
                }
                else
                {
                    buf_ptr += pktSize;
                    size = 0;
                }
            }
            else
            {
                errors++;
            }
        } while (!ackReceived && (errors < 0x0A));
        //���û��Ӧ10�ξͷ��ش���
        if (errors >=  0x0A)
        {
            return errors;
        }

    }
    ackReceived = 0;
    receivedC[0] = 0x00;
    errors = 0;
    do
    {
        Send_Byte(EOT);
        //���� (EOT);
        //�ȴ���Ӧ
        if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
        {
            ackReceived = 1;
        }
        else
        {
            errors++;
        }
    } while (!ackReceived && (errors < 0x0A));

    if (errors >=  0x0A)
    {
        return errors;
    }
    //׼�����һ����
    ackReceived = 0;
    receivedC[0] = 0x00;
    errors = 0;

    packet_data[0] = SOH;
    packet_data[1] = 0;
    packet_data [2] = 0xFF;

    for (i = PACKET_HEADER; i < (PACKET_SIZE + PACKET_HEADER); i++)
    {
        packet_data [i] = 0x00;
    }

    do
    {
        //�������ݰ�
        Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);
        //����CRCУ��
        tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
        Send_Byte(tempCRC >> 8);
        Send_Byte(tempCRC & 0xFF);

        //�ȴ���Ӧ
        if (Receive_Byte(&receivedC[0], 10000) == 0)
        {
            if (receivedC[0] == ACK)
            {
                //��������ȷ
                ackReceived = 1;
            }
        }
        else
        {
            errors++;
        }

    } while (!ackReceived && (errors < 0x0A));
    //���û��Ӧ10�ξͷ��ش���
    if (errors >=  0x0A)
    {
        return errors;
    }

    do
    {
        Send_Byte(EOT);
        //���� (EOT);
        //�ȴ���Ӧ
        if ((Receive_Byte(&receivedC[0], 10000) == 0)  && receivedC[0] == ACK)
        {
            ackReceived = 1;
        }
        else
        {
            errors++;
        }
    } while (!ackReceived && (errors < 0x0A));

    if (errors >=  0x0A)
    {
        return errors;
    }
    return 0;//�ļ�����ɹ�
}

/*******************************�ļ�����***************************************/
