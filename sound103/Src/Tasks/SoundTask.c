// SoundTask.c
#include "main.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "fatfs.h"
#include "usbd_conf.h"
#include "usbd_msc.h"
#include "SoundTask.h"

#define SOUND_BUF_SIZE    (8*(sizeof(USBD_MSC_BOT_HandleTypeDef)/16))

extern TIM_HandleTypeDef htim2;
extern osSemaphoreId DMAsoundSemHandle, doPlayingSemHandle;
extern osThreadId SoundTaskHandle;

char SD_Path[4] = "0:\\";
FATFS fileSystem; 
FIL f;
signed short *Buffer; 
volatile int overflow = 0;

signed short ADPCMDecoder(unsigned char code);
void DecodeFrom_ADPCM_to_WAV(signed short *wav, unsigned char *adpcm, int adpcmLen);

int predsample = 0;	/* Output of ADPCM predictor */
char index = 0;		/* Index into step size table */

/* Table of index changes */
const unsigned char IndexTable[16] = {
 0xff, 0xff, 0xff, 0xff, 2, 4, 6, 8,
 0xff, 0xff, 0xff, 0xff, 2, 4, 6, 8
};

/* Quantizer step size lookup table */
const int StepSizeTable[89] = {
 7, 8, 9, 10, 11, 12, 13, 14, 16, 17,
 19, 21, 23, 25, 28, 31, 34, 37, 41, 45,
 50, 55, 60, 66, 73, 80, 88, 97, 107, 118,
 130, 143, 157, 173, 190, 209, 230, 253, 279, 307,
 337, 371, 408, 449, 494, 544, 598, 658, 724, 796,
 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066,
 2272, 2499, 2749, 3024, 3327, 3660, 4026, 4428, 4871, 5358,
 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
};

void SoundTaskInit(void)
{    
   // return;
    int s = SOUND_BUF_SIZE;
    Buffer = (signed short*)USBD_static_malloc(SOUND_BUF_SIZE);
    //osDelay(5000);
    /* init code for FATFS */
    MX_FATFS_Init();
    f_mount(&fileSystem, SD_Path, 1);
    xSemaphoreTake(DMAsoundSemHandle, 0);
    xSemaphoreTake(doPlayingSemHandle, 0);
    shutUp();
}
//---------------------------------------------------------------------------

void SoundTask(void)
{
  static unsigned int n = SOUND_BUF_SIZE/8;
  if (xSemaphoreTake(DMAsoundSemHandle, 1000) == pdPASS) {
 
      if (htim2.hdma[1]->State == HAL_DMA_STATE_READY_HALF) {
          overflow = 0;
          f_read(&f, (void*)&Buffer[(7*SOUND_BUF_SIZE)/16], SOUND_BUF_SIZE/8, &n);
          DecodeFrom_ADPCM_to_WAV(&Buffer[SOUND_BUF_SIZE/4], (unsigned char*)&Buffer[(7*n)/2], n);
          for (int i = 0; i < 2*n; i++) {
              Buffer[i<<1] = ((Buffer[i + SOUND_BUF_SIZE/4]) /64) + 512;
              Buffer[(i<<1)+1] = ((Buffer[i + SOUND_BUF_SIZE/4] + Buffer[i+1 + SOUND_BUF_SIZE/4]) / 128) + 512;
          }
          Buffer[SOUND_BUF_SIZE/2-1] = Buffer[SOUND_BUF_SIZE/2-2];
          HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, 1);
          if (overflow) {
             HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, 0);
          }
          
      } else {
          overflow = 0;
          f_read(&f, (void*)&Buffer[(15*SOUND_BUF_SIZE)/16], SOUND_BUF_SIZE/8, &n);
          DecodeFrom_ADPCM_to_WAV(&Buffer[3*(SOUND_BUF_SIZE/4)], (unsigned char*)&Buffer[(15*n)/2], n);
          for (int i = 0; i < 2*n; i++) {
              Buffer[(i<<1) + SOUND_BUF_SIZE/2]   = ((Buffer[i + (3*SOUND_BUF_SIZE)/4]) / 64) + 512;
              Buffer[(i<<1)+1 + SOUND_BUF_SIZE/2] = ((Buffer[i + (3*SOUND_BUF_SIZE)/4] + Buffer[i + 1 + (3*SOUND_BUF_SIZE)/4]) / 128) + 512;
          }
          Buffer[SOUND_BUF_SIZE-1] = Buffer[SOUND_BUF_SIZE-2];
          HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, 1);          
          if (overflow) {
             HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, 0);
          }
          
          if (n < SOUND_BUF_SIZE/8) {
          n = SOUND_BUF_SIZE/8;
          shutUp();     
          return;
      }
          
      }
   }
}
//---------------------------------------------------------------------------
 
int playSound(char *fileName)
{
    xSemaphoreTake(doPlayingSemHandle, portMAX_DELAY);
    if (f_open(&f, fileName, FA_READ) != FR_OK) {
        xSemaphoreGive(doPlayingSemHandle);
        return 0;
    }
    memset (Buffer, 0x7F, SOUND_BUF_SIZE*2);
    predsample = 0;	/* Output of ADPCM predictor */
    index = 0;		/* Index into step size table */
    xSemaphoreGive(DMAsoundSemHandle);
    htim2.hdma[1]->State = HAL_DMA_STATE_READY_HALF;
    SoundTask();
    vTaskResume(SoundTaskHandle);
    HAL_TIM_PWM_Start_DMA(&htim2, TIM_CHANNEL_1, (uint32_t *)Buffer, SOUND_BUF_SIZE);
    //xSemaphoreTake(DMAsoundSemHandle, 0);
    return 1;
}
//---------------------------------------------------------------------------

void shutUp(void){
    HAL_TIM_PWM_Stop_DMA(&htim2, TIM_CHANNEL_1);
    f_close(&f);
    xSemaphoreGive(doPlayingSemHandle);
    vTaskSuspend(SoundTaskHandle);
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

void DecodeFrom_ADPCM_to_WAV(signed short *wav, unsigned char *adpcm, int adpcmLen)
{
    for (int i = 0; i < adpcmLen*2; i++) {
       if (!(i & 0x01)) {
          wav[i] = ADPCMDecoder((adpcm[i/2] >> 4) & 0x0F);
       }
       else {
          wav[i] = ADPCMDecoder((adpcm[i/2]) & 0x0F);
       }
    }
}
//-----------------------------------------------------------------------------


signed short ADPCMDecoder(unsigned char code)
{
   int step;
   int diffq;

   step = StepSizeTable[index];

   diffq = step >> 3;
   if( code & 4 ) {
      diffq += step;
   }
   if( code & 2 ) {
      diffq += (step >> 1);
   }
   if( code & 1 ) {
      diffq += (step >> 2);
   }

   if( code & 8 ) {
      predsample -= diffq;
   } else {
      predsample += diffq;
   }

   if(predsample > 32767) {
      predsample = 32767;
   } else if (predsample < -32768) {
      predsample = -32768;
   }

   index += IndexTable[code];

   if( index < 0 ) {
      index = 0;
   }
   if( index > 88 ) {
      index = 88;
   }

   return( (unsigned short)(predsample) );
}
//------------------------------------------------------------------------------


