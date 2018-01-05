// WiFiTask.c
#include <string.h>
#include "main.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "WiFiTask.h"

extern UART_HandleTypeDef huart2;

char buf[200] = "AT+GMR\n\r";

void WiFiTaskInit(void)
{
    
   HAL_UART_Transmit(&huart2, "AT+GMR\r\n", strlen(buf), 1000); 
   HAL_UART_Receive(&huart2, buf, 200, 500); 
}
//----------------------------------------------------------------------------




