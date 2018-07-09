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
    uint32_t answerNum = -1;
    static char str[100];
    AT_result_t res;
    
    res = AT_Command(&gsm, &answerNum, "AT+COPS?\r", 1000, 2, "+COPS:", "+CME ERROR:");
    res = AT_Gets(&gsm, str, sizeof(str), 1000);
    //      printf("\nOperator = \"%s\" (answer %d)", str, answerNum);
    //}
    
    osDelay(2000);
}
//------------------------------------------------------------------------------


/*
NORMAL POWER DOWN

RDY

+CFUN: 1

+CPIN: READY

Call Ready

SMS Ready
AT+SAPBR=1,1AT+SAPBR=1,1
OK
AT+HTTPINITAT+HTTPINIT
OK
AT+HTTPACTION=0AT+HTTPACTION=0
ERROR
AT+HTTPPARA="URL","sergkit.test.host-vrn.ru"AT+HTTPPARA="URL","sergkit.test.host-vrn.ru"
OK
AT+HTTPACTION=0AT+HTTPACTION=0
OK

+HTTPACTION: 0,200,124
AT+HTTPDATA=0,200AT+HTTPDATA=0,200
ERROR
AT+HTTPREAD=0,200AT+HTTPREAD=0,200
+HTTPREAD: 124
GET<br><br>POST<br>
--------------------------
OK! Was successfully written  bytes in file 
=====================



The oldest data was removed. Continue...
AT+HTTPACTION=1
OK

+HTTPACTION: 1,200,272
AT+HTTPREAD=0,200AT+HTTPREAD=0,200
+HTTPREAD: 200

GET<br>

<br>POST<br>

GET<br>

<br>POST<br>
GET<br>$_GET[key] = 4546464<br><br>POST<br>
--------------------------
OK! Was successfully written  bytes in file 
=====================



OK
AT+HTTPACTION=1AT+HTTPACTION=1
OK

+HTTPACTION: 1,200,334
AT+HTTPACTION=1AT+HTTPACTION=1
OK

+HTTPACTION: 1,200,396
AT+HTTPREAD=0,400AT+HTTPREAD=0,400
+HTTPREAD: 396

GET<br>

<br>POST<br>

GET<br>

<br>POST<br>

GET<br>

<br>POST<br>

GET<br>

<br>POST<br>
GET<br>$_GET[key] = 4546464<br><br>POST<br>
--------------------------
OK! Was successfully written  bytes in file 
=====================




Was successfully written.




Was successfully written.




Was successfully written.




Was successfully written.




OK
AT+HTTPACTION=1AT+HTTPACTION=1
OK

+HTTPACTION: 1,200,458
AT+HTTPREAD=0,400AT+HTTPREAD=0,400
+HTTPREAD: 400

GET<br>

<br>POST<br>

GET<br>

<br>POST<br>

GET<br>

<br>POST<br>

GET<br>

<br>POST<br>

GET<br>

<br>POST<br>
GET<br>$_GET[key] = 4546464<br><br>POST<br>
--------------------------
OK! Was successfully written  bytes in file 
=====================




Was successfully written.




Was successfully written.




Was successfully written.




Was succes
OK
AT+HTTPPARA="USERDATA","{\"id\":1}" AT+HTTPPARA="USERDATA","{\"id\":1}" 
OK
AT+HTTPPARA="USERDATA","{\"id\":1}" AT+HTTPPARA="USERDATA","{\"id\":1}" 
OK
AT+HTTPACTION=0AT+HTTPACTION=0
OK

+HTTPACTION: 0,200,62
AT+HTTPACTION=0AT+HTTPACTION=0
OK

+HTTPACTION: 0,200,62
AT+HTTPACTION=1AT+HTTPACTION=1
OK

+HTTPACTION: 1,200,124
AT+HTTPACTION=1AT+HTTPACTION=1
OK

+HTTPACTION: 1,200,186
AT+HTTPACTION=1AT+HTTPACTION=1
OK

+HTTPACTION: 1,200,248
AT+HTTPREAD=0,400AT+HTTPREAD=0,400
+HTTPREAD: 248

GET<br>

<br>POST<br>

GET<br>

<br>POST<br>

GET<br>

<br>POST<br>

GET<br>

<br>POST<br>

Was successfully written.




Was successfully written.




Was successfully written.




Was successfully written.




OK
AT+HTTPACTION=1AT+HTTPACTION=1
OK

+HTTPACTION: 1,200,310
AT+HTTPREAD=0,400AT+HTTPREAD=0,400
+HTTPREAD: 310

GET<br>

<br>POST<br>

GET<br>

<br>POST<br>

GET<br>

<br>POST<br>

GET<br>

<br>POST<br>

GET<br>

<br>POST<br>

Was successfully written.




Was successfully written.




Was successfully written.




Was successfully written.




Was successfully written.




OK
HTTPTERMHTTPTERMAT+HTTPTERMAT+HTTPTERM
OK
AT+HTTPTERMAT+HTTPTERM
ERROR
AT+HTTPTERMAT+HTTPTERM
ERROR
AT+HTTPINITAT+HTTPINIT
OK
AT+HTTPINITAT+HTTPINIT
ERROR
AT+HTTPINITAT+HTTPINIT
ERROR
AT+HTTPACTION=1AT+HTTPACTION=1
ERROR
AT+HTTPACTION=1AT+HTTPACTION=1
ERROR
AT+HTTPPARA="USERDATA","{\"id\":1}" AT+HTTPPARA="USERDATA","{\"id\":1}" 
OK
AT+HTTPPARA="USERDATA","{\"id\":1}" AT+HTTPPARA="USERDATA","{\"id\":1}" 
OK
AT+HTTPPARA="URL","sergkit.test.host-vrn.ru" AT+HTTPPARA="URL","sergkit.test.host-vrn.ru" 
OK
AT+HTTPACTION=1AT+HTTPACTION=1
OK

+HTTPACTION: 1,200,62
AT+HTTPACTION=1AT+HTTPACTION=1
OK

+HTTPACTION: 1,200,124
AT+HTTPREAD=0,400AT+HTTPREAD=0,400
+HTTPREAD: 124

GET<br>

<br>POST<br>

GET<br>

<br>POST<br>

Was successfully written.




Was successfully written.




OK
AT+HTTPACTION=1AT+HTTPACTION=1
OK

+HTTPACTION: 1,200,186
AT+HTTPREAD=0,400AT+HTTPREAD=0,400
+HTTPREAD: 186

GET<br>

<br>POST<br>

GET<br>

<br>POST<br>

GET<br>

<br>POST<br>

Was successfully written.




Was successfully written.




Was successfully written.




OK
AT+HTTPDATAAT+HTTPDATA
ERROR
AT+HTTPDATA=?AT+HTTPDATA=?
+HTTPDATA: (0-319488),(1000-120000)

OK
AT+HTTPDATA=10,10000AT+HTTPDATA=10,10000
DOWNLOAD

OK
AT+HTTPDATA=10,10000AT+HTTPDATA=10,10000
DOWNLOAD
{"id":1}{"id":1}
OK
AT+HTTPACTION=1AT+HTTPACTION=1
OK

+HTTPACTION: 1,200,72
AT+HTTPREAD=0,400AT+HTTPREAD=0,400
+HTTPREAD: 72

GET<br>

<br>POST<br>
{"id":1}{"
Was successfully written.




OK
AT+HTTPDATA=0,10000AT+HTTPDATA=0,10000
OK
AT+HTTPREAD=0,400AT+HTTPREAD=0,400
OK
AT+HTTPACTION=1AT+HTTPACTION=1
OK

+HTTPACTION: 1,200,62
AT+HTTPACTION=1AT+HTTPACTION=1
OK

+HTTPACTION: 1,200,124
AT+HTTPACTION=1AT+HTTPACTION=1
OK

+HTTPACTION: 1,200,186
AT+HTTPDATA=0,10000AT+HTTPDATA=0,10000
OK
AT+HTTPACTION=1AT+HTTPACTION=1
OK

+HTTPACTION: 1,200,62
AT+HTTPREAD=0,400AT+HTTPREAD=0,400
+HTTPREAD: 62

GET<br>

<br>POST<br>

Was successfully written.




OK
OK

*/