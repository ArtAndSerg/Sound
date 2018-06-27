// file mainTask.c

#include <string.h>
#include <stdio.h>
#include "fatfs.h"
#include "main.h"
#include "cmsis_os.h"
#include "my/myTasks.h"
#include "my/mylcd.h"
#include "my/sdCard.h"
#include "my/VS1053.h"

extern SPI_HandleTypeDef hspi1;
extern osMessageQId KeysQueueHandle;

#define ITEM_HEIGHT 10
#define ITEMS_MAXCOUNT 100
#define BUF_SIZE    (2048)//(50 * VS1053_MAX_TRANSFER_SIZE)

char *currPath = NULL, *currFileName = NULL;
int  currItem,  itemsCount;
FILINFO currFile;

void dirToDisplay(void);
int  executeFile(char *fileName);
void stopFile(void);
void showError(char *text);

void InitMainTask(void)
{
    while (1) {
        memset((void*)&currFile, 0, sizeof(FILINFO));
        myFree((void**)&currPath);
        myFree((void**)&currFileName);
        currItem = 0;
        itemsCount = 0;
        osDelay(500); 
        disk_initialize(USERFatFS.drv);
        lcdClearAll();
        lcdUpdate();
        if (myCalloc((void**)&currPath, 3 * (_MAX_LFN + 1), 500) && myCalloc((void**)&currFileName, _MAX_LFN + 1, 500)) {
                currFile.lfsize = _MAX_LFN + 1;
                currFile.lfname = currFileName;
                if (f_mount(&USERFatFS,(TCHAR const*)USERPath, 0) == FR_OK) {
                    break;
                } else {
                    showError(" карты памяти"); 
                }
        }
    }
    dirToDisplay();
}
//------------------------------------------------------------------------------

void showError(char *text)
{
   char dummy;
   lcdClearAll();
   lcdPrintf(1, 10, clWhite, clBlack, "В Н И М А Н И Е!");
   lcdPrintf(1, 30, clWhite, clBlack, "    Ошибка      ");
   lcdPrintf(1, 40, clWhite, clBlack, "%s!", text);       
   lcdUpdate();
   xQueueReceive(KeysQueueHandle, &dummy, 4000); 
   lcdClearAll();
   lcdUpdate();
}
//------------------------------------------------------------------------------

void dirToDisplay(void)
{
    FRESULT res = FR_OK;
    DIR dir;
    FILINFO fno;
    int n = 0, y;
    itemsCount = 0;
    
    fno.lfsize = _MAX_LFN + 1;
    if (!myMalloc((void**)&fno.lfname, fno.lfsize, 500)) {
        return;
    }
    if ((res = f_opendir(&dir, currPath)) != FR_OK) {                           
        showError("открытия папки");
        myFree((void**)&fno.lfname);
        return;
    }    
    lcdClearAll();
    while (res == FR_OK) {
        res = f_readdir(&dir, &fno);                  
        if (res != FR_OK || fno.fname[0] == 0) {       // Break on error or end of dir 
            f_closedir(&dir);    
            break;
        }
        if (fno.fattrib & (AM_HID | AM_SYS)) {
            continue;
        }
        
        if (n >= currItem - (currItem % 6) && n < currItem - (currItem % 6) + 6) {
            y = n % 6;
            lcdPrintf(2, 2 + y * ITEM_HEIGHT, clWhite, clBlack, fno.lfname);
            if (n == currItem) {
                lcdRectangle(0, y * ITEM_HEIGHT, 125, (y + 1) * (ITEM_HEIGHT),  clWhite, clNone, 1);
                currFile = fno;
                currFile.lfname = currFileName;
                memcpy((void*)currFile.lfname, (void*)fno.lfname, fno.lfsize);
            }
        }
        n++;
    }            
    myFree((void**)&fno.lfname);
    lcdUpdate();
    itemsCount = n;
}
//------------------------------------------------------------------------------

void MainTask(void)
{
    char key;
    int playNext;      
    
    if (xQueueReceive(KeysQueueHandle, &key, 10000) != pdPASS) {
        lcdClearAll();
        while (xQueueReceive(KeysQueueHandle, &key, 150) != pdPASS) {
           lcdScreenSaver();
        }
    }
    switch (key) {
        case '<' :  
            if (strchr(currPath, '/') != NULL) {
                *strchr(currPath, '/') = '\0';
            }
            currItem = 0;            
            break;
            
        case '>' : 
            if (currFile.fattrib & AM_DIR) {
                strcat(currPath, "/");
                strcat(currPath, currFileName);
                currItem = 0;                            
                dirToDisplay();
            } else {
                do {
                    dirToDisplay();
                    strcat(currPath, "/");
                    strcat(currPath, currFileName);
                    playNext = executeFile(currPath);
                    *strrchr(currPath, '/') = '\0';
                    if (currItem >= itemsCount) {
                        currItem = 0;
                    }
                } while(playNext);
            }
            break; 
        
        case '+':  
             currItem++;
             break;
          case '-' :  
            currItem--;
            break;
     }
     if (currItem >= itemsCount) {
        currItem = 0; 
     }
     if (currItem < 0) {
        currItem = itemsCount - 1; 
     }                    
     dirToDisplay();            
}
//------------------------------------------------------------------------------

int executeFile(char *fileName)
{
    static uint8_t volume = 60;
    uint8_t *buf;
    uint32_t n;
    char key;
    int playNext = 0;  
    
    if (VS1053_play() != VS1053_OK) {
        showError("декодера");
        return 0;
    }
    if (!myMalloc((void**)&buf, BUF_SIZE, 1000)) {
        return 0;
    }                     
    
    if(f_open(&USERFile, fileName, FA_READ) != FR_OK) { 
        showError("открытия файла");
        myFree((void**)&buf);
        return 1;
    }
    
    VS1053_setVolume(volume);
    do {
        f_read(&USERFile, buf, BUF_SIZE, &n);
        if (!n) {
             currItem++;
             playNext = 1;
        }
        if (xQueueReceive(KeysQueueHandle, &key, 10) == pdPASS) {
            switch(key) {
              case '-': 
                 if (volume > 9) {
                     volume -= 10;
                 }
                 VS1053_setVolume(volume);
                 break;
              case '+': 
                 if (volume < 255-10) {
                     volume += 10;
                 }
                 VS1053_setVolume(volume);
                 break;  
              case '<':
                 n = 0;
                 break;
              case '>':
                 n = 0;
                 currItem++;
                 playNext = 1;
                 break;
            }
        }
        if (VS1053_addData(buf, n) == VS1053_ERROR) {
            showError("воспроизведения");
            break;
        }        
    } while(n);
    myFree((void**)&buf);
    f_close(&USERFile);
    return playNext;
}
//------------------------------------------------------------------------------


int myMalloc(void **buf, uint32_t size, uint32_t timeout) {
    int attempts = timeout / 100;
    char str[16];
    
    do {
       *buf = pvPortMalloc(size);
       if (*buf != NULL) {
           return 1;
       } else {
           osDelay(100);
       }
    } while (attempts--);
    printf("\nERROR! Can't allocate %d bytes of RAM memory!\n", size);
    
    sprintf(str, "malloc(%d)", size);
    showError(str);
    return 0;
}
//-----------------------------------------------------------------------------

int myCalloc(void **buf, uint32_t size, uint32_t timeout) {
    if (!myMalloc(buf, size, timeout)) {
        return 0;
    } 
    memset(*buf, 0, size);
    return 1;
}
//-----------------------------------------------------------------------------

void myFree(void **buf) {
    if (*buf != NULL)
    {
        vPortFree(*buf);
        *buf = NULL;
    }
}
//-----------------------------------------------------------------------------
