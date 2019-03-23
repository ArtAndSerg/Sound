//file: "software_I2C.c"

#include "stm32f1xx_hal.h"
#include "main.h"
#include "cmsis_os.h"
#include "software_I2C.h"


#define SDA_read    HAL_GPIO_ReadPin (SDA_GPIO_Port, SDA_Pin)
#define SDA_H       HAL_GPIO_WritePin(SDA_GPIO_Port, SDA_Pin, GPIO_PIN_SET)
#define SDA_L       HAL_GPIO_WritePin(SDA_GPIO_Port, SDA_Pin, GPIO_PIN_RESET)
#define SCL_H       HAL_GPIO_WritePin(SCL_GPIO_Port, SCL_Pin, GPIO_PIN_SET)
#define SCL_L       HAL_GPIO_WritePin(SCL_GPIO_Port, SCL_Pin, GPIO_PIN_RESET)

int i2cErrorCount = 0;

static void soft_I2C_delay(void)
{
   for (volatile int i = 0; i < 10; i++);
}
//-----------------------------------------------------------------------------

static int soft_I2C_Start(void)
{
    SCL_H;
    soft_I2C_delay();
    SDA_H;
    soft_I2C_delay();
    if (!SDA_read) {
        return 0;
    }
    SDA_L;
    soft_I2C_delay();
    if (SDA_read) {
        return 0;
    }
    SCL_L;
    soft_I2C_delay();
    return 1;
}
//-----------------------------------------------------------------------------

static void soft_I2C_Stop(void)
{
    SCL_L;
    soft_I2C_delay();
    SDA_L;
    soft_I2C_delay();
    SCL_H;
    soft_I2C_delay();
    SDA_H;
    soft_I2C_delay();
    soft_I2C_delay();
}
//-----------------------------------------------------------------------------

static void soft_I2C_Ack(void)
{
    SCL_L;
    soft_I2C_delay();
    SDA_L;
    soft_I2C_delay();
    SCL_H;
    soft_I2C_delay();
    SCL_L;
    soft_I2C_delay();
}
//-----------------------------------------------------------------------------

static void soft_I2C_NoAck(void)
{
    SCL_L;
    soft_I2C_delay();
    SDA_H;
    soft_I2C_delay();
    SCL_H;
    soft_I2C_delay();
    SCL_L;
    soft_I2C_delay();
}
//-----------------------------------------------------------------------------

static int soft_I2C_WaitAck(void)
{
    int i;
    
    SCL_L;
    soft_I2C_delay();
    SDA_H;
    soft_I2C_delay();
    SCL_H;
    for (i = 0; i < 10 && SDA_read; i++) {
        soft_I2C_delay();
    }
    SCL_L;
    soft_I2C_delay();
    return (i != 10);
}
//-----------------------------------------------------------------------------

static void soft_I2C_SendByte(uint8_t byte)
{
    uint8_t i = 8;
    while (i--) {
        SCL_L;
        soft_I2C_delay();
        if (byte & 0x80) {
            SDA_H;
        }
        else {
            SDA_L;
        }
        byte <<= 1;
        soft_I2C_delay();
        SCL_H;
        soft_I2C_delay();
    }
    SCL_L;
    soft_I2C_delay();
}
//-----------------------------------------------------------------------------

static uint8_t soft_I2C_ReceiveByte(void)
{
    uint8_t i = 8;
    uint8_t byte = 0;

    SDA_H;
    while (i--) {
        byte <<= 1;
        SCL_L;
        soft_I2C_delay();
        SCL_H;
        soft_I2C_delay();
        if (SDA_read) {
            byte |= 0x01;
        }
    }
    SCL_L;
    soft_I2C_delay();
    return byte;
}
//-----------------------------------------------------------------------------
 
void i2cFlushLine(int count) {
    taskENTER_CRITICAL();
    for (int i = 0; i < count; i++) {
        soft_I2C_Stop();
    }
    taskEXIT_CRITICAL();	
}
//-----------------------------------------------------------------------------

int i2cWrite(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
    int res = 1;
    
    if (buf == NULL) {
        return 0;
    }
    taskENTER_CRITICAL();	
    if (!soft_I2C_Start()) {
        res = 0;
    }
    soft_I2C_SendByte(addr);
    if (!soft_I2C_WaitAck()) {
        i2cErrorCount++;
        res = 0;
    }
    soft_I2C_SendByte(reg);
    if (!soft_I2C_WaitAck()) {
        i2cErrorCount++;
        res = 0;
    }
    while (len--) {
        soft_I2C_SendByte(*(buf++));
        if (!soft_I2C_WaitAck()) {
            i2cErrorCount++;
            res = 0;
        }   
    }
    soft_I2C_Stop();
    taskEXIT_CRITICAL();	
    return res;
}
//-----------------------------------------------------------------------------

int i2cRead(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len)
{
    int res = 1;
    
    if (buf == NULL) {
        return 0;
    }
    taskENTER_CRITICAL();
    if (!soft_I2C_Start()) {
        res = 0;
    }
    soft_I2C_SendByte(addr);
    if (!soft_I2C_WaitAck()) {
        i2cErrorCount++;
        res = 0;
    }
    soft_I2C_SendByte(reg);
    soft_I2C_WaitAck();
    soft_I2C_Start();
    soft_I2C_SendByte(addr | 0x01);
    soft_I2C_WaitAck();
    while (len--) {
        *(buf++) = soft_I2C_ReceiveByte();
        if (!len) {
            soft_I2C_NoAck();
        }
        else {
            soft_I2C_Ack();
        }
    }
    soft_I2C_Stop();
    taskEXIT_CRITICAL();	
    return res;
}
//-----------------------------------------------------------------------------

int i2cReadDirectly(uint8_t addr, uint8_t *buf, uint8_t len)
{
    int res = 1;
    
    if (buf == NULL) {
        return 0;
    }
    taskENTER_CRITICAL();
    if (!soft_I2C_Start()) {
        res = 0;
    }
    soft_I2C_SendByte(addr | 0x01);
    if (!soft_I2C_WaitAck()) {
        res = 0;
    } else {
        while (len--) {
            *(buf++) = soft_I2C_ReceiveByte();
            //soft_I2C_Ack();
            
            if (!len) {
                soft_I2C_NoAck();
            }
            else {
                soft_I2C_Ack();
            }
            
        }
    }
    soft_I2C_Stop();
    taskEXIT_CRITICAL();	
    return res;
}
//-----------------------------------------------------------------------------

