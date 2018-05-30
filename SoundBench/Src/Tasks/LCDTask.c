// file mainTask.c

#include "main.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "my/myTasks.h"
#include "my/myLCD.h"

#define CS_UP()     HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET)
#define CS_DOWN()   HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET)

#define CLK_UP()    HAL_GPIO_WritePin(SCK_GPIO_Port, SCK_Pin, GPIO_PIN_SET)
#define CLK_DOWN()  HAL_GPIO_WritePin(SCK_GPIO_Port, SCK_Pin, GPIO_PIN_RESET)

#define MOSI_UP()   HAL_GPIO_WritePin(MOSI_GPIO_Port, MOSI_Pin, GPIO_PIN_SET)
#define MOSI_DOWN() HAL_GPIO_WritePin(MOSI_GPIO_Port, MOSI_Pin, GPIO_PIN_RESET)

#define LCDcmd(a)   softSPIWrite(0xF80000 | (((uint16_t)a << 8) & 0xF000) | (a << 4))
#define LCDdata(a)  softSPIWrite(0xFA0000 | (((uint16_t)a << 8) & 0xF000) | (a << 4))

static uint8_t videoBuf[(128*64)/8];

static void softSPIWrite(uint32_t data);
static void LCDWriteArray(uint8_t *data, int len);

void InitLCDTask(void)
{
    memset(videoBuf, 0, sizeof(videoBuf));
    HAL_Delay(50);
    CS_UP();
    LCDcmd(0x30);
    LCDcmd(0x01);
    osDelay(11);
    LCDcmd(0x0C);
    LCDcmd(0x80);
    CS_DOWN();
}
//------------------------------------------------------------------------------

void LCDTask(void)
{
    static uint8_t s = 0;
    CS_UP();
    LCDdata(s++);
    CS_DOWN();
    osDelay(1000);
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
    int i = 24;
    while(i--){
        CLK_DOWN();
        if (data & (1 << i)) {
            MOSI_UP();
        } else {
            MOSI_DOWN();
        }
        CLK_UP();
    }    
}
//------------------------------------------------------------------------------

