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

#define ITEM_HEIGHT 11
#define ITEMS_MAXCOUNT 100
#define BUF_SIZE    (2048)//(50 * VS1053_MAX_TRANSFER_SIZE)

typedef struct 
{
    uint32_t  count;
    FILINFO   *item;
    char      *longFileNamesBuf;
    uint32_t  longFileNamesBufSize;
} menu_t;

menu_t menu;

void deleteMenu(menu_t *menu)
{
    myFree((uint8_t**)&menu->longFileNamesBuf);
    myFree((uint8_t**)&menu->item);
    memset((void*)menu, 0, sizeof(menu_t));    
}
//-----------------------------------------------------------------------------

FRESULT dirToMenu(menu_t *menu, char *path)
{
    FRESULT res = FR_OK;
    DIR dir;
    FILINFO fno;
    int len;
    
    memset (menu, 0, sizeof(menu_t));
    fno.lfsize = _MAX_LFN+1;
    if (!myMalloc((uint8_t**)&fno.lfname, fno.lfsize, 100)) {
        res = FR_NOT_ENOUGH_CORE;
    }
    deleteMenu(menu);   
    for (int i = 0; i < 2 && res == FR_OK; i++) {
        menu->count = 0;
        menu->longFileNamesBufSize = 0;
        res = f_opendir(&dir, path);                           /* Open the directory */
        while (res == FR_OK) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) {       /* Break on error or end of dir */
                f_closedir(&dir);    
                break;
            }
            if (fno.fattrib & (AM_HID | AM_SYS)) {
                continue;
            }
            len = strlen(fno.lfname) + 1;
            if (menu->item != NULL && menu->longFileNamesBuf != NULL) {
                menu->item[menu->count] = fno;
                menu->item[menu->count].lfname = &menu->longFileNamesBuf[menu->longFileNamesBufSize];
                menu->item[menu->count].lfsize = len;
                memcpy((void*)&menu->longFileNamesBuf[menu->longFileNamesBufSize], fno.lfname, len);                               
            }
            menu->count++;
            menu->longFileNamesBufSize += len;
        }
        if (res == FR_OK && !i) {
            if (!myMalloc((uint8_t**)&menu->item, menu->count * sizeof(FILINFO), 1000) || !myMalloc((uint8_t**)&menu->longFileNamesBuf, menu->longFileNamesBufSize, 1000)) {
                printf ("ERROR! No RAM! \n"); 
                res = FR_NOT_ENOUGH_CORE;
                break;
            }
        }
    }
    if (res != FR_OK) {
      deleteMenu(menu);   
    }
    myFree((uint8_t**)&fno.lfname);
    return res;
}
//-----------------------------------------------------------------------------




/*
typedef struct 
{
   char caption[MAX_LFN];
   int  itemJumpTo;
} item_t;


typedef struct 
{
    int id;
    int itemsCount;
    item_t item[ITEMS_MAXCOUNT];
} menu_t;

static const menu_t menu[8] = { {0, 2, 
                                  {{"Русский язык",     1000},
                                   {"English language", 2000}}},
                                {1000, 4,
                                  {{"Историч. справка", 1100},
                                   {"Музыка",           1200},
                                   {"Информация",       0}, 
                                   {"Помощь",           0}}},
                                {2000, 4,
                                  {{"Historical info",  2100},
                                   {"Music",            2200},
                                   {"Information",      2000}, 
                                   {"Help",             2000}}},
                                {1100, 3,
                                 {{"ГУМ",              1000},
                                  {"ЦУМ",              1000},
                                  {"ВДНХ",             1000}}},
                                {2100, 3,
                                 {{"GUM",              2000},
                                  {"TsUM",             2000},
                                  {"VDNKh",            2000}}}, 
                                {1200, 3,
                                 {{"Моцарт",           1000},
                                  {"Штраус",           1000},
                                  {"Чайковский",       1000}}},  
                                 
                                {2200, 3,
                                 {{"Mozart",           2000},
                                  {"Strauss",          2000},
                                  {"Tchaikovsky",      2000}}},   
                                  
};


const menu_t *menuId(int id)
{
    for (int i = 0; i < sizeof(menu) / sizeof(menu[0]); i++) {
        if (menu[i].id == id) {
            return &menu[i];
        }
    }
    return NULL;
}
//-----------------------------------------------------------------------------
*/

void showMenu(int x0, int y0, int selected, const menu_t *m) 
{
    int Xmax = 0, tmp; 
    lcdClearAll();
    
    for (int i = 0; i < m->count; i++) {
        tmp = lcdPrintf(x0 + 2, y0 + 2  + i * ITEM_HEIGHT, clWhite, clBlack, m->item[i].lfname);
        if (tmp > Xmax) {
            Xmax = tmp;
        }
    }
    if (selected >= 0 && selected < m->count) {
        lcdRectangle(x0, y0 + selected * ITEM_HEIGHT, Xmax, y0 + (selected + 1) * ITEM_HEIGHT,  clWhite, clNone, 1);
    }   
    lcdUpdate();    
}
//-----------------------------------------------------------------------------

void InitMainTask(void)
{
    memset (&menu, 0, sizeof(menu));   
    while (1) {
        disk_initialize(USERFatFS.drv);
        lcdClearAll();
        lcdUpdate();
        osDelay(500);
        if (f_mount(&USERFatFS,(TCHAR const*)USERPath, 0) == FR_OK) {
            break;
        } else {
            lcdPrintf(1, 10, clWhite, clBlack, "В Н И М А Н И Е!");
            lcdPrintf(1, 30, clWhite, clBlack, "    Ошибка      ");
            lcdPrintf(1, 40, clWhite, clBlack, " карты памяти!  ");   
            lcdUpdate();
            osDelay(1000);    
        }
    }      
}
//------------------------------------------------------------------------------

void MainTask(void)
{
    static int currMenuId = 0, currItem = 0;   
    static PLAYER_State prevState = PLAYER_ERROR;
    static char path[512] = "/";
    static char filePath[512] = "";
    static uint8_t volume = 60;
    static uint8_t *buf;
    char key;
    uint32_t n = 0;  
    PLAYER_State state = VS1053_getState();
    
    if (prevState != state) {
       showMenu(0, 0, currItem, &menu); 
       VS1053_setVolume(volume);   
    }
    
    switch (state) {
      case  PLAYER_ERROR:
        if (prevState != state) {
            lcdClearAll();
            lcdPrintf(1, 10, clWhite, clBlack, "В Н И М А Н И Е!");
            lcdPrintf(1, 30, clWhite, clBlack, "    Ошибка      ");
            lcdPrintf(1, 40, clWhite, clBlack, "воспроизведения!");       
            lcdUpdate();
            xQueueReceive(KeysQueueHandle, &key, 4000);
            showMenu(0, 0, currItem, &menu);
        }
        
      case PLAYER_STOP:
        if (xQueueReceive(KeysQueueHandle, &key, 10000) != pdPASS) {
            lcdClearAll();
            while (xQueueReceive(KeysQueueHandle, &key, 150) != pdPASS) {
                lcdScreenSaver();
            }
        }
        switch (key) {
          case '<' :  
            deleteMenu(&menu);
            if (strchr(path, '/') != NULL) {
                *strchr(path, '/') = '\0';
            }
            dirToMenu(&menu, path);
            currItem = 0;            
            break;
            
          case '>' : 
            if (menu.item[currItem].fattrib & AM_DIR) {
                strcat(path, "/");
                strcat(path, menu.item[currItem].lfname);
                deleteMenu(&menu);
                dirToMenu(&menu, path);
                currItem = 0;                            
            } else {
                strcpy(filePath, path);
                strcat(filePath, "/");
                strcat(filePath, menu.item[currItem].lfname);
                if(f_open(&USERFile, filePath, FA_READ) != FR_OK) { 
                    lcdClearAll();
                    lcdPrintf(1, 10, clWhite, clBlack, "В Н И М А Н И Е!");
                    lcdPrintf(1, 30, clWhite, clBlack, "Ошибка открытия ");
                    lcdPrintf(1, 40, clWhite, clBlack, "файла!");       
                    lcdUpdate();
                    xQueueReceive(KeysQueueHandle, &key, 4000);
                    showMenu(0, 0, currItem, &menu);
                } else {
                    if (VS1053_play() != VS1053_OK) {
                       f_close(&USERFile);
                    } else {
                       if (!myMalloc(&buf,   BUF_SIZE, 1000)) {
                          lcdPrintf(1, 10, clWhite, clBlack, "ERROR! No RAM!");       
                          lcdUpdate();
                          return;
                       }  
                       
                    }
                    
                }
                
             }
             break; 
          case '+':  currItem++;
             if (currItem == 5) {
                currItem = 0; 
             }
             break;
          case '-' :  
            currItem--;
            if (currItem < 0) {
                currItem = 4; 
            }
            break;
        }
        showMenu(0, 0, currItem, &menu);
        break;
        
      case PLAYER_PLAY:
        f_read(&USERFile, buf, BUF_SIZE, &n);
        VS1053_addData(buf, n);
        if (!n) {
            
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
                 myFree(&buf);
                 f_close(&USERFile);
                 VS1053_addData(NULL, 0);
                 break;
            }
        }        
    }       
    prevState = state;        
        
    /*
    lcdClearAll();
    switch (key) {
      case '<' :  if (currMenuId) {
                      currMenuId = 0;
                      currItem = 0;
                  }
                  break;
      case '>' :  currMenuId = menuId(currMenuId)->item[currItem].itemJumpTo;
                  currItem = 0;
                  break;
      case '-' :  currItem++;
                  if (currItem == menuId(currMenuId)->itemsCount) {
                     currItem = 0; 
                  }
                  break;
      case '+' :  currItem--;
                  if (currItem < 0) {
                     currItem = menuId(currMenuId)->itemsCount - 1; 
                  }
                  break;
    }
    showMenu(3, 3, currItem, menuId(currMenuId));   
    */
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
    if (*buf != NULL)
    {
        vPortFree(*buf);
        *buf = NULL;
    }
}
//-----------------------------------------------------------------------------
