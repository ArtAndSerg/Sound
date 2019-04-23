// file: "Modbus.h"

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MODBUS_H
#define __MODBUS_H

#define MODBUF_RX_BUF_SIZE  25  // mast be 1 byte more, than size of the biggest packet (for overflow detection)

#include "stdint.h"
#include "stdbool.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"

typedef enum {
    MB_OK = 0,
    MB_TIMEOUT,
    MB_TX_FINISH,
    MB_RX_DONE,
    MB_ERR_FRAME,
    MB_ERR_OVERFLOW,
    MB_ERR_CRC,
    MB_ERR_INIT,
    MB_ERR_OTHER
} modbusResult_t;       

typedef struct {
    UART_HandleTypeDef *huart;
    uint8_t addr;
    int rxLen;
    uint8_t rxBuf[MODBUF_RX_BUF_SIZE];
    osSemaphoreId  dataRxSemHandle;
    char debugLevel;
    modbusResult_t lastResult;
} modbus_t;

void modbusSetTxMode(bool txMode);          // Set RE/DE pin of MAX485 or similar IC, if needed.
void modbusBefore_HAL_UART_IRQHandler(modbus_t *mb);  // Must be BEFOR(!) "HAL_UART_IRQHandler(&huart1);" in file "stm32...xx_it.c"
void modbusAfter_HAL_UART_IRQHandler(modbus_t *mb);   // Must be AFTER(!) "HAL_UART_IRQHandler(&huart1);" in file "stm32...xx_it.c"

bool modbusInit(modbus_t *mb, uint8_t addr, uint32_t baudRate);
void modbusReceiveStart(modbus_t *mb);
modbusResult_t modbusDataWait(modbus_t *mb, uint32_t timeout);
void modbusSend(modbus_t *mb, uint8_t *buf, uint32_t len);
unsigned short modbusCrc16(unsigned char* data, unsigned char length);
#endif

