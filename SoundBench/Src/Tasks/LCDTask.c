// file mainTask.c

#include "main.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "../inc/my/myTasks.h"
#include "../inc/my/myLCD.h"

#define CS_UP()     HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET)
#define CS_DOWN()   HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET)

#define CLK_UP()    HAL_GPIO_WritePin(SCK_GPIO_Port, SCK_Pin, GPIO_PIN_SET)
#define CLK_DOWN()  HAL_GPIO_WritePin(SCK_GPIO_Port, SCK_Pin, GPIO_PIN_RESET)

#define MOSI_UP()   HAL_GPIO_WritePin(MOSI_GPIO_Port, MOSI_Pin, GPIO_PIN_SET)
#define MOSI_DOWN() HAL_GPIO_WritePin(MOSI_GPIO_Port, MOSI_Pin, GPIO_PIN_RESET)

#define LCDcmd(a)   softSPIWrite(0x00F80000ul | (((uint16_t)(a) << 8) & 0xF000) | (((a) << 4) & 0xF0))
#define LCDdata(a)  softSPIWrite(0x00FA0000ul | (((uint16_t)(a) << 8) & 0xF000) | (((a) << 4) & 0xF0))

static uint8_t videoBuf[(128*64)/8];

static void softSPIWrite(uint32_t data);
static void LCDWriteArray(uint8_t *data, int len);
static void LCDPixel(int x, int y, int color); // color = 0 / 1 / 2(invert)

void InitLCDTask(void)
{
    memset(videoBuf, 0, sizeof(videoBuf));
    for (int i = 0; i < sizeof(videoBuf); i++) {
        videoBuf[i] = 0;
    }
    osDelay(50);
    CS_UP();
    osDelay(1);
    LCDcmd(0x30);
    osDelay(1);
    LCDcmd(0x30);
    osDelay(1);
    LCDcmd(0x01);
    osDelay(10);
    CS_DOWN();
}
//------------------------------------------------------------------------------

void LCDupdate(void)
{
   
    static uint32_t c = 0;
    CS_UP();
    LCDcmd(0x36);
    for (uint8_t y = 0; y < 32; y++) {  
        LCDcmd(0x80 + y); 
        LCDcmd(0x80);
        LCDWriteArray(&videoBuf[0], 32);    
    }
    CS_DOWN(); 
    
    for (int i = 0; i < sizeof(videoBuf)/4; i++) {
        videoBuf[4*i+3] = c;
        videoBuf[4*i+2] = c>>8;
        videoBuf[4*i+1] = c>>16;
        videoBuf[4*i+0] = c>>24;
        
    }
    c = c << 1;
    if (!c) c = 1;
}
//------------------------------------------------------------------------------

static void LCDPixel(int x, int y, int color) // color = 0 / 1 / 2(invert)
{
    int n = 0;
    
    if (y > 31) {
        n += 16;
        
    }
   //videoBuf[] 
}
//------------------------------------------------------------------------------
    
void LCDTask(void)
{
    static int n = 0, m;
    static uint32_t timer = 0;
    if (HAL_GetTick() - timer > 1000) {
        m = n;
        n = 0;
        timer = HAL_GetTick();
    }
    n++;
    LCDupdate();
    //osDelay(1);
}
//------------------------------------------------------------------------------

static void LCDWriteArray(uint8_t *data, int len)
{
    for (int i = 0; i < len; i++) {
       LCDdata(data[i]); 
    }
}
//------------------------------------------------------------------------------

static void softSPIWrite(uint32_t data)
{
    for(int i = 0; i < 24; i++){
        CLK_DOWN();    
        if (data & (0x800000 >> i)) {
            MOSI_UP();
            MOSI_UP();
        } else {
            MOSI_DOWN();
            MOSI_DOWN();
        }
        CLK_UP();
    }  
}
//------------------------------------------------------------------------------

