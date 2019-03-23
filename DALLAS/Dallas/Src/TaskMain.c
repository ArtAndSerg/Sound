// File: "TaskMain.c"


/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "main.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "dallas.h"
#include "TaskMain.h"
#include "software_I2C.h"
#include "modbus.h"

extern IWDG_HandleTypeDef hiwdg;
extern TIM_HandleTypeDef htim3;
extern UART_HandleTypeDef huart1;
extern osSemaphoreId mbBinarySemHandle;

lineOptions_t lines[LINES_MAXCOUNT];
modbus_t modbus;
uint8_t txBuf[62];

static uint8_t getJumpers(void);

void modbusSetTxMode(bool txMode)          // Set RE/DE pin of MAX485 or similar IC, if needed.
{
    if (txMode) {
        HAL_GPIO_WritePin(RE_DE_GPIO_Port, RE_DE_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(RE_DE_GPIO_Port, RE_DE_Pin, GPIO_PIN_RESET);
    }
}

void initTaskMain(void)
{
    modbus.dataRxSemHandle = mbBinarySemHandle;
    modbus.huart = &huart1;
    HAL_HalfDuplex_EnableReceiver(&huart1);
    if (!modbusInit(&modbus, getJumpers(), 9600)) {
        while(1);
    }
    modbusReceiveStart(&modbus);
    
    memset((void*)lines, 0, sizeof(lines));
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
        lines[i].sensorsCount = SENSORS_PER_LINE_MAXCOUNT;
    }
}
//-----------------------------------------------------------------------------

void processTaskMain(void)
{
    static  uint8_t id[8];
    dsResult_t res;
    static uint32_t timer, t, addr;
    
    HAL_IWDG_Refresh(&hiwdg);
    timer = HAL_GetTick();
    res = dsFindAllId(&lines[0]);
    t = HAL_GetTick() - timer;
    //res = dsGetID(&lines[0], lines[0].sensor[0].id);
    dsConvertStart(&lines[0]);
    osDelay(TIME_FOR_CONVERTATION);
    timer = HAL_GetTick();
    for (int i = 0; i < lines[0].sensorsCount; i++) {
        lines[0].lastResult = dsReadTemperature(&lines[0], &lines[0].sensor[i]);
        if (lines[0].lastResult != DS_ANS_OK) {
            break;
        }
    }
    t = HAL_GetTick() - timer;
    
    
    switch (modbusDataWait(&modbus, 3000)){
      case MB_ERR_CRC:
        modbusSend(&modbus, "ERROR_CRC!", sizeof("ERROR_CRC!"));
        break;
      case MB_ERR_FRAME:
        modbusSend(&modbus, "ERROR_FRAME!", sizeof("ERROR_FRAME!"));
        break;
      case MB_ERR_OTHER: 
        modbusSend(&modbus, "ERROR_OTHER!", sizeof("ERROR_OTHER!"));
        break;
      case MB_ERR_OVERFLOW:
        modbusSend(&modbus, "ERROR_OVF!", sizeof("ERROR_OVF!"));
        break;
      case MB_OK:
        for (int i = 0; i < 5; i++) {
            id[i] = modbus.rxBuf[i] + 1;
        }
        modbusSend(&modbus, id, 5);
        break;
      case MB_TIMEOUT:
        return;
    }
    modbusDataWait(&modbus, 1000);
    modbusReceiveStart(&modbus);
    //osDelay(3000);
}
//-----------------------------------------------------------------------------

static uint8_t getJumpers(void)
{
    uint8_t res = 0, tmp;  
      
    if (i2cReadDirectly(PORT_EXPANDER_BASE_ARRR, &tmp, 1)) {
        for (int i = 0; i < 8; i++) {
            res = res << 1;          
            res |= !(tmp & (0x01 << i));
        }
    }    
    return res;
}
//-----------------------------------------------------------------------------

