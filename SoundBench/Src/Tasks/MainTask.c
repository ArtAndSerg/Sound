// file mainTask.c

#include <string.h>
#include <stdio.h>
#include "my/myTasks.h"
#include "cmsis_os.h"
#include "my/mylcd.h"

extern osMessageQId KeysQueueHandle;

#define ITEM_HEIGHT 11
#define ITEMS_MAXCOUNT 5


typedef struct 
{
   char caption[20];
   int  itemJumpTo;
} item_t;


typedef struct 
{
    int id;
    int itemsCount;
    item_t item[ITEMS_MAXCOUNT];
} menu_t;

static const menu_t menu[8] = { {0, 2, 
                                  {{"������� ����",     1000},
                                   {"English language", 2000}}},
                                {1000, 4,
                                  {{"�������. �������", 1100},
                                   {"������",           1200},
                                   {"����������",       0}, 
                                   {"������",           0}}},
                                {2000, 4,
                                  {{"Historical info",  2100},
                                   {"Music",            2200},
                                   {"Information",      2000}, 
                                   {"Help",             2000}}},
                                {1100, 3,
                                 {{"���",              1000},
                                  {"���",              1000},
                                  {"����",             1000}}},
                                {2100, 3,
                                 {{"GUM",              2000},
                                  {"TsUM",             2000},
                                  {"VDNKh",            2000}}}, 
                                {1200, 3,
                                 {{"������",           1000},
                                  {"������",           1000},
                                  {"����������",       1000}}},  
                                 
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

void showMenu(int x0, int y0, int selected, const menu_t *m) 
{
    int Xmax = 0, tmp; 
    lcdClearAll();
    
    for (int i = 0; i < m->itemsCount; i++) {
        tmp = lcdPrintf(x0 + 2, y0 + 2  + i * ITEM_HEIGHT, clWhite, clBlack, m->item[i].caption);
        if (tmp > Xmax) {
            Xmax = tmp;
        }
    }
    if (selected >= 0 && selected < m->itemsCount) {
        lcdRectangle(x0, y0 + selected * ITEM_HEIGHT, Xmax, y0 + (selected + 1) * ITEM_HEIGHT,  clWhite, clNone, 1);
    }   
    lcdUpdate();    
}
//-----------------------------------------------------------------------------

void InitMainTask(void)
{
    osDelay(500);
    showMenu(3, 3, 0, &menu[0]);
    SD_Init();
}
//------------------------------------------------------------------------------

void MainTask(void)
{
    static int currMenuId = 0, currItem = 0;   
    uint8_t key;
    
    if (xQueueReceive(KeysQueueHandle, &key, 10000) != pdPASS) {
        while (xQueueReceive(KeysQueueHandle, &key, 100) != pdPASS) {
            lcdScreenSaver();
         }
    }
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
