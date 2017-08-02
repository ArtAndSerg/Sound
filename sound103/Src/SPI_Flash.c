// SPI_Flash.c

#include <stdlib.h>
#include "stm32f1xx_hal.h"
#include "main.h"
#include "SPI_Flash.h"

extern SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef *hspi = &hspi1;

DWORD dwCurrentAddr;
static  void _SendCmd(BYTE cmd);

void SPIFlashEndRead(void) {FLASH_CS_HI();}
void SPIFlashRead(BYTE *buf, int count) {HAL_SPI_Receive(hspi, buf, count, HAL_MAX_DELAY);}

void SPIFlashInit(void) 
{
    BYTE buf[2] = {WRSR, 0x00};
    // FLASH_WP_HI();
    _SendCmd(WRDI);

    // Execute Enable-Write-Status-Register (EWSR) instruction
    _SendCmd(EWSR);

    // Clear Write-Protect on all memory locations
    FLASH_CS_LOW();
    HAL_SPI_Transmit(hspi, buf, 2, HAL_MAX_DELAY); // Clear all block protect bits
    FLASH_CS_HI();
    dwCurrentAddr = SPI_FLASH_SIZE;
}
//---------------------------------------------------------------------------

void SPIFlashBeginWrite(DWORD dwAddr)
{
   BYTE cmd[4] = {WRITE, ((BYTE*)&dwAddr)[2], ((BYTE*)&dwAddr)[1], ((BYTE*)&dwAddr)[0]}; 
   dwCurrentAddr = dwAddr;
   if (dwCurrentAddr < SPI_FLASH_SIZE) {
       if (!(dwCurrentAddr & SPI_FLASH_SECTOR_MASK)) {
                SPIFlashEraseSector(dwCurrentAddr);
           }
           _SendCmd(WREN);  // Enable writing
           FLASH_CS_LOW();  // Activate the chip select         
           HAL_SPI_Transmit(hspi, cmd, 4, HAL_MAX_DELAY); // Issue WRITE command with address
   }
}
//--------------------------------------------------------------------

void SPIFlashWrite(BYTE *buf, int count)
{
    int remain, written = 0, len;
    
    while (written < count && dwCurrentAddr < SPI_FLASH_SIZE && buf != NULL) {        
        if(!(dwCurrentAddr & SPI_FLASH_PAGE_MASK)) {
            // If address is a sector boundary, erase a sector first
            SPIFlashEndWrite();
            SPIFlashBeginWrite(dwCurrentAddr);
        }
        remain = SPI_FLASH_PAGE_MASK - (dwCurrentAddr & SPI_FLASH_PAGE_MASK)+1;
        if (remain > (count - written)) {
            len = count - written;
        } else {
            len = remain;
        }
        HAL_SPI_Transmit(hspi, &buf[written], len, HAL_MAX_DELAY); 
        written += len;
        dwCurrentAddr += len;
    }
}
//--------------------------------------------------------------------

void SPIFlashEndWrite(void)
{
    BYTE tmp = RDSR;
    DWORD d = 700;
    FLASH_CS_HI();
    // Activate chip select 400 ns- BAD      100 us - GOOD???
    while(d--) __NOP();
    //delay_ms(1);
    FLASH_CS_LOW();
    
    // Send Read Status Register instruction
    HAL_SPI_Transmit(hspi, &tmp, 1, HAL_MAX_DELAY); 
    
    // Poll the BUSY bit
    d = 0;
    do
    {
      d++;
      HAL_SPI_Receive(hspi, &tmp, 1, HAL_MAX_DELAY); 
      if (tmp)
      {
        osDelay(0);
      }
    } while(tmp & BUSY);
    if (d > 1)
    {
       osDelay(0); 
    }
    // Deactivate chip select
    FLASH_CS_HI();   
}
//--------------------------------------------------------------------

int SPIFlashWriteArray(DWORD Addr, BYTE *vData, WORD wLen)
{
   // Ignore operations when the destination is NULL or nothing to read
    if(vData != NULL && wLen && Addr < SPI_FLASH_SIZE) {
        SPIFlashBeginWrite(Addr);
        SPIFlashWrite(vData, wLen);
        SPIFlashEndWrite();
        return wLen;
    }
    else {
        return 0;
    }
}
//--------------------------------------------------------------------

void SPIFlashBeginRead(DWORD dwAddr)
{
    BYTE buf[4] = {READ, ((BYTE*)&dwAddr)[2], ((BYTE*)&dwAddr)[1], ((BYTE*)&dwAddr)[0]};
     
    dwCurrentAddr = dwAddr;
    if(dwAddr < SPI_FLASH_SIZE) {
        // Activate chip select
        FLASH_CS_LOW();   
        // Send READ opcode
        HAL_SPI_Transmit(hspi, buf, 4, HAL_MAX_DELAY); 
    }
}
//--------------------------------------------------------------------

void SPIFlashReadArray(DWORD dwAddr, BYTE *vData, WORD wLen)
{
    // Ignore operations when the destination is NULL or nothing to read
    if(vData != NULL && wLen && dwAddr < SPI_FLASH_SIZE) {
        SPIFlashBeginRead(dwAddr);
        // Read data
        HAL_SPI_Receive(hspi, vData, wLen, HAL_MAX_DELAY); 
        SPIFlashEndRead();
    }
}
//----------------------------------------------------------------------------

unsigned char SPIFlashReadByte(void)
{
    unsigned char result;;
    HAL_SPI_Receive(hspi, &result, 1, HAL_MAX_DELAY);
    return result;
}
//----------------------------------------------------------------------------

void SPIFlashEraseSector(DWORD dwAddr)
{
    BYTE buf[4] = {ERASE_4K, ((BYTE*)&dwAddr)[2], ((BYTE*)&dwAddr)[1], ((BYTE*)&dwAddr)[0]};
    // Enable writing
    _SendCmd(WREN);

    // Activate the chip select
     FLASH_CS_LOW();
  
    // Issue ERASE command with address
    HAL_SPI_Transmit(hspi, buf, 4, HAL_MAX_DELAY); 
    SPIFlashEndWrite();
}
//---------------------------------------------------------------------------

void SPIFlashEraseAllChip(void)
{
  BYTE buf = ERASE_ALL;
   // Enable writing
  _SendCmd(WREN);
  // Activate the chip select
  FLASH_CS_LOW();
  // Issue WRITE command with address
  HAL_SPI_Transmit(hspi, &buf, 1, HAL_MAX_DELAY); 
  SPIFlashEndWrite();
}
//----------------------------------------------------------------------------

static void _SendCmd(BYTE cmd)
{
  FLASH_CS_LOW();
  HAL_SPI_Transmit(hspi, &cmd, 1, HAL_MAX_DELAY);
  FLASH_CS_HI();
}
//---------------------------------------------------------------------------

/*
static  BYTE _ReadByte(void)
{
  //unsigned char i = 8, buf = 0x00;
  SPI_SendData8(SPI2, 0x00);
  while(SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
  return SPI_ReceiveData8(SPI2);
}
//---------------------------------------------------------------------------
*/
 /*  
static  BYTE _WriteReadByte(BYTE val)
{
    unsigned char buf;
    HAL_SPI_TransmitReceive(hspi, &val, &buf, 1, HAL_MAX_DELAY);
    return buf;    
  
  while(i--)
  {
    FLASH_SCK_LOW();
    if ((val >> i) & 0x01) FLASH_MOSI_HI();
    else FLASH_MOSI_LOW();
    FLASH_SCK_HI();
  }
  
  
}
//---------------------------------------------------------------------------
*/