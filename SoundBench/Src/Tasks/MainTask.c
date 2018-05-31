// file mainTask.c

#include <string.h>
#include <stdio.h>
#include "my/myTasks.h"
#include "cmsis_os.h"

void InitMainTask(void)
{
}
//------------------------------------------------------------------------------

void MainTask(void)
{
}
//------------------------------------------------------------------------------


int myMalloc(uint8_t **buf, uint32_t size, uint32_t timeout) {
    int attempts = timeout / 100;
    do {
       *buf = pvPortMalloc(size);
       if (*buf != NULL) {
           return 1;
       } else {
           osDelay(100);
       }
    } while (attempts--);
    printf("\nERROR! Can't allocate %d bytes of RAM memory!\n");
    return 0;
}
//-----------------------------------------------------------------------------

int myCalloc(uint8_t **buf, uint32_t size, uint32_t timeout) {
    if (!myMalloc(buf, size, timeout)) {
        return 0;
    } 
    memset(*buf, 0, size);
    return 1;
}
//-----------------------------------------------------------------------------

void myFree(uint8_t **buf) {
    vPortFree(*buf);
    *buf = NULL;
}
//-----------------------------------------------------------------------------
