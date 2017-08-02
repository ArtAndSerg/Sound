 // SPI_Flash.h

#ifndef _SPI_FLASH_H
#define _SPI_FLASH_H

//#define osDelay(x)   HAL_Delay(x)
#define FLASH_CS_Pin GPIO_PIN_4
#define FLASH_CS_GPIO_Port GPIOA



#define SPI_FLASH_SECTOR_SIZE	(4096ul)
#define SPI_FLASH_PAGE_SIZE	(256ul)		
#define SPI_FLASH_SIZE		(8ul*1024ul*1024ul) // WINBOND 8 Mbytes

#define SPI_FLASH_SECTOR_MASK  (SPI_FLASH_SECTOR_SIZE - 1)
#define SPI_FLASH_PAGE_MASK    (SPI_FLASH_PAGE_SIZE - 1)

#define READ            0x03    // SPI Flash opcode: Read up up to 25MHz
#define READ_FAST       0x0B    // SPI Flash opcode: Read up to 50MHz with 1 dummy byte
#define ERASE_4K        0x20    // SPI Flash opcode: 4KByte sector erase
#define ERASE_32K       0x52    // SPI Flash opcode: 32KByte block erase
#define ERASE_SECTOR    0xD8    // SPI Flash opcode: sector block erase
#define ERASE_ALL       0x60    // SPI Flash opcode: Entire chip erase
#define WRITE           0x02    // SPI Flash opcode: Write one byte
//#define WRITE_STREAM    0xAD    // SPI Flash opcode: Write continuous stream of words (AAI mode)
#define RDSR            0x05    // SPI Flash opcode: Read Status Register
#define EWSR            0x50    // SPI Flash opcode: Enable Write Status Register
#define WRSR            0x01    // SPI Flash opcode: Write Status Register
#define WREN            0x06    // SPI Flash opcode: Write Enable
#define WRDI            0x04    // SPI Flash opcode: Write Disable / End AAI mode
#define RDID            0x90    // SPI Flash opcode: Read ID
#define JEDEC_ID        0x9F    // SPI Flash opcode: Read JEDEC ID
#define EBSY            0x70    // SPI Flash opcode: Enable write BUSY status on SO pin
#define DBSY            0x80    // SPI Flash opcode: Disable write BUSY status on SO pin

#define BUSY    0x01    // Mask for Status Register BUSY bit
#define WEL     0x02    // Mask for Status Register BUSY bit
#define BP0     0x04    // Mask for Status Register BUSY bit
#define BP1     0x08    // Mask for Status Register BUSY bit
#define BP2     0x10    // Mask for Status Register BUSY bit
#define BP3     0x20    // Mask for Status Register BUSY bit
#define AAI     0x40    // Mask for Status Register BUSY bit
#define BPL     0x80    // Mask for Status Register BUSY bit

//Winbond pins
#define FLASH_CS_LOW()    HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_RESET)
#define FLASH_CS_HI()     HAL_GPIO_WritePin(FLASH_CS_GPIO_Port, FLASH_CS_Pin, GPIO_PIN_SET) 

typedef unsigned char  BYTE;
typedef unsigned short WORD;
typedef unsigned long  DWORD;


// FLASH
void SPIFlashInit(void);		
void SPIFlashBeginRead(DWORD dwAddr);
//#define SPIFlashRead()          _ReadByte()
//#define SPIFlashEndRead()       FLASH_CS_HI()
BYTE _WriteReadByte(BYTE val);
void SPIFlashRead(BYTE *buf, int count);
void SPIFlashEndRead(void);   
unsigned char SPIFlashReadByte(void);
void SPIFlashReadArray(DWORD dwAddress, BYTE *vData, WORD wLen);
void SPIFlashBeginWrite(DWORD dwAddr);
void SPIFlashWrite(BYTE *buf, int count);
void SPIFlashEndWrite(void);
int  SPIFlashWriteArray(DWORD Addr, BYTE *vData, WORD wLen);
void SPIFlashEraseSector(DWORD dwAddr);
void SPIFlashEraseAllChip(void);


#endif

