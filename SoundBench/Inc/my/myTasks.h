// file myTasks.h

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MYTASKS_H__
#define __MYTASKS_H__

void InitMainTask(void);
void InitSoundTask(void);
void InitGSMTask(void);
void InitKeysTask(void);
void InitLCDTask(void);

void MainTask(void);
void SoundTask(void);
void GSMTask(void);
void KeysTask(void);
void LCDTask(void);

#endif