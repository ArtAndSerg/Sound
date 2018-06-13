// file mainTask.c

#include "my/myTasks.h"
#include "my/VS1053.h"

void InitSoundTask(void)
{
    VS1053_Init();
}
//------------------------------------------------------------------------------

void SoundTask(void)
{
    VS1053_thread();     
}
//------------------------------------------------------------------------------
