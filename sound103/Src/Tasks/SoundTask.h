// SoundTask.h
#ifndef __SOUND_TASK_H
#define __SOUND_TASK_H

void SoundTaskInit(void);
void SoundTask(void);
void sound_IRQ_DMA(void);
int  playSound(char *fileName);
void shutUp(void);
#endif

