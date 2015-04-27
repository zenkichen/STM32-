#ifndef _SPI_FLASH_H_
#define _SPI_FLASH_H_  1
//#include "stm32f10x_lib.h"

#define FLASH_CHREAD 0x0B
#define FLASH_CLREAD 0x03
#define FLASH_PREAD	0xD2

#define FLASH_BUFWRITE1 0x84
#define FLASH_IDREAD 0x9F
#define FLASH_STATUS 0xD7
#define PAGE_ERASE 0x81
#define PAGE_READ 0xD2
#define MM_PAGE_TO_B1_XFER 0x53				// �����洢����ָ��ҳ���ݼ��ص���һ������
#define BUFFER_2_WRITE 0x87					// д��ڶ�������
#define B2_TO_MM_PAGE_PROG_WITH_ERASE 0x86	// ���ڶ�������������д�����洢��������ģʽ��

#define SST25_SECTOR_SIZE 4096
#define SST25_BLOCK_SIZE 512 

#define Dummy_Byte 0xA5

/* Select SPI FLASH: ChipSelect pin low  */
#define Select_Flash()     GPIO_ResetBits(GPIOA, GPIO_Pin_4)
/* Deselect SPI FLASH: ChipSelect pin high */
#define NotSelect_Flash()    GPIO_SetBits(GPIOA, GPIO_Pin_4)



void SPI_Flash_Init(void);	         //SPI��ʼ��
u8 SPI_Flash_ReadByte(void);		//flash����������������һ���ֽ�
u8 SPI_Flash_SendByte(u8 byte);		//	FLASH������������������һ���ֽ�
void FlashPageEarse(u16 page);	//����ָ����ҳ,ҳ��Χ0-4095

void FlashPageRead(u16 page,u8 *Data);		//��ȡ��ҳ��ҳ��Χ0-4095
void FlashPageWrite(u16 page,u8 *Data);		//дһ��ҳ��ҳ��Χ0-4095


void FlashWaitBusy(void);			    //Flashæ���
void FlashReadID(void);		//��ȡflashID�ĸ��ֽ�
	
void SST25_R_BLOCK(unsigned long addr, unsigned char *readbuff, unsigned int BlockSize);
void SST25_W_BLOCK(uint32_t addr, u8 *readbuff, uint16_t BlockSize);





#endif

