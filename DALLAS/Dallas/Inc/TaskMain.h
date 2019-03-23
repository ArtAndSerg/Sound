// File: "TaskMain.h"

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TASK_MAIN_H__
#define __TASK_MAIN_H__


/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"

#define LINES_MAXCOUNT 10
#define PORT_EXPANDER_BASE_ARRR 0x70


void initTaskMain(void);
void processTaskMain(void);
unsigned short crcModbus(unsigned char* data, unsigned char length);

#endif


