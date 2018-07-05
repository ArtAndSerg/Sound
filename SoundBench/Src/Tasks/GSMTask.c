// file mainTask.c

#include "main.h"
#include "stm32f1xx_hal.h"
#include "my/myTasks.h"
#include "../src/AT_com/ATcom.h"

#define GSM_BUFSIZE_RX 1024
#define GSM_BUFSIZE_TX 1024

extern UART_HandleTypeDef huart3;
extern osSemaphoreId gsmTxSemHandle;
ATcom_t gsm;

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    AT_Start(&gsm);
}
//------------------------------------------------------------------------------

void HAL_UART_RxHalfCpltCallback(UART_HandleTypeDef *huart)
{
   AT_RxUartDmaISR(&gsm);
}
//------------------------------------------------------------------------------

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
   AT_RxUartDmaISR(&gsm);
}
//------------------------------------------------------------------------------

void gsmErrorProcessing(char *errorMessage, int errorCode)
{
     printf("ERROR!  %s - %d !\n", errorMessage, errorCode);
}
//------------------------------------------------------------------------------

void InitGsmTask(void)
{
    printf("\n\nGSM init... ");
    HAL_GPIO_WritePin(GPIOA, VS_RESET_Pin|POWER_KEY_Pin, GPIO_PIN_RESET);
    osDelay(2000);
    HAL_GPIO_WritePin(GPIOA, VS_RESET_Pin|POWER_KEY_Pin, GPIO_PIN_SET);
    
    gsm.errorProcessingCallback = gsmErrorProcessing;   
    gsm.huart = &huart3;
    if (!myMalloc((void*)&gsm.rxBuf,  GSM_BUFSIZE_RX, 1000)) {
        gsmErrorProcessing("Allocate RAM", GSM_BUFSIZE_RX);
        return;
    }
    if (!myMalloc((void*)&gsm.txBuf,  GSM_BUFSIZE_TX, 1000)) {
        gsmErrorProcessing("Allocate RAM", GSM_BUFSIZE_RX);
        myFree((void*)&gsm.rxBuf);
        return;
    }
    gsm.txSemaphore = gsmTxSemHandle;
    gsm.rxSize = GSM_BUFSIZE_RX;
    gsm.txSize = GSM_BUFSIZE_TX;
    AT_Start(&gsm);
    printf(" ok.\n");
}
//------------------------------------------------------------------------------

void gsmTask(void)
{
    uint32_t answerNum;
    static char str[100];
    
    answerNum = AT_Command(&gsm, "AT+COPS?\r", 1000, 2, "+COPS:", "+CME ERROR:");
    if (AT_Gets(&gsm, str, sizeof(str), 1000)) {
        printf("\nOperator = \"%s\" (answer %d)", str, answerNum);
    }
    
    osDelay(2000);
}
//------------------------------------------------------------------------------
