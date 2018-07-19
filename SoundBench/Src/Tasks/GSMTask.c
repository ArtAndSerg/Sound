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

bool gsmIncomingCommandsProcessing(void)
{
    bool res = false;
    static char str[100];
    
    if (AT_LookupStr(&gsm, "RING\r")) {
        printf("\n>> Ring!\n");
        AT_SendString(&gsm, "ATH\r");
        res = true;
    }
    if (AT_LookupStr(&gsm, "RDY\r")) {
        printf("\n>> Ready.\n");
        res = true;
    }
    if (AT_LookupStr(&gsm, "+CLIP: ")) {
        AT_Gets(&gsm, str, 100);
        printf("\n>> Calling: %s\n", str);
        res = true;
    }
    
    if (AT_LookupStr(&gsm, "NORMAL POWER DOWN\r")) {
        printf("\n>> Power down.\n");
        osDelay(1000);
        HAL_NVIC_SystemReset();
        res = true;
    }
    
    return res;
}
//------------------------------------------------------------------------------

void InitGsmTask(void)
{
    printf("\n\nGSM init... ");
    HAL_GPIO_WritePin(GPIOA, VS_RESET_Pin|POWER_KEY_Pin, GPIO_PIN_RESET);
    osDelay(2000);
    HAL_GPIO_WritePin(GPIOA, VS_RESET_Pin|POWER_KEY_Pin, GPIO_PIN_SET);
    osDelay(1000);
    gsm.errorLoggingCallback = gsmErrorProcessing; 
    gsm.incomingCommandsProcessing = gsmIncomingCommandsProcessing;
    gsm.huart = &huart1;
    if (!myCalloc((void*)&gsm.rxBuf,  GSM_BUFSIZE_RX, 1000)) {
        gsmErrorProcessing("Allocate RAM", GSM_BUFSIZE_RX);
        return;
    }
    //gsm.txSemaphore = gsmTxSemHandle;
    gsm.rxSize = GSM_BUFSIZE_RX;
    gsm.useEcho = true;
    AT_Start(&gsm);
    printf(" ok.\n");
}
//------------------------------------------------------------------------------

void gsmTask(void)
{
    int answerNum;
    static char str[100];
    memset(str, 0, 10);
    AT_result_t res;
    
    // "RDY\r", "+CPIN: ",    "Call Ready\r",   "SMS Ready\r",   "RING\r",   "NO CARRIER\r");
    if (AT_Command(&gsm, "AT+CLIP=1\r", 1000, 2, "OK\r", "ERROR\r") == 1) {
        printf("AOH is on!\n");
    }
    
    
    if (AT_Command(&gsm, "AT+COPS?\r", 1000, 2, "+COPS:", "ERROR") == 1)
    {
        AT_Gets(&gsm, str, 100);
        printf("Operator - \"%s\"\n", str);
        AT_ClearCurrentCommand(&gsm);
    }
    
    for (int i = 0; i < 10; i++) {
        while (AT_LookupNextCommand(&gsm, 100)) {
            AT_ClearCurrentCommand(&gsm);
        }
    }
    
    
    
    
   // osDelay(1000);
    
    
    
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

41 54 2B 43 4F 50 53 3F 0D 0D 0A 2B 43 4F 50 53 3A 20 30 2C 30 2C 22 42 65 65 20 4C 69 6E 65 20 47 53 4D 22 0D 0A 0D 0A 4F 4B 0D 0A                                                                                                                                                                                                                                                                                                                                                             
AT+COPS?...+COPS: 0,0,"Bee Line GSM"....OK..


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