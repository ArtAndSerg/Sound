// File: "TaskMain.c"


/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "main.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "dallas.h"
#include "TaskMain.h"

extern IWDG_HandleTypeDef hiwdg;
lineOptions_t lines[LINES_MAXCOUNT];

void initTaskMain(void)
{
    memset((void*)lines, 0, sizeof(lines));
    
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
        lines[i].count = SENSORS_PER_LINE_MAXCOUNT;
    }
}
//-----------------------------------------------------------------------------

void processTaskMain(void)
{
   static  uint8_t id[8];
    
    HAL_IWDG_Refresh(&hiwdg);
    dsGetID(&lines[0], id);
    osDelay(500);
}
//-----------------------------------------------------------------------------
