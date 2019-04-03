// File: "TaskPolling.c"

#include <string.h>
#include "main.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "dallas.h"

extern uint16_t temperatureBuf[LINES_MAXCOUNT][SENSORS_PER_LINE_MAXCOUNT];
lineOptions_t lines[LINES_MAXCOUNT];
extern osSemaphoreId senorsSemHandle;
extern IWDG_HandleTypeDef hiwdg;

void initTaskPolling(void)
{
    memset((void*)lines, 0, sizeof(lines));
    memset((void*)lines, 0, sizeof(lines));
    
    for (int i = 0; i < LINES_MAXCOUNT; i++) {
        for (int j = 0; j < SENSORS_PER_LINE_MAXCOUNT; j++) {
            lines[i].sensor[j].currentTemperature = 32767;
        }
    }
    
    lines[0].ioPin = U5_Pin;
    lines[0].ioPort = U5_GPIO_Port;

    lines[1].ioPin = U6_Pin;
    lines[1].ioPort = U6_GPIO_Port;
    
    lines[2].ioPin = U7_Pin;
    lines[2].ioPort = U7_GPIO_Port;
    
    lines[3].ioPin = U8_Pin;
    lines[3].ioPort = U8_GPIO_Port;
    
    lines[4].ioPin = U9_Pin;
    lines[4].ioPort = U9_GPIO_Port;
    
    lines[5].ioPin = U10_Pin;
    lines[5].ioPort = U10_GPIO_Port;
    
    lines[6].ioPin = U11_Pin;
    lines[6].ioPort = U11_GPIO_Port;
    
    lines[7].ioPin = U12_Pin;
    lines[7].ioPort = U12_GPIO_Port;
    
    lines[8].ioPin = U13_Pin;
    lines[8].ioPort = U13_GPIO_Port;
    
    lines[9].ioPin = U14_Pin;
    lines[9].ioPort = U14_GPIO_Port;   
    
    for (int i = 0; i < LINES_MAXCOUNT; i++) {
        dsFindAllId(&lines[i]);
    }
    
}
//----------------------------------------------------------------------------

void processTaskPolling(void)
{       
    HAL_IWDG_Refresh(&hiwdg);    
   
    for (int i = 0; i < LINES_MAXCOUNT; i++) {
        dsFindAllId(&lines[i]);
    }
    
    for (int i = 0; i < LINES_MAXCOUNT; i++) {
        if (lines[i].sensorsCount) {
            dsConvertStart(&lines[i]);
        } else {          
            for (int j = 0; j < SENSORS_PER_LINE_MAXCOUNT; j++) {
                lines[i].sensor[j].currentTemperature = 9999;
            }
        }
    }
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    osDelay(TIME_FOR_CONVERTATION);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    for (int i = 0; i < LINES_MAXCOUNT; i++) {
        for (int j = 0; j < lines[i].sensorsCount; j++) {
            if (dsReadTemperature(&lines[i], &lines[i].sensor[j]) != DS_ANS_OK) {
                lines[i].sensor[j].currentTemperature = 9999;
            }
        }        
    }
    osSemaphoreWait(senorsSemHandle, osWaitForever);
    for (int i = 0; i < LINES_MAXCOUNT; i++) {
        for (int j = 0; j < SENSORS_PER_LINE_MAXCOUNT; j++) {
            temperatureBuf[i][j] = 9999;
        }
        for (int j = 0; j < SENSORS_PER_LINE_MAXCOUNT; j++) {
            if (lines[i].sensor[j].num && lines[i].sensor[j].num <= SENSORS_PER_LINE_MAXCOUNT) {
                temperatureBuf[i][lines[i].sensor[j].num - 1] = lines[i].sensor[j].currentTemperature; 
            }
        }        
    }
    osSemaphoreRelease(senorsSemHandle);
}
//----------------------------------------------------------------------------

