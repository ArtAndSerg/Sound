// file: ATcom.h

#ifndef _AT_COM_H
#define _AT_COM_H

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include "cmsis_os.h"

#define WAIT_PARAM_MAX_COUNT 10
#define	ATCOM_MIN_TIMEOUT 	 10

typedef enum {
    AT_OK = 0,
    AT_ERROR_SEND,
    AT_ERROR_ECHO,
    AT_ERROR_FORMAT,
    AT_REBOOT,
    AT_TIMEOUT
} AT_result_t;

typedef struct
{
	UART_HandleTypeDef  *huart;
    osSemaphoreId       txSemaphore;
	uint8_t             *rxBuf;
    uint32_t            rxSize;
    uint32_t            rxPtr;
    int                 rxLen;
	uint8_t             *txBuf;
    uint32_t            txSize;
    void (*errorProcessingCallback)(char *errorMessage, int errorCode);
} ATcom_t;

bool AT_Start(ATcom_t *com);   // must added at start of programm and in HAL_UART_ErrorCallback  !
int AT_GetData(ATcom_t *com, uint8_t *buf, int bufSize, uint32_t timeout);
bool AT_GetByte(ATcom_t *com, uint8_t *byte);
uint32_t AT_GetString(ATcom_t *com, char *str, uint32_t strMaxLen, uint32_t timeout);
void AT_RxUartDmaISR(ATcom_t *com);  // must added to HAL_UART_RxCpltCallback  and  HAL_UART_RxHalfCpltCallback 
void AT_txCompleteISR (ATcom_t *com);
void AT_waitTxComplete(ATcom_t *com);
bool AT_SendRaw(ATcom_t *com, uint8_t *data, uint16_t len);
bool AT_SendString(ATcom_t *com, char *data);
AT_result_t AT_Command(ATcom_t *com, int *result, char *command, uint32_t timeout, uint32_t countOfParameters, ...);

#endif