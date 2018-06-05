// file mainTask.c

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "main.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "my/myTasks.h"
#include "my/CYRILL3.h"
#include "my/logo.h"
#include "my/mylcd.h"


#define LCD_MUTEX_TIMEOUT 100

#define CS_UP()     HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET)
#define CS_DOWN()   HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET)

#define CLK_UP()    HAL_GPIO_WritePin(SCK_GPIO_Port, SCK_Pin, GPIO_PIN_SET)
#define CLK_DOWN()  HAL_GPIO_WritePin(SCK_GPIO_Port, SCK_Pin, GPIO_PIN_RESET)

#define MOSI_UP()   HAL_GPIO_WritePin(MOSI_GPIO_Port, MOSI_Pin, GPIO_PIN_SET)
#define MOSI_DOWN() HAL_GPIO_WritePin(MOSI_GPIO_Port, MOSI_Pin, GPIO_PIN_RESET)

#define lcdCmd(a)   softSPIWrite(0x00F80000ul | (((uint16_t)(a) << 8) & 0xF000) | (((a) << 4) & 0xF0))
#define lcdData(a)  softSPIWrite(0x00FA0000ul | (((uint16_t)(a) << 8) & 0xF000) | (((a) << 4) & 0xF0))
#define lcdClear()  memset(videoBuf, 0, sizeof(videoBuf))


extern osMutexId lcdMutexHandle;
static uint8_t videoBuf[(LCD_WITDTH * LCD_HEIHGT) / 8];

static inline void softSPIWrite(uint32_t data);
static inline void lcdWriteArray(uint8_t *data, int len);
static int lcdPixel(int x, int y, int color); // color = 0 / 1 / 2(invert)

void InitLcdTask(void)
{
    if (xSemaphoreTake(lcdMutexHandle, LCD_MUTEX_TIMEOUT) != pdPASS) {
        return;
    }
    memset(videoBuf, 0, sizeof(videoBuf));
    osDelay(50);
    CS_UP();
    osDelay(1);
    lcdCmd(0x30);
    osDelay(1);
    lcdCmd(0x30);
    osDelay(1);
    lcdCmd(0x01);
    osDelay(10);
    CS_DOWN();
    xSemaphoreGive(lcdMutexHandle);
}
//------------------------------------------------------------------------------

void lcdClearAll(void) 
{
    if (xSemaphoreTake(lcdMutexHandle, LCD_MUTEX_TIMEOUT) != pdPASS) {
        return;
    }
    lcdClear();
    xSemaphoreGive(lcdMutexHandle);
}
//------------------------------------------------------------------------------

void lcdUpdate(void)
{ 
    if (xSemaphoreTake(lcdMutexHandle, LCD_MUTEX_TIMEOUT) != pdPASS) {
        return;
    }
        CS_UP();
    lcdCmd(0x36);
    for (uint8_t y = 0; y < 32; y++) {  
        lcdCmd(0x80 + y); 
        lcdCmd(0x80);
        lcdWriteArray(&videoBuf[32 * y], 32);    
    }
    CS_DOWN();  
    xSemaphoreGive(lcdMutexHandle);
}
//------------------------------------------------------------------------------

static int lcdPixel(int x, int y, int color) // color = 0 / 1 / 2(invert)
{
    int n = 0;
    if  (color == clNone || x < 0 || x >= LCD_WITDTH || y < 0 || y >= LCD_HEIHGT) {
        return 0;
    }
    if (y > 31) {
        n += 16;
        y -= 32;
    }
    n += x/8 + y * 32;
    switch (color) {
      case clBlack:  videoBuf[n] &= ~(0x80 >> (x % 8));
                     break;
      case clWhite:  videoBuf[n] |= (0x80 >> (x % 8));
                     break;
      case clInvert: videoBuf[n] ^= (0x80 >> (x % 8));
                     break;      
    }
    return 1;
}
//------------------------------------------------------------------------------
 
void lcdcircle(int X1, int Y1, int R, int color)
{
   if (xSemaphoreTake(lcdMutexHandle, LCD_MUTEX_TIMEOUT) != pdPASS) {
        return;
   }
   int x = 0;
   int y = R;
   int delta = 1 - 2 * R;
   int error = 0;
   lcdPixel(X1 + R+1, Y1, color);
   lcdPixel(X1 - R-1, Y1, color);
   lcdPixel(X1, Y1 + R + 1, color);
   lcdPixel(X1, Y1 - R - 1, color);
   while (y >= 0) {
       lcdPixel(X1 + x, Y1 + y+1, color);
       lcdPixel(X1 + x, Y1 - y-1, color);
       lcdPixel(X1 - x, Y1 + y+1, color);
       lcdPixel(X1 - x, Y1 - y-1, color);
       error = 2 * (delta + y) - 1;
       if ((delta <= 0) && (error <= 0)) {
           delta += 2 * ++x + 1;
           continue;
       }
       error = 2 * (delta - x) - 1;
       if ((delta > 0) && (error > 0)) {
           delta += 1 - 2 * --y;
           continue;
       }
       x++;
       delta += 2 * (x - y);
       y--;
   }
   xSemaphoreGive(lcdMutexHandle);
}
//------------------------------------------------------------------------------

void lcdLine(int x0, int y0, int x1, int y1, int color) 
{
  if (xSemaphoreTake(lcdMutexHandle, LCD_MUTEX_TIMEOUT) != pdPASS) {
        return;
  }
  int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
  int err = (dx>dy ? dx : -dy)/2, e2;

  for(;;){
    if (!lcdPixel(x0, y0, color)) break;
    if (x0==x1 && y0==y1) break;
    e2 = err;
    if (e2 >-dx) { err -= dy; x0 += sx; }
    if (e2 < dy) { err += dx; y0 += sy; }
  }
  xSemaphoreGive(lcdMutexHandle);
}
//-----------------------------------------------------------------------------

void lcdRectangle(int left, int top, int right, int bottom, int colorOutline, int colorFill, int thicknessOutline) 
{
    if (xSemaphoreTake(lcdMutexHandle, LCD_MUTEX_TIMEOUT) != pdPASS) {
        return;
    }

    for (int x = left; x <= right; x++) {
        for (int y = top; y <= bottom; y++) {
            if (x < left + thicknessOutline || x > right - thicknessOutline || 
                y < top + thicknessOutline || y > bottom - thicknessOutline) {
                    lcdPixel(x, y, colorOutline);
                } else {
                    lcdPixel(x, y, colorFill);
                }    
        }
    }
    xSemaphoreGive(lcdMutexHandle);
}
//-----------------------------------------------------------------------------

int lcdPrintf(int xStart, int yStart, int colorFont, int colorBack, const char *format, ...)
{
  if (xSemaphoreTake(lcdMutexHandle, LCD_MUTEX_TIMEOUT) != pdPASS) {
        return 0;
  }

  unsigned char *str, symbol;
  int len;
  int x0 = xStart, y0 = yStart, leftBlank, xMax, xMin;
  
  myCalloc(&str, 256, 1000);
  va_list marker;
  va_start(marker, format);
  vsprintf((char *)str, format, marker);
  va_end(marker);
  len = strlen((char*)str);
  if (len > LCD_WITDTH) {
      len = LCD_WITDTH;
  }
  for (int i = 0; i < len; i++) {
      symbol = str[i];
      xMax = 0;
      xMin = 8;
      switch (symbol) {
        case 'ё':  symbol = 'e';
                   break;           
        case 'Ё':  symbol = 'E';
                   break;           
        case '\a': symbol = 192; // Char 192 (+/-)
                   break;
        case '\b': symbol = 193; // Char 193 (degrees Celsius)
                   break;
      
        case '\f': symbol = 194; // Char 194 ( (c) )
                   break;
        case '\v': symbol = 195; // Char 195 ( ^2 in square )
                   break;
        case '\t': symbol = 196; // Char 196 ( |_|  )
                   break;
        case ' ':  symbol = 197; // Char 197 (  space )
                   xMax = 4;
                   xMin = 0;
                   break;
        case '\n': y0 += 9; 
                   x0 = xStart;
                   continue;
                         
        default:   if (symbol >= 'А') {
                       symbol -= ('А' - 128);
                   }
      }
      symbol -= '!';
      for (int x = 0; x < 8; x++) {
          for (int y = 0; y < 8; y++) {
              if (font[y + 8 * symbol] & (0x80 >> x)) {
                  if (x > xMax) {
                      xMax = x;
                  }
                  if (x < xMin) {
                      xMin = x;
                  }
              }
          }
      }
      for (int x = xMin; x < xMax + 2; x++) {
          for (int y = 0; y < 8; y++) {
              if (font[y + 8 * symbol] & (0x80 >> x)) {
                  lcdPixel(x0, y0 + y, colorFont);
              } else {
                  lcdPixel(x0, y0 + y, colorBack);
              }
          }
          x0++;
      }
  }
  myFree(&str);
  xSemaphoreGive(lcdMutexHandle);
  return x0;
}
//------------------------------------------------------------------------------

void lcdShowImage(unsigned char *data, int x0, int y0, int width, int height, int colorFront, int colorBack)
{
    if (xSemaphoreTake(lcdMutexHandle, LCD_MUTEX_TIMEOUT) != pdPASS) {
        return;
    }

    int n = 0;
    for (int x = x0; x < x0 + width; x++) {
        for (int y = y0; y < y0 + height; y++) {
            if (data[n/8] & (0x80 >> (n & 0x07))) {
               lcdPixel(x, y, colorFront); 
            } else {
               lcdPixel(x, y, colorBack); 
            }
            n++;    
        }
    }
    xSemaphoreGive(lcdMutexHandle);
}
//------------------------------------------------------------------------------

void lcdScreenSaver(void)
{
        static int x = 0 , y = 0, dx = 1, dy = 1;
        x += dx;
        y += dy;
        if (y <= 0)   dy = 1;
        if (y == 40)  dy = -1;
        if (x == -64) dx = 1;
        if (x == 64)  dx = -1;
        lcdClearAll();
        lcdShowImage((uint8_t *)bmp, x, y, 128, 32, clWhite, clNone);
        lcdShowImage((uint8_t *)bmp, 64-x, 32 - y, 128, 32, clWhite, clNone);
        lcdPrintf(x, 64-y, clWhite, clNone, "Надёжная доствка! \f");               
        lcdUpdate();
}
//------------------------------------------------------------------------------

void lcdTask(void)
{
        osDelay(50000);              
}
//------------------------------------------------------------------------------

static inline void lcdWriteArray(uint8_t *data, int len)
{
    for (int i = 0; i < len; i++) {
       lcdData(data[i]); 
    }
}
//------------------------------------------------------------------------------

static inline void softSPIWrite(uint32_t data)
{
    for(int i = 0; i < 24; i++){
        CLK_DOWN();    
        if (data & (0x800000 >> i)) {
            MOSI_UP();
            MOSI_UP();
        } else {
            MOSI_DOWN();
            MOSI_DOWN();
        }
        CLK_UP();
    }  
}
//------------------------------------------------------------------------------

