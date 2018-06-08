// file sdCard.c
#include "main.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "my/sdCard.h"

#define hspi   hspi2
extern SPI_HandleTypeDef hspi2;
sd_card_type_t sd_card_type;

static inline void spi_cs_set(void);
static inline void spi_cs_clear(void);
static inline uint8_t spi_query_byte(uint8_t Byte);

static uint32_t ext_bits(unsigned char *data, int msb, int lsb) {
    uint32_t bits = 0;
    uint32_t size = 1 + msb - lsb;
    for (uint32_t i = 0; i < size; i++) {
        uint32_t position = lsb + i;
        uint32_t byte = 15 - (position >> 3);
        uint32_t bit = position & 0x7;
        uint32_t value = (data[byte] >> bit) & 1;
        bits |= value << i;
    }
    return bits;
}
//-----------------------------------------------------------------------------

void spi_set_low_speed(void)
{
  hspi.Init.BaudRatePrescaler =  SD_LOW_SPEED_PRESCALER;
  HAL_SPI_Init(&hspi);
  __HAL_SPI_ENABLE(&hspi);
}
//-----------------------------------------------------------------------------

void spi_set_high_speed(void)
{
  hspi.Init.BaudRatePrescaler = SD_HIGH_SPEED_PRESCALER;
  HAL_SPI_Init(&hspi);
  __HAL_SPI_ENABLE(&hspi);
}
//-----------------------------------------------------------------------------

static inline void spi_cs_set(void)
{
  HAL_GPIO_WritePin(SD_SS_GPIO_Port, SD_SS_Pin, GPIO_PIN_SET);
}
//-----------------------------------------------------------------------------

static inline void spi_cs_clear(void)
{
  HAL_GPIO_WritePin(SD_SS_GPIO_Port, SD_SS_Pin, GPIO_PIN_RESET);
}
//-----------------------------------------------------------------------------

static inline uint8_t spi_query_byte(uint8_t Byte)
{
  uint8_t receivedbyte = 0;
 
  *(__IO uint8_t *)&hspi.Instance->DR = Byte;
  while(!(__HAL_SPI_GET_FLAG((&hspi), SPI_FLAG_RXNE)));
  receivedbyte = *(__IO uint8_t *)&hspi.Instance->DR;
  // if (HAL_SPI_TransmitReceive(&hspi, &Byte, (uint8_t*) &receivedbyte, 1, 100) != HAL_OK) {
  //    osDelay(1);
 // }
  return receivedbyte;
}
//-----------------------------------------------------------------------------

uint8_t spi_wait_resp(void)
{
  int cnt = 0xfff;

  uint8_t res = 0;

  while (cnt--) {
    if ((res = spi_query_byte(0xff)) != 0xff)
      return res;
  }
  return 0xff;
}
//-----------------------------------------------------------------------------

void sd_release(void)
{
  spi_cs_set();
  spi_query_byte(0xff);
}
//-----------------------------------------------------------------------------

uint8_t sd_get_r1(void)
{
  uint8_t ret = spi_wait_resp();

  sd_release();

  return ret;
}
//-----------------------------------------------------------------------------

uint8_t sd_get_r3(uint32_t* r3)
{
  uint32_t res = spi_wait_resp();
  if (res != 0x00)
    return res;

  res = spi_query_byte(0xff) << 24;
  res |= spi_query_byte(0xff) << 16;
  res |= spi_query_byte(0xff) << 8;
  res |= spi_query_byte(0xff);

  *r3 = res;

  sd_release();

  return 0x00;
}
//-----------------------------------------------------------------------------

uint8_t sd_get_r7(uint32_t* r7)
{
  uint32_t res = spi_wait_resp();
  if (res != 0x01)
    return res;

  res = spi_query_byte(0xff) << 24;
  res |= spi_query_byte(0xff) << 16;
  res |= spi_query_byte(0xff) << 8;
  res |= spi_query_byte(0xff);

  *r7 = res;

  sd_release();

  return 0x01;
}
//-----------------------------------------------------------------------------

void sd_cmd(uint8_t cmd, uint32_t data)
{
 // uint8_t buf[6];
  spi_cs_clear();
 /* 
  buf[0] = 0x40 | (cmd & 0x3f);
  buf[1] = (data >> 24) & 0xff;
  buf[2] = (data >> 16) & 0xff;
  buf[3] = (data >> 8) & 0xff;
  buf[4] = (data) & 0xff;
  if (cmd != 8) {
    buf[5] = 0x95;  // CRC
  } else {
    buf[5] = 0x87;
  }
  
  HAL_SPI_Transmit(&hspi, buf, sizeof(buf), 100);
  
  */
  spi_query_byte(0x40 | (cmd & 0x3f));
  spi_query_byte((data >> 24) & 0xff);
  spi_query_byte((data >> 16) & 0xff);
  spi_query_byte((data >> 8) & 0xff);
  spi_query_byte((data) & 0xff);
  if (cmd != 8)
    spi_query_byte(0x95);  // CRC
  else
    spi_query_byte(0x87);
}
//-----------------------------------------------------------------------------


uint32_t sd_get_size(void)
{
    uint32_t c_size, blocks, csd_structure, block_len, mult, capacity;
    static  uint8_t csd[50];
    sd_cmd(9, 0);
    while(spi_query_byte(0xFF) != 0xFE);  
    for (int i = 0; i < 50; i++) {
        csd[i] = spi_query_byte(0xFF);
    }
    csd_structure = ext_bits(csd, 127, 126); // csd_structure : csd[127:126]
    switch (csd_structure) {
        case 0:
            c_size    = ext_bits(csd, 73, 62);              // c_size        : csd[73:62]
            mult      = ext_bits(csd, 49, 47);         // c_size_mult   : csd[49:47]
            block_len = ext_bits(csd, 83, 80);         // read_bl_len   : csd[83:80] - the *maximum* read block length
            mult = 1 << (mult + 2);               // MULT = 2^C_SIZE_MULT+2 (C_SIZE_MULT < 8)
            block_len = 1 << block_len;                // BLOCK_LEN = 2^READ_BL_LEN
            capacity = (c_size + 1) * mult * block_len;              // memory capacity = BLOCKNR * BLOCK_LEN
            blocks = capacity / SD_BLOCK_SIZE;
            break;

        case 1:
            c_size = ext_bits(csd, 69, 48);            // device size : C_SIZE : [69:48]
            blocks = (c_size+1) << 10;                 // block count = C_SIZE+1) * 1K byte (512B is block size)
            break;

        default:
            printf("CSD struct unsupported!\n");
            return 0;
    };
    sd_release();
    return blocks;
}
//-----------------------------------------------------------------------------

uint8_t sd_init()
{
  spi_set_low_speed();
  
  /* Enable SPI peripheral */
  
  spi_cs_set();
  for (uint8_t i = 0; i < 10; ++i)
    spi_query_byte(0xFF);  // >74 clocks

  // reset
  sd_cmd(0, 0);
  if (sd_get_r1() != 0x01)
    return 1;

  // read power supply
  uint8_t r = 0;
  uint32_t r7 = 0;
  sd_cmd(8, 0x1aa);

  r = sd_get_r7(&r7);
  
  if (r == 0x05) {  // SDv1/MMC
    sd_card_type = SD_CARD_MMC;
  } else if (r == 0x01) {  // SDCv2
    if (r7 == 0x1aa)
      sd_card_type = SD_CARD_SD;
    else
      return 5;
  } else {
    return 5;
  }

  // init
  int cnt = 0xfff;
  int done = 0;
  while (cnt-- && (!done)) {
    if (sd_card_type == SD_CARD_SD) {
      sd_cmd(55, 0);
      if (sd_get_r1() != 0x01)
        return 4;
      sd_cmd(41, (1 << 30));  // SDHC bit
    } else {
      sd_cmd(1, 0);
    }
    if ((r = sd_get_r1()) == 0x00)
      done = 1;
  }
  if (!done)
    return 2;  // init timeout

  // read OCR - check for SDHC
  sd_cmd(58, 0);
  if (sd_get_r3(&r7) == 0x00) {
    if (r7 & (1 << 30))
      sd_card_type = SD_CARD_SDHC;
  }

  // set block length to 512
  sd_cmd(16, 0x0200);
  if (sd_get_r1() != 0x00)
    return 3;

  spi_set_high_speed();

  return 0;
}
//-----------------------------------------------------------------------------

uint8_t sd_block_read(sd_addr_t block_addr, uint8_t* buf)
{
  uint8_t dummy[2];
  
  if (sd_card_type != SD_CARD_SDHC)
    block_addr <<= 9;  // scale to bytes

  sd_cmd(17, block_addr);
  
  if (spi_wait_resp() != 0x00) {
    sd_release();
    return 1;
  }
  
  if (spi_wait_resp() != 0xfe) { // invalid data token received
    sd_release();
    return 2;
  }

  for (uint32_t i = 0; i < SD_BLOCK_SIZE; ++i)
    buf[i] = spi_query_byte(0xff);

  //HAL_SPI_Receive(&hspi, buf, SD_BLOCK_SIZE, 1000);
  //HAL_SPI_Receive(&hspi, dummy, 2, 10);
  // read csum
  spi_query_byte(0xff);
  spi_query_byte(0xff);

  sd_release();
/*
  printf("%08X: ", block_addr, buf);
        for (int i = 0; i < 512; i++) {
            printf("%02X ", buf[i]);
        }
        printf("\n\n");
  */
  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
  return 0;
}
//-----------------------------------------------------------------------------

uint8_t sd_block_write(sd_addr_t block_addr, uint8_t* data)
{
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
  if (sd_card_type != SD_CARD_SDHC)
    block_addr <<= 9;  // scale to bytes

  sd_cmd(24, block_addr);
  if (spi_wait_resp() != 0x00) {
    sd_release();
    return 1;
  }

  spi_query_byte(0xfe);  // data token

  for (uint32_t i = 0; i < SD_BLOCK_SIZE; ++i)
    spi_query_byte(data[i]);

//  HAL_SPI_Transmit (&hspi, data, SD_BLOCK_SIZE, 1000);  
  // write dummy csum
  spi_query_byte(0xff);
  spi_query_byte(0xff);

  if ((spi_query_byte(0xff) & 0x0f) != 0x05) {  // write failed
    sd_release();
    return 2;
  }

  int cnt = 0xffff;
  while (cnt--) {
    if (spi_query_byte(0xff) != 0)  // not busy
      break;
  }

  sd_release();
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
  return (cnt == 0 ? 3 : 0);
}
//-----------------------------------------------------------------------------
