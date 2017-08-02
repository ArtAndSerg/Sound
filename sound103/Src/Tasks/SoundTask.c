// SoundTask.c
#include "main.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "fatfs.h"
#include "usb_device.h"
#include "SoundTask.h"

#define SOUND_BUF_SIZE 1000

extern TIM_HandleTypeDef htim2;
extern osSemaphoreId DMAsoundSemHandle;
char SD_Path[4] = "0:\\";
FATFS fileSystem;
FIL f;
volatile unsigned short Buffer[SOUND_BUF_SIZE];
volatile int overflow = 0;

void SoundTaskInit(void)
{ 
    unsigned int n;
    FRESULT res;
    
    /* init code for FATFS */
    MX_FATFS_Init();
    f_mount(&fileSystem, SD_Path, 1);
    res = f_open(&f, "lenin.wav", FA_READ);
    xSemaphoreTake(DMAsoundSemHandle, 0);
}
//---------------------------------------------------------------------------

void SoundTask(void)
{
  unsigned int n;
  
  if (xSemaphoreTake(DMAsoundSemHandle, 1000) == pdPASS){
      if (htim2.hdma[1]->State == HAL_DMA_STATE_READY_HALF) {
          overflow = 0;
          f_read(&f, (void*)&Buffer[SOUND_BUF_SIZE/4], sizeof(unsigned short) * (SOUND_BUF_SIZE/4), &n);
          
          for (int i = 0; i < SOUND_BUF_SIZE/2; i++) {
               //if (!(i & 0x01)) Buffer[i] = Buffer[i/2 + SOUND_BUF_SIZE/4];
               //else Buffer[i] = (Buffer[i/2 + SOUND_BUF_SIZE/4] + Buffer[i/2 - 1 + SOUND_BUF_SIZE/4]) / 2; 
              Buffer[i] = Buffer[i/2 + SOUND_BUF_SIZE/4];
          }
         
          for (int i = 0; i < SOUND_BUF_SIZE/2; i++) {
               Buffer[i] =  ((signed short)Buffer[i])/64 + 512;
          }
          if (overflow) {
             osDelay(1);
          }
              
      } else {
          overflow = 0;
          f_read(&f, (void*)&Buffer[3*(SOUND_BUF_SIZE/4)], sizeof(unsigned short) * (SOUND_BUF_SIZE/4), &n);
          
          for (int i = SOUND_BUF_SIZE/2; i < SOUND_BUF_SIZE; i++) {
             // if (!(i & 0x01)) Buffer[i] = Buffer[i/2 + SOUND_BUF_SIZE/2];
              //else Buffer[i] = (Buffer[i/2 + SOUND_BUF_SIZE/2] + Buffer[i/2 - 1 + SOUND_BUF_SIZE/2]) / 2; 
              Buffer[i] = Buffer[i/2 + SOUND_BUF_SIZE/2];
          }
          
          for (int i = SOUND_BUF_SIZE/2; i < SOUND_BUF_SIZE; i++) {
               Buffer[i] =  ((signed short)Buffer[i])/64 + 512;
          }
          if (overflow) {
             osDelay(1);
          }
      }
      if (!n) {
          //osDelay(1000);
          f_close(&f);
          f_open(&f, "lenin.wav", FA_READ);
      }
  } else {
      HAL_TIM_PWM_Start_DMA(&htim2, TIM_CHANNEL_1, (uint32_t *)Buffer, SOUND_BUF_SIZE);
  }

}
//---------------------------------------------------------------------------

void sound_IRQ_DMA(void)
{
   portBASE_TYPE xTaskWoken;
   overflow = 1;
   xSemaphoreGiveFromISR(DMAsoundSemHandle, &xTaskWoken );
   if( xTaskWoken == pdTRUE) {
	   taskYIELD();
   }
}
//---------------------------------------------------------------------------
