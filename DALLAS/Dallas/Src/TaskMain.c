// File: "TaskMain.c"


/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "main.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "dallas.h"
#include "software_I2C.h"
#include "modbus.h"

extern IWDG_HandleTypeDef hiwdg;
extern UART_HandleTypeDef huart1;
extern osSemaphoreId mbBinarySemHandle;
extern osSemaphoreId senorsSemHandle;
extern lineOptions_t lines[LINES_MAXCOUNT];

modbus_t modbus;
uint16_t temperatureBuf[LINES_MAXCOUNT][SENSORS_PER_LINE_MAXCOUNT];
uint8_t modbusBuf[SENSORS_PER_LINE_MAXCOUNT * sizeof(uint16_t) + 10];

static uint8_t getJumpers(void);

void modbusSetTxMode(bool txMode)          // Set RE/DE pin of MAX485 or similar IC, if needed.
{
    if (txMode) {
        HAL_GPIO_WritePin(RE_DE_GPIO_Port, RE_DE_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(RE_DE_GPIO_Port, RE_DE_Pin, GPIO_PIN_RESET);
    }
}
//-----------------------------------------------------------------------------

void initTaskMain(void)
{
    uint8_t addr;
   
    modbus.dataRxSemHandle = mbBinarySemHandle;
    modbus.huart = &huart1;
    HAL_IWDG_Refresh(&hiwdg); 
    osDelay(200);
    addr = getJumpers();
    if (!addr) {
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
        osDelay(50);
        NVIC_SystemReset();
    }
    addr = 255 - addr;
    memset((void*)temperatureBuf, 0, sizeof(temperatureBuf));
    if (!modbusInit(&modbus, addr, 9600)) {
        while(1);
    }
    modbusReceiveStart(&modbus);
}
//-----------------------------------------------------------------------------

void processTaskMain(void)
{
    int txLen = 0, baseNum, count;
    uint16_t crc;
    static int currLine = 0;
    modbusResult_t mbRes;
    
 // 01 03 00 00 00 02 C4 0B
 // 0  1  2  3  4  5  6  7   
    
    HAL_IWDG_Refresh(&hiwdg); 
    HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
    mbRes = modbusDataWait(&modbus, 1000);
    if (mbRes == MB_OK) {
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);        
        modbusBuf[txLen++] = modbus.addr;
        if (modbus.rxBuf[1] == 6) { // write command
            if (modbus.rxBuf[3] == 0) { // register
                if (modbus.rxBuf[5] < LINES_MAXCOUNT) {
                    currLine = modbus.rxBuf[5] + 1; 
                } else {
                    currLine = 0;
                }
            }
            if (modbus.rxBuf[3] == 1 && modbus.rxBuf[5] == 0) {  // stop
                currLine = 0;
            }
            memcpy((void*)&modbusBuf[1], &modbus.rxBuf[1], 7);              
            txLen += 7;
        } else if (modbus.rxBuf[1] == 3) { //read
            modbusBuf[txLen++] = 3;
            if (modbus.rxBuf[3] == 0) { // register
                modbusBuf[txLen++] = 4;  // bytes count
                modbusBuf[txLen++] = 0;  // data...
                modbusBuf[txLen++] = 0;
                modbusBuf[txLen++] = 0;
                modbusBuf[txLen++] = 2;
            } else {
                baseNum = modbus.rxBuf[3] - 2;
                count = modbus.rxBuf[5];
                //count = SENSORS_PER_LINE_MAXCOUNT;
                if (count > SENSORS_PER_LINE_MAXCOUNT) {
                    count = SENSORS_PER_LINE_MAXCOUNT;
                }
                if (currLine) {
                    modbusBuf[txLen++] = count * sizeof(uint16_t);  // bytes count
                    osSemaphoreWait(senorsSemHandle, osWaitForever);
                    for (int i = baseNum; i < baseNum + count; i++) {
                        modbusBuf[txLen++] = temperatureBuf[currLine - 1][i] >> 8;
                        modbusBuf[txLen++] = temperatureBuf[currLine - 1][i] & 0xFF;        
                    }
                    osSemaphoreRelease(senorsSemHandle);
                }
            }
            crc = modbusCrc16(modbusBuf, txLen);
            modbusBuf[txLen++] = crc & 0xFF;
            modbusBuf[txLen++] = crc >> 8;
        }            
        modbusSend(&modbus, modbusBuf, txLen);
        modbusDataWait(&modbus, osWaitForever);
       
    } else if (mbRes != MB_TIMEOUT) {
        modbusReceiveStart(&modbus);
    }
    
    /*
    
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
    modbusDataWait(&modbus, 500);
    //modbusReceiveStart(&modbus);
    //osDelay(3000);*/
}
//-----------------------------------------------------------------------------

static uint8_t getJumpers(void)
{
    uint8_t res = 0, val, valPrev;  
    static uint8_t i2cAddr = 0;  
    
    if (!i2cAddr) {
        for (int i = 0; i < 8; i++) {
            i2cAddr = PCF8574_BASE_ARRR | i;
            if (i2cReadDirectly(i2cAddr, &val, 1)) {
                break;
            }
            i2cAddr = PCF8574A_BASE_ARRR | i;
            if (i2cReadDirectly(i2cAddr, &val, 1)) {
                break;
            }
        }
    }
    osDelay(10);
    if (i2cReadDirectly(i2cAddr, &val, 1)) {
        osDelay(10);
        if (i2cReadDirectly(i2cAddr, &valPrev, 1)) {
            if (val == valPrev) {
                res = val;
                /*
                for (int i = 0; i < 8; i++) {
                    res = res << 1;          
                    res |= !(val & (0x01 << i));
                }
                */
            }
        }
    }    
    return res;
}
//-----------------------------------------------------------------------------

