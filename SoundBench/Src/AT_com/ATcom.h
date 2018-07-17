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

#define	AT_MIN_TIMEOUT 	     10

#define AT_DATA_PIPE         0
#define AT_COMMAND_PIPE      1
#define AT_INCOMING_PIPE     2

#define AT_PIPES_MAXCOUNT    3

typedef enum {
    AT_OK = 0,
    AT_ERROR_RX,
    AT_ERROR_TX,
    AT_ERROR_INCOMING_OVERFLOW,
    AT_ERROR_ECHO,
    AT_ERROR_FORMAT,
    AT_REBOOT,
    AT_TIMEOUT
} AT_result_t;

typedef enum {
    AS_NOT_INIT = 0,
    AS_POWER_OFF,
    AS_READY, //(waiting AT command)
    AS_SENDING,
    AS_WAITING_ECHO,
    AS_WAITING_ANSWER,
    AS_SENDING_DATA,
    AS_RECEIVING_DATA,
    AS_ERROR
} AT_mainState_t;

typedef struct {
    uint32_t start;
    uint32_t end;
    uint32_t ptr;
    int      len;
    uint32_t n;
} AT_pipe_t;

typedef struct
{
	UART_HandleTypeDef  *huart;
    AT_mainState_t      state;
    AT_result_t         lastResult;
    osMutexId           busyMutex;
	uint8_t             *rxBuf;
    int                 rxSize;
    AT_pipe_t           rxPipe[AT_PIPES_MAXCOUNT];
    uint8_t             *txBuf;
    int                 txLen;
    bool                useEcho;  
    void                (*errorLoggingCallback)(char *errorMessage, int errorArgument);
    void                (*errorProcessingCallback)(void);
} ATcom_t;

bool        AT_Init(ATcom_t *com);   
bool        AT_Start(ATcom_t *com);   // must added at start of programm and in HAL_UART_ErrorCallback  !
bool        AT_Lock(ATcom_t *com, uint32_t timeout);
bool        AT_Unlock(ATcom_t *com);
void        AT_Process(ATcom_t *com);
uint32_t    AT_GetString(ATcom_t *com, char *str, uint32_t strMaxLen, uint32_t timeout);
void        AT_RxUartDmaISR(ATcom_t *com);  // must added to HAL_UART_RxCpltCallback  and  HAL_UART_RxHalfCpltCallback 
bool        AT_SendRaw(ATcom_t *com, uint8_t *data, uint32_t len);
bool        AT_SendString(ATcom_t *com, char *data);
uint32_t    AT_Command(ATcom_t *com, char *command, uint32_t timeout, uint32_t countOfAnswersVariants, ...);
uint32_t    AT_GetIncomingCommands(ATcom_t *com, uint32_t timeout, uint32_t countOfAnswersVariants, ...);
uint32_t  AT_GetData(ATcom_t *com, uint8_t *buf, uint32_t bufSize, uint32_t timeout);
#endif


