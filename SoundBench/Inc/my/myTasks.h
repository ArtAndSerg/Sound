// file myTasks.h

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MYTASKS_H__
#define __MYTASKS_H__

#include <stdint.h>

void InitMainTask(void);
void InitSoundTask(void);
void InitGsmTask(void);
void InitKeysTask(void);
void InitLcdTask(void);

void MainTask(void);
void SoundTask(void);
void gsmTask(void);
void KeysTask(void);
void lcdTask(void);

int  myMalloc(void **buf, uint32_t size, uint32_t timeout);
int  myCalloc(void **buf, uint32_t size, uint32_t timeout);
void myFree(void **buf);

#endif