// File: "TaskMain.c"


/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdlib.h>
#include "main.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "dallas.h"
#include "software_I2C.h"
#include "modbus.h"

extern FLASH_ProcessTypeDef pFlash;
extern IWDG_HandleTypeDef hiwdg;
extern UART_HandleTypeDef huart1;
extern osSemaphoreId mbBinarySemHandle;
extern osSemaphoreId senorsSemHandle;
extern lineOptions_t lines[LINES_MAXCOUNT];
extern osThreadId pollingTaskHandle;

uint32_t saveToFlash(uint8_t *data, int len);
uint32_t loadFromFlash(uint8_t *data, int len);

modbus_t modbus;
uint16_t temperatureBuf[LINES_MAXCOUNT][SENSORS_PER_LINE_MAXCOUNT];
uint8_t modbusBuf[SENSORS_PER_LINE_MAXCOUNT * sizeof(uint16_t) + 10];
uint16_t savedSensNum[LINES_MAXCOUNT][SENSORS_PER_LINE_MAXCOUNT]; 

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

void blinkLed1(int t)
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);
    osDelay(t);
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
}
//-----------------------------------------------------------------------------

void showError(dsResult_t errorCode, int lineNum, int sensNum) 
{
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
    HAL_IWDG_Refresh(&hiwdg); 
    
    for (int i = 0; i < 10; i++) {
        blinkLed1(10);
        osDelay(100);
    }
    osDelay(1000);
    for (int i = 0; i < (int)errorCode; i++) {
        HAL_IWDG_Refresh(&hiwdg); 
        osDelay(1000);
        blinkLed1(500);
    }
    osDelay(2000);
    for (int i = 0; i < lineNum + 1; i++) {
        HAL_IWDG_Refresh(&hiwdg); 
        osDelay(700);
        blinkLed1(200);            
    }        
    osDelay(2000);
    for (int i = 0; i < sensNum + 1; i++) {
        HAL_IWDG_Refresh(&hiwdg); 
        osDelay(700);
        blinkLed1(50);            
    }
}
//-----------------------------------------------------------------------------

int cmpFunction(const void *a, const void *b) {
     return (int)*((uint16_t*)a) - (int)*((uint16_t*)b);
}
//-----------------------------------------------------------------------------

void initTaskMain(void)
{
    uint8_t addr;
    lineOptions_t *tmpLine = NULL;
   
    modbus.dataRxSemHandle = mbBinarySemHandle;
    modbus.huart = &huart1;
    
    memset((void*)savedSensNum, 0, sizeof(savedSensNum));
    memset((void*)temperatureBuf, 0, sizeof(temperatureBuf));    
    HAL_IWDG_Refresh(&hiwdg); 
    osDelay(200);
    addr = getJumpers();
    
    if (!addr) { 
        initTaskPolling();
        for (int i = 0; i < LINES_MAXCOUNT; i++) {
            HAL_IWDG_Refresh(&hiwdg);    
            dsFindAllId(&lines[i]);
            for (int j = 0; j < lines[i].sensorsCount; j++) {
                if (dsReadTemperature(&lines[i], &lines[i].sensor[j]) != DS_ANS_OK) {
                    break;
                }
            }
            if (lines[i].lastResult != DS_ANS_OK) {
                showError(lines[i].lastResult, i, -1);
            }
        }
        NVIC_SystemReset();
    }
    if (addr == 255) {  
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET);
        initTaskPolling();
        tmpLine =  pvPortMalloc(sizeof(lineOptions_t));
        for (int rep = 0; rep < 5; rep++) {
            for (int i = 0; i < LINES_MAXCOUNT; i++) {
                HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
                HAL_IWDG_Refresh(&hiwdg); 
                *tmpLine = lines[i];
                dsFindAllId(tmpLine);
                for (int j = 0; j < tmpLine->sensorsCount; j++) {
                    if (dsReadTemperature(tmpLine, &tmpLine->sensor[j]) != DS_ANS_OK) {
                        break;
                    }
                    tmpLine->sensor[j].currentTemperature = 9999;
                    if (!rep) {
                        lines[i].sensor[j] = tmpLine->sensor[j];
                    } else {
                        if (memcmp((void*)&lines[i].sensor[j], (void*)&tmpLine->sensor[j], sizeof(sensorOptions_t)) != 0) {
                            showError(DS_ANS_BAD_SENSOR, i, -1);
                            NVIC_SystemReset();
                        }
                    }
                }
                if (lines[i].lastResult != DS_ANS_OK && lines[i].lastResult != DS_ANS_NOANS) {
                    showError(lines[i].lastResult, i, -1);
                    NVIC_SystemReset();
                }
                if (!rep) {
                    lines[i] = *tmpLine;
                } else {
                    if (lines[i].sensorsCount != tmpLine->sensorsCount) {
                        showError(DS_ANS_BAD_COUNT, i, -1);
                        NVIC_SystemReset();
                    }
                }               
            }
        }
        vPortFree(tmpLine);
        for (int i = 0; i < LINES_MAXCOUNT; i++) {
            for (int j = 0; j < lines[i].sensorsCount; j++) {
                savedSensNum[i][j] = lines[i].sensor[j].num;
            }
            qsort((void*) savedSensNum[i], lines[i].sensorsCount, sizeof(uint16_t), cmpFunction);
        }
       // dsWriteNum(&lines[0], lines[0].sensor[3].id, 22);
        saveToFlash((uint8_t*)savedSensNum, sizeof(savedSensNum));
        memset((void*)savedSensNum, 0, sizeof(savedSensNum));
        if (!loadFromFlash((uint8_t*)savedSensNum, sizeof(savedSensNum))) {
            showError(DS_ANS_CRC, 0, 0);
            NVIC_SystemReset();
        }
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET);
        while(1) {
            HAL_IWDG_Refresh(&hiwdg); 
            HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
            osDelay(100);
            HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin);
            osDelay(100);
        }       
    }
    if (!loadFromFlash((uint8_t*)savedSensNum, sizeof(savedSensNum))) {
        showError(DS_ANS_CRC, 0, 0);
        memset((void*)savedSensNum, 0, sizeof(savedSensNum));
    }
    if (!modbusInit(&modbus, addr, 9600)) {
        while(1);
    }
    for (int i = 0; i < 10; i++) {
        blinkLed1(50 - 5 * i);
        osDelay(50 + 30 * i);
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
        modbusBuf[txLen++] = modbus.addr; // address
        modbusBuf[txLen++] = modbus.rxBuf[1];  // command
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
            memcpy((void*)&modbusBuf[txLen], &modbus.rxBuf[2], 4); // register + value              
            txLen += 4;
        } else if (modbus.rxBuf[1] == 3) { //read
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
        }
        crc = modbusCrc16(modbusBuf, txLen);
        modbusBuf[txLen++] = crc & 0xFF;
        modbusBuf[txLen++] = crc >> 8;        
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
    if (i2cReadDirectly(i2cAddr, &valPrev, 1)) {
        osDelay(10);
        if (i2cReadDirectly(i2cAddr, &val, 1)) {
            if (val == valPrev) {
                //res = val;
                for (int i = 0; i < 8; i++) {
                    res = res << 1;          
                    res |= !(val & (0x01 << i));
                }    
            }
        }
    }    
    return (res);
}
//-----------------------------------------------------------------------------

#define SAVE_BASE_ADDR     (FLASH_BASE + 0x1E000)

uint32_t saveToFlash(uint8_t *data, int len)
{
    uint32_t addr = SAVE_BASE_ADDR, crc, PageError = 0;
    uint8_t *buf = data;
    FLASH_EraseInitTypeDef eraseFlash;
    
    HAL_FLASH_Unlock();
    if (pFlash.ErrorCode != HAL_FLASH_ERROR_NONE) {
        return 0;
    }
    eraseFlash.TypeErase = FLASH_TYPEERASE_PAGES;
    eraseFlash.PageAddress = addr;
    eraseFlash.NbPages = 2;
    if (HAL_FLASHEx_Erase(&eraseFlash, &PageError) != HAL_OK) {
        HAL_FLASH_Lock();
        //printf("Flash erasing ERROR at address 0x%08X", PageError);
        return 0;
    }
    if (pFlash.ErrorCode != HAL_FLASH_ERROR_NONE) {
        HAL_FLASH_Lock();
        return 0;
    }
    while (addr < SAVE_BASE_ADDR + len) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr, *((uint16_t*)buf));
        FLASH_WaitForLastOperation(10000);
        if (pFlash.ErrorCode != HAL_FLASH_ERROR_NONE) {
            break;
        }
        addr += sizeof(uint16_t);
        buf  += sizeof(uint16_t);
    }
    crc = modbusCrc16((uint8_t*)data, len);  // 7261  
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, addr, crc);  //0x0801F4B0
    FLASH_WaitForLastOperation(10000);
    HAL_FLASH_Lock(); 
    return (pFlash.ErrorCode == HAL_FLASH_ERROR_NONE);
}
//-----------------------------------------------------------------------------

uint32_t loadFromFlash(uint8_t *data, int len)
{
    uint16_t crc;
    
    memcpy((void*)data, (void*)(SAVE_BASE_ADDR), len);
    memcpy((void*)&crc, (void*)(SAVE_BASE_ADDR + len), sizeof(crc));  //0x3892
    return (crc == modbusCrc16((uint8_t*)data, len));          
}
//-----------------------------------------------------------------------------


