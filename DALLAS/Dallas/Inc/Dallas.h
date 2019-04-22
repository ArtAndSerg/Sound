// File: "Dallas.h"

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DALLAS_H__
#define __DALLAS_H__

#include "stm32f1xx_hal.h"

//Internal function----------------------------
#define SENSORS_PER_LINE_MAXCOUNT   60
#define LINE_TIMEOUT                1000   // in microseconds
#define ROM_ID_SIZE                 8
#define TIME_FOR_CONVERTATION       750
#define SCRATCHPAD_SIZE             9

typedef enum
{
    DS_ANS_UNKNNOWN = 0,
    DS_ANS_OK,
    DS_ANS_NOANS,
    DS_ANS_SHORTCUT,
    DS_ANS_FAIL,
    DS_ANS_CRC,
    DS_ANS_DISABLED,
    DS_ANS_BAD_COUNT,
    DS_ANS_BAD_SENSOR,
} dsResult_t;

//iButton commands-----------------------------
#define DS_ROM_READ 	0x33
#define DS_ROM_MATCH	0x55
#define DS_ROM_SKIP	    0xCC
#define DS_ROM_ALARM	0xEC
#define DS_ROM_SEARCH	0xF0

//Scratchpad commands--------------------------
#define DS_SRC_CONVERT	0x44
#define DS_SRC_COPY	    0x48
#define DS_SRC_WRITE	0x4E
#define DS_SRC_READ	    0xBE
#define DS_SRC_RECALL	0xB8
#define DS_SRC_POWER	0xB4
//---------------------------------------------

#define DS_CONF_9_BITS   0x1F 
#define DS_CONF_10_BITS  0x3F 
#define DS_CONF_11_BITS  0x5F 
#define DS_CONF_12_BITS  0x7F 

typedef struct
{
    unsigned short num; 
    unsigned char id[ROM_ID_SIZE];
    signed short currentTemperature;
} sensorOptions_t;

typedef struct
{
    sensorOptions_t sensor[SENSORS_PER_LINE_MAXCOUNT];    
    int sensorsCount;
    int setUpTime;
    unsigned int ioPin;
    GPIO_TypeDef *ioPort;
    dsResult_t lastResult;    
    int lastDiscrepancy;
} lineOptions_t;

dsResult_t dsGetID           (lineOptions_t *line, unsigned char *val);
dsResult_t dsReadTemperature (lineOptions_t *line, sensorOptions_t *sensor);
dsResult_t dsConvertStart    (lineOptions_t *line);
dsResult_t dsFindAllId       (lineOptions_t *line);
dsResult_t dsWriteNum        (lineOptions_t *line, uint8_t *id, uint16_t num);

#endif
