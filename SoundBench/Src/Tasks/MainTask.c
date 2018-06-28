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
    memset((void*)&currFile, 0, sizeof(FILINFO));        
    while (1) {
        myFree((void**)&currFile.lfname);
        myFree((void**)&currPath);
        myFree((void**)&currFileName);
        currItem = 0;
        itemsCount = 0;
        osDelay(500); 
        disk_initialize(USERFatFS.drv);
        lcdClearAll();
        lcdUpdate();
        if (myCalloc((void**)&currPath, 3 * (_MAX_LFN + 1), 500) && myCalloc((void**)&currFileName, _MAX_LFN + 1, 500) && myCalloc((void**)&currFile.lfname, _MAX_LFN + 1, 500)) {
                currFile.lfsize = _MAX_LFN + 1;
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
    char *tmp;
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
            if (n == currItem) {
                tmp = currFile.lfname;
                currFile = fno;
                currFile.lfname = tmp;
                memcpy((void*)currFile.lfname, (void*)fno.lfname,     fno.lfsize);                
            }
            
            tmp = strrchr(fno.lfname, '.');
            if (tmp != NULL) {
                *tmp = '\0';
            }
            y = n % 6;
            lcdPrintf(2, 2 + y * ITEM_HEIGHT, clWhite, clBlack, &fno.lfname[3]);            
            if (n == currItem) {
                memcpy((void*)currFileName,    (void*)&fno.lfname[3], fno.lfsize - 3);
                lcdRectangle(0, y * ITEM_HEIGHT, 127, (y + 1) * (ITEM_HEIGHT),  clWhite, clNone, 1);
            }
        }
        n++;
    }            
    myFree((void**)&fno.lfname);
    lcdUpdate();
    itemsCount = n;
}
//------------------------------------------------------------------------------

void scrollCurrentItem(int pos) 
{
    static int x0 = 0, delay = 0;
    int y = currItem % 6;
    int width;
    
    if (delay) {
        delay--;
    }
    if (strlen(currFileName) > 16 && !delay) {
        if (!x0) {
          delay = 10;
        }
        x0++;
        if (!pos) {
            x0 = 1;
        }
        width = lcdPrintf(2 - x0, 2 + y * ITEM_HEIGHT, clWhite, clBlack, currFileName);            
        if (width < LCD_WITDTH - 8) {
            x0 = 0; 
        }
        lcdRectangle(0, y * ITEM_HEIGHT, 127, (y + 1) * (ITEM_HEIGHT),  clWhite, clNone, 1);
        lcdUpdate();
    }
}
//------------------------------------------------------------------------------

void MainTask(void)
{
    char key;
    int playNext;
    uint32_t timer;
        
    if (xQueueReceive(KeysQueueHandle, &key, 1000) != pdPASS) {
        for (timer = 0; timer < 100 && xQueueReceive(KeysQueueHandle, &key, 200) != pdPASS; timer++) {
            scrollCurrentItem(timer);
        }
        if (timer == 100) {
            lcdClearAll();
            while (xQueueReceive(KeysQueueHandle, &key, 150) != pdPASS) {
                lcdScreenSaver();
            }
        }
    }
    switch (key) {
        case '<' :  
            if (strchr(currPath, '/') != NULL) {
                *strrchr(currPath, '/') = '\0';
            }
            currItem = 0;            
            break;
            
        case '>' : 
            if (currFile.fattrib & AM_DIR) {
                strcat(currPath, "/");
                strcat(currPath, currFile.lfname);
                currItem = 0;                            
                dirToDisplay();
            } else {
                do {
                    dirToDisplay();
                    strcat(currPath, "/");
                    strcat(currPath, currFile.lfname);
                    playNext = executeFile(currPath);
                    *strrchr(currPath, '/') = '\0';
                    if (currItem >= itemsCount) {
                        currItem = 0;
                        break;
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
    int num = 0;
    int pos = 0;
    char str[10];
    static int volume = 50;
    uint8_t *buf;
    uint32_t n;
    char key;
    int playNext = 0;  
    int witdh;
    
    memset(str, 0, sizeof(str));
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
    lcdClearAll();
    witdh = lcdPrintf(0, 0, clWhite, clBlack, currFileName);            
    do {
        f_read(&USERFile, buf, BUF_SIZE, &n);
        if ((num & 0x03) == 0 || key) {
            if ((num & 0x07) == 0) {
                pos++;    
            }    
            lcdClearAll();
            lcdPrintf(1 - pos % witdh, 10, clWhite, clBlack, currFileName);            
            lcdPrintf(witdh + 32 - pos % witdh, 10, clWhite, clBlack, currFileName);            
            lcdRectangle(1, 30, 125, 38,  clWhite, clNone, 1);
            lcdRectangle(3, 32, 12300 / ((USERFile.fsize * 100) / USERFile.fptr), 36,  clWhite, clWhite, 0);
            sprintf(str, "%d%%", volume);
            lcdPrintf(10, 52, clWhite, clBlack, str);            
            for(int i = 0; i < volume/4 + 4; i++) {
                for(int j = 0; j < i/2; j++) {
                   lcdPixel(50 + 2*i,  56-j, clWhite);
                }
            }            
            lcdUpdate();
        }
        num++;               
        if (!n) {
             currItem++;
             playNext = 1;
        }
        key = 0;
        if (xQueueReceive(KeysQueueHandle, &key, 10) == pdPASS) {
            switch(key) {
              case '+': 
                 if (volume > 5) {
                     volume -= 5;
                 }
                 VS1053_setVolume(volume);
                 break;
              case '-': 
                 if (volume < 100) {
                     volume += 5;
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
