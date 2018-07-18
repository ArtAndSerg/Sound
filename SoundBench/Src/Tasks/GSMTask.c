// file mainTask.c

#include "main.h"
#include "stm32f1xx_hal.h"
#include "my/myTasks.h"
#include "../src/AT_com/ATcom.h"

#define GSM_BUFSIZE_RX 1024
#define GSM_BUFSIZE_TX 1024

AT_result_t gsmLinkUp (void);
AT_result_t gsmIncomingCall (void);

extern UART_HandleTypeDef huart1;
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
    
    gsm.errorLoggingCallback = gsmErrorProcessing;   
    gsm.huart = &huart1;
    if (!myCalloc((void*)&gsm.rxBuf,  GSM_BUFSIZE_RX, 1000)) {
        gsmErrorProcessing("Allocate RAM", GSM_BUFSIZE_RX);
        return;
    }
    //gsm.txSemaphore = gsmTxSemHandle;
    gsm.rxSize = GSM_BUFSIZE_RX;
    AT_Start(&gsm);
    printf(" ok.\n");
}
//------------------------------------------------------------------------------

void gsmTask(void)
{
    int answerNum;
    char str[10];
    memset(str, 0, 10);
    AT_result_t res;
    
    // "RDY\r", "+CPIN: ",    "Call Ready\r",   "SMS Ready\r",   "RING\r",   "NO CARRIER\r");
    printf(".");
    
    
    if (AT_LookupStr(&gsm, "+CPIN: ")) {
        AT_Gets(&gsm, str, 10, 100);
        printf("\n>> +CPIN: \"%s\"\n", str);
    }
    if (AT_LookupStr(&gsm, "RING\r")) {
        printf("\n>> Ring!\n");
    }
    if (AT_LookupStr(&gsm, "RDY\r")) {
        printf("\n>> Reary.\n");
    }
    AT_IncomingResetLookup(&gsm);
    
    osDelay(1000);
    
    
    
    /*
    
    if (answerNum == 2) {
        AT_Gets(&gsm, str, 10, 100);
    }
    if (answerNum == 5) {
        AT_Command(&com, "ATH\r", 1, "" 
    }
    printf("ANSWER = %d   \"%s\"\n", answerNum, str);
    //printf("%s\n", gsm.rxBuf);
    //osDelay(1000);
    //res = AT_Command(&gsm, &answerNum, "AT+COPS?\r", 1000, 2, "+COPS:", "+CME ERROR:");
    //res = AT_Gets(&gsm, str, sizeof(str), 1000);
    //      printf("\nOperator = \"%s\" (answer %d)", str, answerNum);
    //}
    */
   // 
}
//------------------------------------------------------------------------------

AT_result_t gsmLinkUp (void)
{
    return AT_OK;
}
//------------------------------------------------------------------------------

AT_result_t gsmIncomingCall(void) 
{
    return AT_OK;
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