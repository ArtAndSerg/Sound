// File: "Dallas.h"

#include "stm32f1xx_hal.h"

//Internal function----------------------------
#define SENSORS_PER_LINE_MAXCOUNT 60
#define LINES_MAXCOUNT 10

typedef enum
{
    DS_ANS_OK = 0,
    DS_ANS_FAIL,
    DS_ANS_SHORTCUT,
    DS_ANS_NOANS,
    DS_ANS_EMPTY
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

typedef struct
{
    dsResult_t lastResult;
    int count;
    int setUpTime;
    unsigned char id[SENSORS_PER_LINE_MAXCOUNT][8];
    unsigned int ioPin;
    GPIO_TypeDef *ioPort;
} lineOptions_t;

dsResult_t DSGetID          (lineOptions_t *line, unsigned char *val);
dsResult_t DSGetTemperature (lineOptions_t *line, short *val, unsigned char *id);
dsResult_t DSConvertStart   (lineOptions_t *line);
dsResult_t DSFindAllId      (lineOptions_t *line);