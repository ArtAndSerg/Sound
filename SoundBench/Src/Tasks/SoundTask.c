// file mainTask.c

#include "my/myTasks.h"
#include "my/VS1053.h"

void InitSoundTask(void)
{
    VS1053_Init();
    VS1053_thread(); 
}
//------------------------------------------------------------------------------

void SoundTask(void)
{
    
}
//------------------------------------------------------------------------------
