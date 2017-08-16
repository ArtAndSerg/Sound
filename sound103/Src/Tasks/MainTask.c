// MainTask.c

#include "main.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "fatfs.h"
#include "usb_device.h"
#include "MainTask.h"
#include "SoundTask.h"

void MainTaskInit(void)
{
    
}
//---------------------------------------------------------------------------


void MainTask(void)
{
    if (HAL_GPIO_ReadPin (JUMPER_GPIO_Port, JUMPER_Pin)) {
       /*
       playSound("snd_1000.raw"); 
       osDelay(3000);
       playSound("snd_900.raw");
       osDelay(3000);
       playSound("snd_80.raw");
       osDelay(3000);
       playSound("snd_2.raw");
       osDelay(3000);
       */
       
       playSound("lenin.raw"); 
       osDelay(3000);
       shutUp();
       osDelay(3000);
       playSound("snd_1000.raw");
       osDelay(3000);
    }
      
}
//---------------------------------------------------------------------------
