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

int gsmState = 0;
int echo = 0;

bool gsmIncomingCommandsProcessing(void);

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
    printf("\n\nERROR! %s - %d  ", errorMessage, errorCode);
    switch (gsm.lastResult) {
      case AT_ERROR_ECHO:               printf("AT_ERROR_ECHO");       break;
      case AT_ERROR_FORMAT:             printf("AT_ERROR_FORMAT");     break;
      case AT_ERROR_INCOMING_OVERFLOW:  printf("AT_ERROR_INCOMING_OVERFLOW");  break;
      case AT_ERROR_RX:                 printf("AT_ERROR_RX");         break;
      case AT_ERROR_TX:                 printf("AT_ERROR_TX");         break;
      case AT_OK:                       printf("AT_OK");               break;
      case AT_REBOOT:                   printf("AT_REBOOT");           break;
      case AT_TIMEOUT:                  printf("AT_TIMEOUT");          break;
    }
    printf ("   \"");
    AT_PrintfLastCommand(&gsm);
    printf ("\" \n");
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

bool gsmIncomingCommandsProcessing(void)
{
    bool res = false;
    static char str[100];

    

    if (AT_LookupStr(&gsm, "+BTPAIRING: \"")) {
        AT_Gets(&gsm, str, sizeof(str), "\"");
        printf("Pairing with phone \"%s\"\n", str);
        AT_SendString(&gsm, "AT+BTPAIR=1,1\r");
        res = true;
    }
    
    if (AT_LookupStr(&gsm, "+BTCONNECT: ")) {
        AT_Gets(&gsm, str, sizeof(str), "\"");
        AT_Gets(&gsm, str, sizeof(str), "\"");
        printf("Connected phone \"%s\"\n", str);
        res = true;
    }
    
    if (AT_LookupStr(&gsm, "+BTCONNECTING: \"")) {
        printf("Connected programm\n");
        AT_SendString(&gsm, "AT+BTACPT=1\r");
        res = true;
    }
    
    
    if (AT_LookupStr(&gsm, "+BTSPPDATA: ")) {
        AT_Gets(&gsm, str, sizeof(str), ",");
        AT_Gets(&gsm, str, sizeof(str), ",");
        AT_Gets(&gsm, str, sizeof(str), NULL);
        printf("<<<<<< \"%s\"\n", str);
        if (str[0] == '1') {
            echo = 1;
        } 
        //echo++;
        res = true;
    }
    
    

    

    
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
        AT_Gets(&gsm, str, 100, NULL);
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



void gsmTask(void)
{
    int answerNum, pecho=0;
    static char str[100];
    memset(str, 0, 10);
    AT_result_t res;
   
    AT_WaitCommand(&gsm, "+CPIN: READY", 20000);
    osDelay(3000);    
    // "RDY\r", "+CPIN: ",    "Call Ready\r",   "SMS Ready\r",   "RING\r",   "NO CARRIER\r");
    if (AT_Command(&gsm, "AT+BTHOST=Postamat\r", 3000, 2, "OK\r", "ERROR\r") == 1) {
        printf("BT is on!\n");
    }
    
    if (AT_Command(&gsm, "AT+BTPOWER=1\r", 10000, 2, "OK\r", "ERROR") == 1) {
        printf("BT is in power!\n");
        
    }
    osDelay(500);
    
    
    while (1) {
        while (AT_WaitCommand(&gsm, NULL, 1000));
        if (echo) {
            if (AT_Command(&gsm, "AT+BTRSSI=1\r", 5000, 2, "+BTRSSI: ", "ERROR\r") == 1) {
                AT_Gets(&gsm, str, sizeof(str), NULL);
                strcat(str, " RSSI\r\n\r\n\r\n");
                //osDelay(3000);
                if (AT_Command(&gsm, "AT+BTSPPSEND=10\r", 5000, 2, "> ", "ERROR\r") == 1) {
                    //sprintf(str, "%04d\r\n\r\n", echo);
                    //sprintf(str, "Shiptor!\r\n\r");
                    AT_SendString(&gsm, str);
                    printf(">>>>>>  ", str);
                }
            } else {
                printf("|\n");
                echo = 0;
               // break;
            }
        }
        pecho = echo;
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