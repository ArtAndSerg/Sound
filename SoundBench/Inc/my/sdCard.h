// file sdCard.h

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SD_CARD_H__
#define __SD_CARD_H__

#include <stdint.h>

#define SD_LOW_SPEED_PRESCALER  SPI_BAUDRATEPRESCALER_256
#define SD_HIGH_SPEED_PRESCALER SPI_BAUDRATEPRESCALER_32

#define SD_BLOCK_SIZE 512
typedef uint32_t sd_addr_t;

typedef enum {
  SD_CARD_MMC,
  SD_CARD_SD,
  SD_CARD_SDHC
} sd_card_type_t;

extern   sd_card_type_t    sd_card_type;
uint8_t  sd_init(void);  // 0 if card ok
uint8_t  sd_block_read(sd_addr_t block_addr, uint8_t* buf);
uint8_t  sd_block_write(sd_addr_t block_addr, uint8_t* data);
uint32_t sd_get_size(void);

#endif
