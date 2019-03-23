//file: "software_I2C.h"

#ifndef __SOFTWARE_soft_I2C_H__
#define __SOFTWARE_soft_I2C_H__

#include <stdint.h>

void i2cFlushLine(int count);
int i2cWrite(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);
int i2cRead(uint8_t addr, uint8_t reg, uint8_t *buf, uint8_t len);
int i2cReadDirectly(uint8_t addr, uint8_t *buf, uint8_t len);
#endif