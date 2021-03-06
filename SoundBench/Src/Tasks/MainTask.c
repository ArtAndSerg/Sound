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

extern UART_HandleTypeDef huart1;

char *currPath = NULL, *currFileName = NULL;
int  currItem,  itemsCount;
GPIO_PinState muteState = GPIO_PIN_SET;
FILINFO currFile;

void dirToDisplay(void);
int  executeFile(char *fileName);
void stopFile(void);
void showError(char *text, int errCode);
int InitFAT(void);
void scrollCurrentItem(int pos);
static char sstr[20];

void InitMainTask(void)
{
    /*
    HAL_UART_Transmit(&huart1, "AT\r\n", 4, 1000);
    osDelay(1000);
    HAL_UART_Transmit(&huart1, "AT\r\n", 4, 1000);
    osDelay(1000);
    HAL_UART_Transmit(&huart1, "AT\r\n", 4, 1000);
    osDelay(1000);
    HAL_UART_Transmit(&huart1, "AT+IPR=115200\r\n", strlen("AT+IPR=115200\r\n"), 10000);
    osDelay(1000);
    HAL_UART_Transmit(&huart1, "AT&W\r\n", 6, 1000);
    */
   
    //while (strstr(sstr, "\r\nRDY\r\n\r\n") == NULL) {
    while (strstr(sstr, "NORMAL POWER") == NULL) {
        HAL_GPIO_WritePin(POWER_KEY_GPIO_Port, POWER_KEY_Pin, GPIO_PIN_RESET);
        osDelay(500);
        lcdPrintf(1, 10, clWhite, clBlack, "������ �������...");       
        lcdUpdate();
        osDelay(500);
        lcdPrintf(1, 30, clWhite, clBlack, "  ����������");
        lcdPrintf(1, 40, clWhite, clBlack, "  ���������.");
        lcdUpdate();
        osDelay(1000);
        HAL_GPIO_WritePin(POWER_KEY_GPIO_Port, POWER_KEY_Pin, GPIO_PIN_SET);
        HAL_UART_Receive(&huart1, sstr, 19, 2000);
        osDelay(1000);
        lcdClearAll();
        lcdUpdate();
    }
    
    memset((void*)&currFile, 0, sizeof(FILINFO));        
    currItem = 0;
    itemsCount = 0;
    osDelay(500); 
    if (myCalloc((void**)&currPath, 3 * (_MAX_LFN + 1),  500) && 
        myCalloc((void**)&currFileName,  _MAX_LFN + 1,   500) && 
        myCalloc((void**)&currFile.lfname, _MAX_LFN + 1, 500)) {
            while(!InitFAT()) {
                osDelay(100); 
            }
    } else {
         _Error_Handler(__FILE__, __LINE__);
    }
    
    
}
//------------------------------------------------------------------------------

void MainTask(void)
{
    char key;
    int playNext;
    uint32_t timer;
    
    dirToDisplay();    
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
    //dirToDisplay();            
}
//------------------------------------------------------------------------------

int InitFAT(void) 
{
    static int tryCount = 0;
    FRESULT res;
    if (tryCount > 10) {
        HAL_NVIC_SystemReset();
    }
    res = disk_initialize(USERFatFS.drv);
    if (res) {
        showError("����� ������", (int) res); 
        tryCount++;
        return 0;
    }
    currFile.lfsize = _MAX_LFN + 1;
    res = f_mount(&USERFatFS,(TCHAR const*)USERPath, 0) ;
    if (res == FR_OK) {
        return 1;
    } else {
        showError("�������� �������", res); 
        tryCount++;
    }
    return 0;
}
//------------------------------------------------------------------------------

void showError(char *text, int errCode)
{
   char dummy;
   lcdClearAll();
   lcdPrintf(1, 10, clWhite, clBlack, "� � � � � � � �!");
   lcdPrintf(1, 30, clWhite, clBlack, "    ������      ");
   lcdPrintf(1, 40, clWhite, clBlack, "%s!", text);
   lcdPrintf(1, 55, clWhite, clBlack, "(��� %d)", errCode);
   lcdUpdate();
   osDelay(1000);
   xQueueReceive(KeysQueueHandle, &dummy, 2000); 
   lcdClearAll();
   lcdUpdate();
   osDelay(500);
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
    while ((res = f_opendir(&dir, currPath)) != FR_OK) {                           
        showError("�������� �����", (int)res);
        InitFAT();
    }    
    lcdClearAll();
    while (res == FR_OK) {
        res = f_readdir(&dir, &fno);   
        if (res != FR_OK) {
            showError("������ ������", (int)res);
            break;
        }
            
        if (fno.fname[0] == 0) {       // Break on error or end of dir   
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
    f_closedir(&dir);
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

int executeFile(char *fileName)
{
    FRESULT res;
    int num = 0;
    int pos = 0;
    char str[6];
    uint8_t *buf;
    uint32_t n;
    char key;
    int playNext = 0;  
    int witdh;
    static int volume[2] = {30, 70};
    GPIO_PinState p_muteState = muteState;
    
    
    memset(str, 0, sizeof(str));
    if (VS1053_play() != VS1053_OK || VS1053_setVolume(volume[(int)muteState]) != VS1053_OK) {
        showError("��������", 0);
        return 0;
    }
    if (!myMalloc((void**)&buf, BUF_SIZE, 1000)) {
        return 0;
    }                     
    res = f_open(&USERFile, fileName, FA_READ);
    if(res != FR_OK) { 
        showError("�������� �����", (int)res);
        myFree((void**)&buf);
        return 1; 
    } 
    witdh = lcdPrintf(0, 0, clWhite, clBlack, currFileName);            
    do {
        if (muteState != p_muteState) {
            VS1053_setVolume(volume[(int)muteState]);
        }
        p_muteState = muteState;
        res = f_read(&USERFile, buf, BUF_SIZE, &n);
        while (res != FR_OK) {
            showError("������ �����", (int)res); 
            f_close(&USERFile);          
            if (InitFAT()) {
              f_open(&USERFile, fileName, FA_READ);
              f_lseek(&USERFile, num * BUF_SIZE);
              res = f_read(&USERFile, buf, BUF_SIZE, &n);
            } 
        }
        if (num > 3) {
            HAL_GPIO_WritePin(MUTE_GPIO_Port, MUTE_Pin, muteState);
        }
        if ((num & 0x03) == 0 || key) {
            if ((num & 0x07) == 0) {
                pos++;    
            }    
            lcdClearAll();
            lcdPrintf(1 - pos % witdh, 10, clWhite, clBlack, currFileName);            
            lcdPrintf(witdh + 32 - pos % witdh, 10, clWhite, clBlack, currFileName);            
            lcdRectangle(1, 30, 125, 38,  clWhite, clNone, 1);
            lcdRectangle(3, 32, 12300 / ((USERFile.fsize * 100) / USERFile.fptr), 36,  clWhite, clWhite, 0);
            sprintf(str, "%d%%", volume[(int)muteState]);
            lcdPrintf(10, 52, clWhite, clBlack, str);            
            for(int i = 0; i < volume[(int)muteState]/4 + 4; i++) {
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
                 if (volume[(int)muteState] > 5) {
                     volume[(int)muteState] -= 5;
                 }
                 VS1053_setVolume(volume[(int)muteState]);
                 break;
              case '-': 
                  if (volume[(int)muteState] < 100) {
                      volume[(int)muteState] += 5;
                  }
                  VS1053_setVolume(volume[(int)muteState]);
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
             showError("���������������", 0);
             break;
         } 
    } while(n);
    myFree((void**)&buf);
    f_close(&USERFile);
    HAL_GPIO_WritePin(MUTE_GPIO_Port, MUTE_Pin, GPIO_PIN_SET);
    return playNext;
}
//------------------------------------------------------------------------------


int myMalloc(void **buf, uint32_t size, uint32_t timeout) {
    int attempts = timeout / 100; 
    
    do {
       *buf = pvPortMalloc(size);
       if (*buf != NULL) {
           return 1;
       } else {
           osDelay(100);
       }
    } while (attempts--);
    printf("\nERROR! Can't allocate %d bytes of RAM memory!\n", size);
    showError("��������� ������", size);
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
