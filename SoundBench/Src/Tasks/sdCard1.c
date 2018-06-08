// file sdCard.c
#include "main.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "my/sdCard.h"

#define SS_SD_SELECT()   HAL_GPIO_WritePin(SD_SS_GPIO_Port, SD_SS_Pin, GPIO_PIN_RESET)
#define SS_SD_DESELECT() HAL_GPIO_WritePin(SD_SS_GPIO_Port, SD_SS_Pin, GPIO_PIN_SET)
#define SPI_SendByte(a)      SPIx_WriteRead(a)
#define SPI_ReceiveByte()    SPIx_WriteRead(0xFF)
#define SPI_Release()        SPIx_WriteRead(0xFF)

static uint8_t SD_cmd (uint8_t cmd, uint32_t arg);
static uint8_t SPIx_WriteRead(uint8_t Byte);
extern SPI_HandleTypeDef hspi2;

sd_info_ptr sdinfo;

void SD_PowerOn(void)
{
    osDelay(100);
}
//-----------------------------------------------------------------------------

static uint8_t SD_cmd (uint8_t cmd, uint32_t arg)
{
    uint8_t n, res;
    // ACMD<n> is the command sequense of CMD55-CMD<n>
    if (cmd & 0x80) {
        cmd &= 0x7F;
        res = SD_cmd(CMD55, 0);
        if (res > 1) {
            return res;
        }
    }
    // Select the card
    SS_SD_DESELECT();
    SPI_ReceiveByte();
    SS_SD_SELECT();
    SPI_ReceiveByte();
    SPI_ReceiveByte();
    // Send a command packet
    SPI_SendByte(cmd); // Start + Command index
    SPI_SendByte((uint8_t)(arg >> 24)); // Argument[31..24]
    SPI_SendByte((uint8_t)(arg >> 16)); // Argument[23..16]
    SPI_SendByte((uint8_t)(arg >> 8)); // Argument[15..8]
    SPI_SendByte((uint8_t)arg); // Argument[7..0]
    SPI_SendByte((uint8_t)arg); // Argument[7..0]
    n = 0x01; // Dummy CRC + Stop
    if (cmd == CMD0) {
        n = 0x95;
    } // Valid CRC for CMD0(0)
    if (cmd == CMD8) {
        n = 0x87;
    } // Valid CRC for CMD8(0x1AA)
    SPI_SendByte(n);
    // Receive a command response
    n = 10; // Wait for a valid response in timeout of 10 attempts
    do {
        res = SPI_ReceiveByte();
    } while ((res & 0x80) && --n);
    return res;
}
//-----------------------------------------------------------------------------

uint8_t SD_Read_Block (uint8_t *buff, uint32_t lba)
{
  uint8_t result;
  uint16_t cnt;
  result = SD_cmd (CMD17, lba); //CMD17 datasheet pages 50 and 96
  if (result != 0x00) {
      return 5; 
  }
  SPI_Release();
  cnt=0;
  do{ 
    result = SPI_ReceiveByte();
    cnt++;
  } while ( (result != 0xFE)&&(cnt < 0xFFFF) );
  if (cnt >= 0xFFFF) {
      return 5;
  }
  for (int i = 0; i < 512; i++) {
      buff[i] = SPI_ReceiveByte(); 
  }
  SPI_Release(); // skip crc
  SPI_Release();
  return 0;
}
//-----------------------------------------------

uint8_t SD_Init(void)
{
    
  uint8_t ocr[10] = {0x40, 0x00, 0x00, 0x00, 0x00, 0x95}, cmd;
  int16_t tmr;
  uint32_t temp;
  sdinfo.type = 0;
  
  while(1) {
      memset(ocr, 0, 7); 
      ocr[0] = 0x40;
      ocr[5] = 0x95;
  
  
  //temp = hspi2.Init.BaudRatePrescaler;
  //hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_128; //156.25 kbbs
  //HAL_SPI_Init(&hspi2);
  SS_SD_DESELECT();
  for( int i=0; i<10; i++) {
      SPI_Release();
  }  
  //hspi2.Init.BaudRatePrescaler = temp;
  //HAL_SPI_Init(&hspi2);
  SS_SD_SELECT();
  osDelay(1);
  HAL_SPI_Transmit(&hspi2, ocr, 6, 1000);
  osDelay(1);
  HAL_SPI_Receive(&hspi2, ocr, 6, 1000);
  osDelay(500);
    }
  
  
  if (SD_cmd(CMD0, 0) == 1) { // Enter Idle state 
      SPI_Release();
      if (SD_cmd(CMD8, 0x1AA) == 1) { // SDv2
          for (int i = 0; i < 4; i++) {
              ocr[i] = SPI_ReceiveByte();
          }
          printf("OCR: 0x%02X 0x%02X 0x%02X 0x%02Xrn",ocr[0],ocr[1],ocr[2],ocr[3]);
          // Get trailing return value of R7 resp
          if (ocr[2] == 0x01 && ocr[3] == 0xAA) {// The card can work at vdd range of 2.7-3.6V
              for (tmr = 12000; tmr && SD_cmd(ACMD41, 1UL << 30); tmr--); // Wait for leaving idle state (ACMD41 with HCS bit)
              if (tmr && SD_cmd(CMD58, 0) == 0) { // Check CCS bit in the OCR
                  for (int i = 0; i < 4; i++) {
                      ocr[i] = SPI_ReceiveByte();
                  }
              }
              printf("OCR: 0x%02X 0x%02X 0x%02X 0x%02Xrn",ocr[0],ocr[1],ocr[2],ocr[3]);
              sdinfo.type = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2; // SDv2 (HC or SC)
          }
      } else {//SDv1 or MMCv3
          if (SD_cmd(ACMD41, 0) <= 1) {
              sdinfo.type = CT_SD1; cmd = ACMD41; // SDv1
          } else {
              sdinfo.type = CT_MMC; cmd = CMD1; // MMCv3
          }
          for (tmr = 25000; tmr && SD_cmd(cmd, 0); tmr--); // Wait for leaving idle state
          if (!tmr || SD_cmd(CMD16, 512) != 0) { // Set R/W block length to 512
              sdinfo.type = 0;
          }          
      }
  } else {
      return 1;
  }
  printf("Type SD: 0x%02Xrn", sdinfo.type);
  return 0;
}  
//-----------------------------------------------

static uint8_t SPIx_WriteRead(uint8_t Byte)

{
  uint8_t receivedbyte = 0;

  if(HAL_SPI_TransmitReceive(&hspi2, (uint8_t*) &Byte, (uint8_t*) &receivedbyte, 1, 500) != HAL_OK) {
      osDelay(1);
  }
  return receivedbyte;
}
//-----------------------------------------------

 
 