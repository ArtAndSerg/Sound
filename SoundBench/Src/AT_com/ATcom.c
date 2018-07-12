// file: ATcom.c

#include <string.h>
#include "cmsis_os.h"
//#include "stm32f4xx_hal.h"
#include "stm32f1xx_hal.h"
#include "../src/AT_com/ATcom.h"

static AT_result_t AT_TxProcess(ATcom_t *com); 

bool AT_Start(ATcom_t *com)   // must added at start of programm and in HAL_UART_ErrorCallback  !
{
    com->rxLen = 0;
    com->rxPtr = 0;
    HAL_UART_Abort(com->huart);
    if (HAL_UART_Receive_DMA(com->huart, com->rxBuf, com->rxSize) != HAL_OK) {
        com->errorProcessingCallback("Can't start Rx DMA", 0);
        return false;
    }
    return true;
}
//---------------------------------------------------------------------------

void AT_RxUartDmaISR(ATcom_t *com)  // must added to HAL_UART_RxCpltCallback  and  HAL_UART_RxHalfCpltCallback  
{
    com->rxLen += com->rxSize / 2; 
}
//---------------------------------------------------------------------------

void AT_txCompleteISR (ATcom_t *com)
{
   portBASE_TYPE xTaskWoken;
   
   HAL_UART_Abort(com->huart);
   xSemaphoreGiveFromISR(com->txSemaphore, &xTaskWoken );
   if (xTaskWoken == pdTRUE) {
       taskYIELD();
   } 
}
//---------------------------------------------------------------------------

bool AT_SendRaw(ATcom_t *com, uint8_t *data, uint16_t len)
{
    HAL_StatusTypeDef res;
    if (len <= com->txSize)	{
		memcpy(com->txBuf, data, len);
        res = HAL_UART_Transmit_DMA(com->huart, data, len);
		if (res == HAL_OK) {
            return true;
        } else {
			com->errorProcessingCallback("Can't transmit via DMA", (int)res);
            return false;
        }
	} 
    com->errorProcessingCallback("Too many data for TX", len);
    return false;
}
//---------------------------------------------------------------------------

bool AT_GetByte(ATcom_t *com, uint8_t *byte)
{
    int writePtr = com->rxSize - com->huart->hdmarx->Instance->CNDTR;
    if (com->rxLen >= com->rxSize){
        com->errorProcessingCallback("Rx buffer overflow", com->rxLen - com->rxSize);
        AT_Start(com);
        return false;
    }
    if (com->rxPtr != writePtr) {
        if (byte != NULL) {
            *byte = com->rxBuf[com->rxPtr];
        }
        com->rxPtr++;
        if (com->rxPtr == com->rxSize) {
            com->rxPtr = 0;
        }
        return true;
    }
    return false;
}
//---------------------------------------------------------------------------

int  AT_GetData(ATcom_t *com, uint8_t *buf, int bufSize, uint32_t timeout)
{
    int n = 0;
    
    for (int i = 0; i < timeout+1; i += ATCOM_MIN_TIMEOUT) {
        while (AT_GetByte(com, buf)) {
            n++;
            if (buf != NULL) {
                buf++;
            }
            if (n == bufSize) {
                return n;
            }
        }
        osDelay(ATCOM_MIN_TIMEOUT);       
    }
    return n;
}
//---------------------------------------------------------------------------

bool AT_SendString(ATcom_t *com, char *data)
{
	return AT_SendRaw(com, (uint8_t*)data, strlen(data));
}
//---------------------------------------------------------------------------

void AT_SkipAllCrLf(ATcom_t *com, uint32_t timeout)
{
    uint32_t pPtr;
    uint8_t byte = 0;
    
    for (int i = 0; i < timeout+1; i+= ATCOM_MIN_TIMEOUT) {
        pPtr = com->rxPtr;
        while (AT_GetByte(com, &byte)) {
            if (byte != '\n' && byte != '\r' && byte != '\0') {
              com->rxPtr = pPtr;
              return;
            }
        }
        osDelay(ATCOM_MIN_TIMEOUT); 
    }
}
//---------------------------------------------------------------------------

void AT_SkipRxData(ATcom_t *com, uint32_t writePtr, uint32_t count)
{
    uint8_t byte = 0;
    for (int i = 0; i < count; i++) {
        if (!AT_GetByte(com, &byte) || com->rxPtr == writePtr) {
            break;
        }
    }
}
//---------------------------------------------------------------------------

uint32_t AT_Gets(ATcom_t *com, char *str, uint32_t strSize, uint32_t timeout) 
{
    int n = 0;
    
    for (int i = 0; i < timeout+1; i += ATCOM_MIN_TIMEOUT) {
        while (AT_GetByte(com, (uint8_t*)&str[n])) {
            if (n == strSize - 1 || str[n] == '\n' || str[n] == '\r' || str[n] == '\0') {
                str[n] = '\0';
                AT_SkipAllCrLf(com, 0);
                return n + 1;
            }
            n++;
        }
        osDelay(ATCOM_MIN_TIMEOUT);       
    }
    str[n] = '\0';
    return n;
}
//---------------------------------------------------------------------------

uint32_t AT_FindString(ATcom_t *com, char *str, uint32_t writePtr)  // return position (0 if string not found)
{
  uint32_t readPtr = com->rxPtr, len = strlen(str), i = 0, n = 0;
    
  while (readPtr != writePtr) {
      n++;
      if (com->rxBuf[readPtr] == str[i]) {
          i++;
          if (i == len) {
              return n;
          }
      } else {
          i = 0;
      }
      readPtr++;
      if (readPtr == com->rxSize) {
          readPtr = 0;
      }
  }
  return 0;
}
//---------------------------------------------------------------------------

void AT_AlwaysWaitingStrings(ATcom_t *com) 
{
    
}
//---------------------------------------------------------------------------

AT_result_t AT_Command(ATcom_t *com, int *result, char *command, uint32_t timeout, uint32_t countOfParameters, ...)
{	
    char *waitingParam[WAIT_PARAM_MAX_COUNT];
    uint8_t n[WAIT_PARAM_MAX_COUNT];
    char  byte;
    uint32_t timer = 0, commandLen, m = 0;
    
    
    if (countOfParameters > WAIT_PARAM_MAX_COUNT || com == NULL || command == NULL) {
        com->errorProcessingCallback("AT_Command format error", countOfParameters);
        return AT_ERROR_FORMAT; 
    }
    if (!AT_SendString(com, command)) {
        return AT_ERROR_SEND;
    }
    do{
        osDelay(ATCOM_MIN_TIMEOUT);        
        timer += ATCOM_MIN_TIMEOUT;
        while (AT_GetByte(com, (uint8_t*)&byte)) {
            if (command[m] == byte) {
                m++;
                if (m == strlen(command)) {
                    break;
                } 
            } else {
                m = 0; 
            }
        }
	}while (timer < timeout && m < strlen(command));
    if (timer >= timeout) {
        return AT_ERROR_ECHO;
    }
    if (!countOfParameters) {   
        return AT_OK;
    }
        
    va_list tag;
	va_start (tag, countOfParameters);
	for(int i = 0; i < countOfParameters; i++) {
		waitingParam[i] = va_arg (tag, char *);	
        if (waitingParam[i] == NULL || result == NULL) {
            com->errorProcessingCallback("AT_Command parameter format error", i);
            return AT_ERROR_FORMAT; 
        }
    }
    va_end (tag);
	memset(&n, 0, sizeof(n));
    timer = 0;
    do {
        osDelay(ATCOM_MIN_TIMEOUT);        
        timer += ATCOM_MIN_TIMEOUT;
        while (AT_GetByte(com, (uint8_t*)&byte)) {
            for (int i = 0; i < countOfParameters; i++) {
                if (waitingParam[i][n[i]] == byte) {
                    n[i]++;
                    if (n[i] == strlen(waitingParam[i])) {
                        AT_SkipAllCrLf(com, ATCOM_MIN_TIMEOUT); 
                        *result = i;
                        return AT_OK;
                    }
                } else {
                    n[i] = 0; 
                }
            }
        }
	} while (timer < timeout);
	return AT_TIMEOUT;	
}
//------------------------------------------------------------------------------
   
               
               /*
bool Wifi_ReturnString(char *result,uint8_t WantWhichOne,char *SplitterChars)
{
	if(result == NULL) 
		return false;
	if(WantWhichOne==0)
		return false;

	char *str = (char*)Wifi.RxBuffer;
	

	str = strtok (str,SplitterChars);
	if(str == NULL)
	{
		strcpy(result,"");
		return false;
	}
	while (str != NULL)
  {
    str = strtok (NULL,SplitterChars);
		if(str != NULL)
			WantWhichOne--;
		if(WantWhichOne==0)
		{
			strcpy(result,str);
			return true;
		}
  }
	strcpy(result,"");
	return false;	
}

//#########################################################################################################
bool	Wifi_ReturnStrings(char *InputString,char *SplitterChars,uint8_t CountOfParameter,...)
{
	if(CountOfParameter == 0)
		return false;
	va_list tag;
	va_start (tag,CountOfParameter);
	char *arg[CountOfParameter];
	for(uint8_t i=0; i<CountOfParameter ; i++)
		arg[i] = va_arg (tag, char *);	
  va_end (tag);
	
	char *str;
	str = strtok (InputString,SplitterChars);
	if(str == NULL)
		return false;
	uint8_t i=0;
	while (str != NULL)
  {
    str = strtok (NULL,SplitterChars);
		if(str != NULL)
			CountOfParameter--;
		strcpy(arg[i],str);
		i++;
		if(CountOfParameter==0)
		{
			return true;
		}
  }
	return false;	
	
}
//#########################################################################################################
bool	Wifi_ReturnInteger(int32_t	*result,uint8_t WantWhichOne,char *SplitterChars)
{
	if((char*)Wifi.RxBuffer == NULL)
		return false;
	if(Wifi_ReturnString((char*)Wifi.RxBuffer,WantWhichOne,SplitterChars)==false)
		return false;
	*result = atoi((char*)Wifi.RxBuffer);
	return true;
}
//#########################################################################################################

bool	Wifi_ReturnFloat(float	*result,uint8_t WantWhichOne,char *SplitterChars)
{	
	if((char*)Wifi.RxBuffer == NULL)
		return false;
	if(Wifi_ReturnString((char*)Wifi.RxBuffer,WantWhichOne,SplitterChars)==false)
		return false;
	*result = atof((char*)Wifi.RxBuffer);
	return true;
}
//#########################################################################################################
void Wifi_RemoveChar(char *str, char garbage)
{
	char *src, *dst;
  for (src = dst = str; *src != '\0'; src++)
	{
		*dst = *src;
		if (*dst != garbage)
			dst++;
  }
  *dst = '\0';
}
//#########################################################################################################
void	Wifi_RxClear(void)
{
	memset(Wifi.RxBuffer,0,_WIFI_RX_SIZE);
	Wifi.RxIndex=0;	
  HAL_UART_Receive_IT(&_WIFI_USART,&Wifi.usartBuff,1);
}
//#########################################################################################################
void	Wifi_TxClear(void)
{
	memset(Wifi.TxBuffer,0,_WIFI_TX_SIZE);
}
//#########################################################################################################
void	Wifi_RxCallBack(void)
{
  //+++ at command buffer
  if(Wifi.RxIsData==false)                                              
  {
    Wifi.RxBuffer[Wifi.RxIndex] = Wifi.usartBuff;
    if(Wifi.RxIndex < _WIFI_RX_SIZE)
      Wifi.RxIndex++;
  }
  //--- at command buffer
  //+++  data buffer
  else                                                                  
  {
    if( HAL_GetTick()-Wifi.RxDataLastTime > 50)
      Wifi.RxIsData=false;
    //+++ Calculate Data len after +IPD
    if(Wifi.RxDataLen==0)
    {
      //+++ Calculate Data len after +IPD ++++++ Multi Connection OFF
      if (Wifi.TcpIpMultiConnection==false)
      {
        Wifi.RxBufferForDataTmp[Wifi.RxIndexForDataTmp] = Wifi.usartBuff;
        Wifi.RxIndexForDataTmp++;
        if(Wifi.RxBufferForDataTmp[Wifi.RxIndexForDataTmp-1]==':')
        {
          Wifi.RxDataConnectionNumber=0;
          Wifi.RxDataLen=atoi((char*)&Wifi.RxBufferForDataTmp[1]);
        }
      }
      //--- Calculate Data len after +IPD ++++++ Multi Connection OFF
      //+++ Calculate Data len after +IPD ++++++ Multi Connection ON
      else
      {
        Wifi.RxBufferForDataTmp[Wifi.RxIndexForDataTmp] = Wifi.usartBuff;
        Wifi.RxIndexForDataTmp++;
        if(Wifi.RxBufferForDataTmp[2]==',')
        {
          Wifi.RxDataConnectionNumber=Wifi.RxBufferForDataTmp[1]-48;
        }
        if((Wifi.RxIndexForDataTmp>3) && (Wifi.RxBufferForDataTmp[Wifi.RxIndexForDataTmp-1]==':'))
          Wifi.RxDataLen=atoi((char*)&Wifi.RxBufferForDataTmp[3]);
      }
      //--- Calculate Data len after +IPD ++++++ Multi Connection ON
    }
    //--- Calculate Data len after +IPD
    //+++ Fill Data Buffer
    else  
    {      
      Wifi.RxBufferForData[Wifi.RxIndexForData] = Wifi.usartBuff;
      if(Wifi.RxIndexForData < _WIFI_RX_FOR_DATA_SIZE)
        Wifi.RxIndexForData++;
      if( Wifi.RxIndexForData>= Wifi.RxDataLen)
      {
        Wifi.RxIsData=false;         
        Wifi.GotNewData=true;
      }
    }
    //--- Fill Data Buffer    
  }           
  //--- data buffer
	HAL_UART_Receive_IT(&_WIFI_USART,&Wifi.usartBuff,1);
  //+++ check +IPD in At command buffer
  if(Wifi.RxIndex>4)
  {
    if( (Wifi.RxBuffer[Wifi.RxIndex-4]=='+') && (Wifi.RxBuffer[Wifi.RxIndex-3]=='I') && (Wifi.RxBuffer[Wifi.RxIndex-2]=='P') && (Wifi.RxBuffer[Wifi.RxIndex-1]=='D'))
    {
      memset(Wifi.RxBufferForDataTmp,0,sizeof(Wifi.RxBufferForDataTmp));
      Wifi.RxBuffer[Wifi.RxIndex-4]=0;
      Wifi.RxBuffer[Wifi.RxIndex-3]=0;
      Wifi.RxBuffer[Wifi.RxIndex-2]=0;
      Wifi.RxBuffer[Wifi.RxIndex-1]=0;
      Wifi.RxIndex-=4;
      Wifi.RxIndexForData=0;
      Wifi.RxIndexForDataTmp=0;
      Wifi.RxIsData=true;
      Wifi.RxDataLen=0;  
      Wifi.RxDataLastTime = HAL_GetTick();      
    }
  }
  //--- check +IPD in At command buffer  
}
//#########################################################################################################
//#########################################################################################################
//#########################################################################################################
//#########################################################################################################
void WifiTask(void const * argument)
{
	osDelay(3000);
	Wifi_SendStringAndWait("AT\r\n",1000);
 	Wifi_SetRfPower(82);
  Wifi_TcpIp_GetMultiConnection();
  Wifi_TcpIp_Close(0);
  Wifi_TcpIp_Close(1);
  Wifi_TcpIp_Close(2);
  Wifi_TcpIp_Close(3);
  Wifi_TcpIp_Close(4);
  Wifi_TcpIp_SetMultiConnection(false);
	Wifi_GetMode();
	Wifi_Station_DhcpIsEnable();
	Wifi_UserInit();  


 
 
	//#######################		
	while(1)
	{	
		Wifi_GetMyIp();	
    if((Wifi.Mode==WifiMode_SoftAp) || (Wifi.Mode==WifiMode_StationAndSoftAp))
      Wifi_SoftAp_GetConnectedDevices();
		Wifi_TcpIp_GetConnectionStatus();
    Wifi_RxClear();  
		for(uint8_t i=0; i< 100; i++)
    {
      if( Wifi.GotNewData==true)
      {
        Wifi.GotNewData=false;
        for(uint8_t ii=0; ii<5 ; ii++)
        {
          if((strstr(Wifi.TcpIpConnections[ii].Type,"UDP")!=NULL) && (Wifi.RxDataConnectionNumber==Wifi.TcpIpConnections[ii].LinkId))
            Wifi_UserGetUdpData(Wifi.RxDataConnectionNumber,Wifi.RxDataLen,Wifi.RxBufferForData);        
          if((strstr(Wifi.TcpIpConnections[ii].Type,"TCP")!=NULL) && (Wifi.RxDataConnectionNumber==Wifi.TcpIpConnections[ii].LinkId))
            Wifi_UserGetTcpData(Wifi.RxDataConnectionNumber,Wifi.RxDataLen,Wifi.RxBufferForData);        
        }        
      }
      osDelay(10);
    }
    Wifi_UserProcess();
	}
}
//#########################################################################################################
//#########################################################################################################
//#########################################################################################################
//#########################################################################################################
void	Wifi_Init(osPriority	Priority)
{
	HAL_UART_Receive_IT(&_WIFI_USART,&Wifi.usartBuff,1);
	Wifi_RxClear();
	Wifi_TxClear();
	osSemaphoreDef(WifiSemHandle);
	WifiSemHandle = osSemaphoreCreate(osSemaphore(WifiSemHandle), 1);
	osThreadDef(WifiTaskName, WifiTask, Priority, 0, _WIFI_TASK_SIZE);
	WifiTaskHandle = osThreadCreate(osThread(WifiTaskName), NULL);	
}
//#########################################################################################################
//#########################################################################################################
//#########################################################################################################
//#########################################################################################################
bool	Wifi_Restart(void)
{
	osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+RST\r\n");
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;			
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;	
}
//#########################################################################################################
bool	Wifi_DeepSleep(uint16_t DelayMs)
{
	osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+GSLP=%d\r\n",DelayMs);
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;			
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;		
}
//#########################################################################################################
bool	Wifi_FactoryReset(void)
{
	osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+RESTORE\r\n");
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;			
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;		
}
//#########################################################################################################
bool	Wifi_Update(void)
{
	osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+CIUPDATE\r\n");
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(1000*60*5,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;			
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;			
}
//#########################################################################################################
bool	Wifi_SetRfPower(uint8_t Power_0_to_82)
{
	osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+RFPOWER=%d\r\n",Power_0_to_82);
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;			
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;		
}
//#########################################################################################################
//#########################################################################################################
//#########################################################################################################
//#########################################################################################################
bool	Wifi_SetMode(WifiMode_t	WifiMode_)
{
	osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+CWMODE_CUR=%d\r\n",WifiMode_);
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;			
		Wifi.Mode = WifiMode_;
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;		
}
//#########################################################################################################
bool	Wifi_GetMode(void)
{
	osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+CWMODE_CUR?\r\n");
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;			
		if(Wifi_ReturnInteger((int32_t*)&result,1,":"))
			Wifi.Mode = (WifiMode_t)result ;
		else
			Wifi.Mode = WifiMode_Error;
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;		
}
//#########################################################################################################
bool	Wifi_GetMyIp(void)
{	
	osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+CIFSR\r\n");
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;		
		sscanf((char*)Wifi.RxBuffer,"AT+CIFSR\r\r\n+CIFSR:APIP,\"%[^\"]",Wifi.MyIP);
    sscanf((char*)Wifi.RxBuffer,"AT+CIFSR\r\r\n+CIFSR:STAIP,\"%[^\"]",Wifi.MyIP);			
    
    
    Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+CIPSTA?\r\n");
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;	
    
    char *str=strstr((char*)Wifi.RxBuffer,"gateway:");
    if(str==NULL)
      break;
    if(Wifi_ReturnStrings(str,"\"",1,Wifi.MyGateWay)==false)
      break;    
    
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;		
}

//#########################################################################################################
//#########################################################################################################
//#########################################################################################################
//#########################################################################################################
bool	Wifi_Station_ConnectToAp(char *SSID,char *Pass,char *MAC)
{
	osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		if(MAC==NULL)
			sprintf((char*)Wifi.TxBuffer,"AT+CWJAP_CUR=\"%s\",\"%s\"\r\n",SSID,Pass);
		else
			sprintf((char*)Wifi.TxBuffer,"AT+CWJAP_CUR=\"%s\",\"%s\",\"%s\"\r\n",SSID,Pass,MAC);
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_VERYHIGH,&result,3,"\r\nOK\r\n","\r\nERROR\r\n","\r\nFAIL\r\n")==false)
			break;
		if( result > 1)
			break;		
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;		
}
//#########################################################################################################
bool	Wifi_Station_Disconnect(void)
{
	osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+CWQAP\r\n");
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;				
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;		
}
//#########################################################################################################
bool	Wifi_Station_DhcpEnable(bool Enable)
{         
	osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+CWDHCP_CUR=1,%d\r\n",Enable);
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;		
		Wifi.StationDhcp=Enable;		
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;		
}
//#########################################################################################################
bool	Wifi_Station_DhcpIsEnable(void)
{
	osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+CWDHCP_CUR?\r\n");
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;		
		if(Wifi_ReturnInteger((int32_t*)&result,1,":")==false)
			break;
		switch(result)
		{
			case 0:
				Wifi.StationDhcp=false;
				Wifi.SoftApDhcp=false;				
			break;
			case 1:
				Wifi.StationDhcp=false;
				Wifi.SoftApDhcp=true;				
			break;
			case 2:
				Wifi.StationDhcp=true;
				Wifi.SoftApDhcp=false;				
			break;
			case 3:
				Wifi.StationDhcp=true;
				Wifi.SoftApDhcp=true;				
			break;			
		}
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;		
}
//#########################################################################################################
bool	Wifi_Station_SetIp(char *IP,char *GateWay,char *NetMask)
{
	osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+CIPSTA_CUR=\"%s\",\"%s\",\"%s\"\r\n",IP,GateWay,NetMask);
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;		
		Wifi.StationDhcp=false;		
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;		
}
//#########################################################################################################

//#########################################################################################################

//#########################################################################################################
//#########################################################################################################
//#########################################################################################################
//#########################################################################################################
bool  Wifi_SoftAp_GetConnectedDevices(void)
{
  osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+CWLIF\r\n");
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;		
		Wifi_RemoveChar((char*)Wifi.RxBuffer,'\r');
    Wifi_ReturnStrings((char*)Wifi.RxBuffer,"\n,",10,Wifi.SoftApConnectedDevicesIp[0],Wifi.SoftApConnectedDevicesMac[0],Wifi.SoftApConnectedDevicesIp[1],Wifi.SoftApConnectedDevicesMac[1],Wifi.SoftApConnectedDevicesIp[2],Wifi.SoftApConnectedDevicesMac[2],Wifi.SoftApConnectedDevicesIp[3],Wifi.SoftApConnectedDevicesMac[3],Wifi.SoftApConnectedDevicesIp[4],Wifi.SoftApConnectedDevicesMac[4]);
		for(uint8_t i=0 ; i<6 ; i++)
    {
      if( (Wifi.SoftApConnectedDevicesIp[i][0]<'0') || (Wifi.SoftApConnectedDevicesIp[i][0]>'9'))
        Wifi.SoftApConnectedDevicesIp[i][0]=0;      
      if( (Wifi.SoftApConnectedDevicesMac[i][0]<'0') || (Wifi.SoftApConnectedDevicesMac[i][0]>'9'))
        Wifi.SoftApConnectedDevicesMac[i][0]=0;      
    }
    
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;			
}
//#########################################################################################################
bool  Wifi_SoftAp_Create(char *SSID,char *password,uint8_t channel,WifiEncryptionType_t WifiEncryptionType,uint8_t MaxConnections_1_to_4,bool HiddenSSID)
{
  osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+CWSAP=\"%s\",\"%s\",%d,%d,%d,%d\r\n",SSID,password,channel,WifiEncryptionType,MaxConnections_1_to_4,HiddenSSID);
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;		  
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;		  
}
//#########################################################################################################
//#########################################################################################################
//#########################################################################################################
//#########################################################################################################
bool  Wifi_TcpIp_GetConnectionStatus(void)
{
	osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+CIPSTATUS\r\n");
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;		
		
    
		char *str = strstr((char*)Wifi.RxBuffer,"\nSTATUS:");
    if(str==NULL)
      break;
    str = strchr(str,':');
    str++;
    for(uint8_t i=0 ; i<5 ;i++)
      Wifi.TcpIpConnections[i].status=(WifiConnectionStatus_t)atoi(str);
    str = strstr((char*)Wifi.RxBuffer,"+CIPSTATUS:");
    for(uint8_t i=0 ; i<5 ;i++)
    {
      sscanf(str,"+CIPSTATUS:%d,\"%3s\",\"%[^\"]\",%d,%d,%d",(int*)&Wifi.TcpIpConnections[i].LinkId,Wifi.TcpIpConnections[i].Type,Wifi.TcpIpConnections[i].RemoteIp,(int*)&Wifi.TcpIpConnections[i].RemotePort,(int*)&Wifi.TcpIpConnections[i].LocalPort,(int*)&Wifi.TcpIpConnections[i].RunAsServer);
      str++;
      str = strstr(str,"+CIPSTATUS:");
      if(str==NULL)
        break;
    }
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;			
}
//#########################################################################################################
bool  Wifi_TcpIp_Ping(char *PingTo)
{
  osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+PING=\"%s\"\r\n",PingTo);
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_MED,&result,3,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;		
    if(Wifi_ReturnInteger((int32_t*)&Wifi.TcpIpPingAnswer,2,"+")==false)
      break;    
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;		
}
//#########################################################################################################
bool  Wifi_TcpIp_SetMultiConnection(bool EnableMultiConnections)
{
  osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+CIPMUX=%d\r\n",EnableMultiConnections);
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;				
    Wifi.TcpIpMultiConnection=EnableMultiConnections;		
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;			
}
//#########################################################################################################
bool  Wifi_TcpIp_GetMultiConnection(void)
{
  
  osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
		sprintf((char*)Wifi.TxBuffer,"AT+CIPMUX?\r\n");
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;				
    if(Wifi_ReturnInteger((int32_t*)&result,1,":")==false)
      break;
    Wifi.TcpIpMultiConnection=(bool)result;		
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;			
}
//#########################################################################################################
bool  Wifi_TcpIp_StartTcpConnection(uint8_t LinkId,char *RemoteIp,uint16_t RemotePort,uint16_t TimeOut)
{
  osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
    Wifi_RxClear();
    sprintf((char*)Wifi.TxBuffer,"AT+CIPSERVER=1,%d\r\n",RemotePort);
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;		
		Wifi_RxClear();
    if(Wifi.TcpIpMultiConnection==false)
      sprintf((char*)Wifi.TxBuffer,"AT+CIPSTART=\"TCP\",\"%s\",%d,%d\r\n",RemoteIp,RemotePort,TimeOut);
    else
      sprintf((char*)Wifi.TxBuffer,"AT+CIPSTART=%d,\"TCP\",\"%s\",%d,%d\r\n",LinkId,RemoteIp,RemotePort,TimeOut);
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_HIGH,&result,3,"OK","CONNECT","ERROR")==false)
			break;
		if(result == 3)
			break;		
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;		
}
//#########################################################################################################
bool  Wifi_TcpIp_StartUdpConnection(uint8_t LinkId,char *RemoteIp,uint16_t RemotePort,uint16_t LocalPort)
{ 
  osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
    if(Wifi.TcpIpMultiConnection==false)
      sprintf((char*)Wifi.TxBuffer,"AT+CIPSTART=\"UDP\",\"%s\",%d,%d\r\n",RemoteIp,RemotePort,LocalPort);
    else
      sprintf((char*)Wifi.TxBuffer,"AT+CIPSTART=%d,\"UDP\",\"%s\",%d,%d\r\n",LinkId,RemoteIp,RemotePort,LocalPort);
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_HIGH,&result,3,"OK","ALREADY","ERROR")==false)
			break;
		if(result == 3)
			break;		
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;		
}
//#########################################################################################################
bool  Wifi_TcpIp_Close(uint8_t LinkId)
{
  osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
    if(Wifi.TcpIpMultiConnection==false)
      sprintf((char*)Wifi.TxBuffer,"AT+CIPCLOSE\r\n");
    else
      sprintf((char*)Wifi.TxBuffer,"AT+CIPCLOSE=%d\r\n",LinkId);
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;		
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;		
}
//#########################################################################################################
bool  Wifi_TcpIp_SetEnableTcpServer(uint16_t PortNumber)
{
  osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
    if(Wifi.TcpIpMultiConnection==false)
    {
      sprintf((char*)Wifi.TxBuffer,"AT+CIPMUX=1\r\n");
      if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
        break;
      if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
        break;    
      Wifi.TcpIpMultiConnection=true;
      Wifi_RxClear();      
    }
    else
      sprintf((char*)Wifi.TxBuffer,"AT+CIPSERVER=1,%d\r\n",PortNumber);
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;		
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;		
}
//#########################################################################################################
bool  Wifi_TcpIp_SetDisableTcpServer(uint16_t PortNumber)
{
  osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
    sprintf((char*)Wifi.TxBuffer,"AT+CIPSERVER=0,%d\r\n",PortNumber);
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		if(result == 2)
			break;		
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;	
}
//#########################################################################################################
bool  Wifi_TcpIp_SendDataUdp(uint8_t LinkId,uint16_t dataLen,uint8_t *data)
{
  osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
    if(Wifi.TcpIpMultiConnection==false)
      sprintf((char*)Wifi.TxBuffer,"AT+CIPSERVER=0\r\n");
    else
      sprintf((char*)Wifi.TxBuffer,"AT+CIPSEND=%d,%d\r\n",LinkId,dataLen);
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,">","ERROR")==false)
			break;
		if(result == 2)
			break;		
    Wifi_RxClear();
    Wifi_SendRaw(data,dataLen);
    if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;	
  
}
//#########################################################################################################
bool  Wifi_TcpIp_SendDataTcp(uint8_t LinkId,uint16_t dataLen,uint8_t *data)
{
  osSemaphoreWait(WifiSemHandle,osWaitForever);
	uint8_t result;
	bool		returnVal=false;
	do
	{
		Wifi_RxClear();
    if(Wifi.TcpIpMultiConnection==false)
      sprintf((char*)Wifi.TxBuffer,"AT+CIPSENDBUF=%d\r\n",dataLen);
    else
      sprintf((char*)Wifi.TxBuffer,"AT+CIPSENDBUF=%d,%d\r\n",LinkId,dataLen);
		if(Wifi_SendString((char*)Wifi.TxBuffer)==false)
			break;
		if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
			if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,3,">","ERROR","busy")==false)
			break;
		if(result > 1)
			break;		
    Wifi_RxClear();
    Wifi_SendRaw(data,dataLen);
    if(Wifi_WaitForString(_WIFI_WAIT_TIME_LOW,&result,2,"OK","ERROR")==false)
			break;
		returnVal=true;	
	}while(0);
	osSemaphoreRelease(WifiSemHandle);
	return returnVal;	  
}
//#########################################################################################################
*/