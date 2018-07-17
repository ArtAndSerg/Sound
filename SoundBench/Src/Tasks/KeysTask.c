// file mainTask.c

#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "my/myTasks.h"
#include "my/mylcd.h"
#include "my/VS1053.h"

#define ADC_COUNT     6  
#define ADC_HALFBUF   5  
#define ADC_BUFSIZE   (2 * ADC_HALFBUF * ADC_COUNT)
#define VREFINT       1203

extern ADC_HandleTypeDef hadc1;
extern osSemaphoreId adcReadySemHandle;
extern osMessageQId KeysQueueHandle;

uint16_t adcBuff[ADC_BUFSIZE];
volatile uint16_t adcU, adcT, adcVdd, adcEarphones[3] = {0, 0, 0};
extern GPIO_PinState muteState;

void InitKeysTask(void)
{
    HAL_ADCEx_Calibration_Start(&hadc1);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adcBuff, ADC_BUFSIZE); 
    osDelay(100);
}
//------------------------------------------------------------------------------

void KeysTask(void)
{
    uint8_t currKey = 0;
    static uint8_t prevKey = 0;
    static int antiBounce = 0, antiBounceEarphones = 0;
    
    osDelay(10);
    xSemaphoreTake(adcReadySemHandle, portMAX_DELAY);
    if (adcU < 200) {
        currKey = '-';  // <
    } else if (adcU < 600) {
        currKey = '<';  // +
    } else if (adcU < 900) {
        currKey = '>';  //- 
    } else if (adcU < 1600) {
        currKey = '+'; //>
    } else {
        currKey = 0;
    }
    xSemaphoreGive(adcReadySemHandle);
    if (antiBounce == 3) {
        xQueueSend(KeysQueueHandle, &currKey, 1000);
    }
    if (currKey && currKey == prevKey) {
        antiBounce++;
        return;
    } else {
        antiBounce = 0;
    }
    prevKey = currKey; 
    
    if (adcEarphones[0] > 15000 || adcEarphones[1] > 15000 || adcEarphones[2] > 15000) {
        if (antiBounceEarphones < 10) {
            antiBounceEarphones++;
        }
    } else {
        antiBounceEarphones = 0;
    }
    if (antiBounceEarphones == 10) {
        muteState = GPIO_PIN_SET;
    } else {
        muteState = GPIO_PIN_RESET;
    }
}
//------------------------------------------------------------------------------

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    uint32_t sum[ADC_COUNT];
    portBASE_TYPE xTaskWoken;
    if (xSemaphoreTakeFromISR(adcReadySemHandle, &xTaskWoken ) == pdPASS) {
        if( xTaskWoken == pdTRUE) {
	        taskYIELD();
	    }
        memset((void*)sum, 0, sizeof(sum));
   
        for (int i = ADC_BUFSIZE / 2; i < ADC_BUFSIZE; i += ADC_COUNT) {
            for(int j = 0; j < ADC_COUNT; j++) {
                sum[j] += adcBuff[i + j];
            }
        }
        adcVdd = (VREFINT * 4096ul) / (sum[0] / ADC_HALFBUF);
        adcU   = sum[1] / (ADC_BUFSIZE / 6);
        adcT   = (VREFINT * sum[2]) / sum[0];  
        adcEarphones[0] = sum[3];
        adcEarphones[1] = sum[4];
        adcEarphones[2] = sum[5];
        xSemaphoreGiveFromISR(adcReadySemHandle, &xTaskWoken );
	    if( xTaskWoken == pdTRUE) {
	        taskYIELD();
	    } 
    }
}
//-----------------------------------------------------------------------------

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
    uint32_t sum[ADC_COUNT];
    portBASE_TYPE xTaskWoken;
    if (xSemaphoreTakeFromISR(adcReadySemHandle, &xTaskWoken ) == pdPASS) {
        if( xTaskWoken == pdTRUE) {
	        taskYIELD();
	    }
        memset((void*)sum, 0, sizeof(sum));
   
        for (int i = 0; i < ADC_BUFSIZE / 2; i += ADC_COUNT) {
            for(int j = 0; j < ADC_COUNT; j++) {
                sum[j] += adcBuff[i + j];
            }
        }
        adcVdd = (VREFINT * 4096ul) / (sum[0] / ADC_HALFBUF);
        adcU   = sum[1] / (ADC_BUFSIZE / 6);
        adcT   = (VREFINT * sum[2]) / sum[0];  
        adcEarphones[0] = sum[3];
        adcEarphones[1] = sum[4];
        adcEarphones[2] = sum[5];
        xSemaphoreGiveFromISR(adcReadySemHandle, &xTaskWoken );
	    if( xTaskWoken == pdTRUE) {
	        taskYIELD();
	    } 
    }
}
//-----------------------------------------------------------------------------

