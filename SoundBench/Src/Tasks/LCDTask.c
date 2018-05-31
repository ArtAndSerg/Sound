// file mainTask.c

#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include "main.h"
#include "stm32f1xx_hal.h"
#include "cmsis_os.h"
#include "my/myTasks.h"
#include "my/mylcd.h"
#include "my/CYRILL3.h"

#define clBlack  0
#define clWhite  1
#define clInvert 2

#define CS_UP()     HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_SET)
#define CS_DOWN()   HAL_GPIO_WritePin(CS_GPIO_Port, CS_Pin, GPIO_PIN_RESET)

#define CLK_UP()    HAL_GPIO_WritePin(SCK_GPIO_Port, SCK_Pin, GPIO_PIN_SET)
#define CLK_DOWN()  HAL_GPIO_WritePin(SCK_GPIO_Port, SCK_Pin, GPIO_PIN_RESET)

#define MOSI_UP()   HAL_GPIO_WritePin(MOSI_GPIO_Port, MOSI_Pin, GPIO_PIN_SET)
#define MOSI_DOWN() HAL_GPIO_WritePin(MOSI_GPIO_Port, MOSI_Pin, GPIO_PIN_RESET)

#define lcdCmd(a)   softSPIWrite(0x00F80000ul | (((uint16_t)(a) << 8) & 0xF000) | (((a) << 4) & 0xF0))
#define lcdData(a)  softSPIWrite(0x00FA0000ul | (((uint16_t)(a) << 8) & 0xF000) | (((a) << 4) & 0xF0))

static uint8_t videoBuf[(128*64)/8];

static void softSPIWrite(uint32_t data);
static void lcdWriteArray(uint8_t *data, int len);


void InitLcdTask(void)
{
    memset(videoBuf, 0, sizeof(videoBuf));
    for (int i = 0; i < sizeof(videoBuf); i++) {
        videoBuf[i] = 0;
    }
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
}
//------------------------------------------------------------------------------

void lcdUpdate(void)
{ 
    CS_UP();
    lcdCmd(0x36);
    for (uint8_t y = 0; y < 32; y++) {  
        lcdCmd(0x80 + y); 
        lcdCmd(0x80);
        lcdWriteArray(&videoBuf[32 * y], 32);    
    }
    CS_DOWN();    
}
//------------------------------------------------------------------------------

int lcdPixel(int x, int y, int color) // color = 0 / 1 / 2(invert)
{
    int n = 0;
    if  (x < 0 || x > 128 || y < 0 || y > 64) {
        return 0;
    }
    if (y > 31) {
        n += 16;
        y -= 32;
    }
    n += x/8 + y * 32;
    switch (color) {
      case 0:  videoBuf[n] &= ~(0x80 >> (x % 8));
               break;
      case 1:  videoBuf[n] |= (0x80 >> (x % 8));
               break;
      case 2:  videoBuf[n] ^= (0x80 >> (x % 8));
               break;      
    }
    return 1;
}
//------------------------------------------------------------------------------
 
void lcdcircle(int X1, int Y1, int R, int color)
{
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
}
//------------------------------------------------------------------------------

void lcdline(int x0, int y0, int x1, int y1, int color) 
{
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
}
//-----------------------------------------------------------------------------


void lcdPrintf(int xStart, int yStart, int colorFont, int colorBack, const char *format, ...)
{
  unsigned char *str, symbol;
  int len;
  int x0 = xStart, y0 = yStart;
  
  myCalloc(&str, 256, 1000);
  va_list marker;
  va_start(marker, format);
  vsprintf((char *)str, format, marker);
  va_end(marker);
  len = strlen((char*)str);
  if (len > 128) {
      len = 128;
  }
  for (int i = 0; i < len; i++) {
      symbol = str[i];
      switch (symbol) {
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
        case '\n': y0 += 9;
                   x0 = xStart;
                   continue;
                         
        default: if (symbol > 'À') {
                     symbol -= 'À' - 127;
                 } else {
                     symbol -= '!';
                 }
      }
      
      for (int y = 0; y < 8; y++) {
          for (int x = 0; x < 8; x++) {
              if (str[i] != ' ' && font[y + 8 * symbol] & (0x80 >> x)) {
                  lcdPixel(x0 + x, y0 + y, colorFont);
              } else {
                  lcdPixel(x0 + x, y0 + y, colorBack);
              }
          }
      }
      x0 += 8;
  }
  myFree(&str);
}
//------------------------------------------------------------------------------


void lcdTask(void)
{
    /*
    static uint32_t c = 0;
    for (int i = 0; i < sizeof(videoBuf)/4; i++) {
        videoBuf[4*i+3] = c;
        videoBuf[4*i+2] = c>>8;
        videoBuf[4*i+1] = c>>16;
        videoBuf[4*i+0] = c>>24;
        
    }
    c = c << 1;
    if (!c) c = 1;
    */
    static uint8_t str[150];
    int c = 'z';
    
    
    for (int i = 0; i < 128; i++) {
        //for (int i = 0; i < 16; i++) {
          // str[i] = c++;
        //}
        str[0] = '\0';    
        for (int i = 0; i < 7; i++) {
          sprintf (&str[strlen(str)], "\"%c\" - %d\n", c, c);
          c++;
        }
        
        
        
        //lcdPrintf(0, 0, clWhite, clBlack, str);
        lcdPrintf(i, i%64, clWhite, clBlack, "Ýé, æëîá! Ãäå\nòóç? Ïðÿ÷ü þíûõ\nñú¸ìùèö â øêàô.");
        //lcdPrintf(i, i%64, clWhite, clBlack, "Hello");
        
        lcdcircle(63, 31, i%32, clInvert);
        lcdline (0, 0, i, 64, clInvert);
        lcdline (0, 64, i, 0, clInvert);
        
        lcdUpdate();       
        
        //lcdPrintf(0, 0, clBlack, clBlack, str);
        lcdPrintf(i, i%64, clBlack, clBlack, "Ýé, æëîá! Ãäå\nòóç? Ïðÿ÷ü þíûõ\nñú¸ìùèö â øêàô.");
        //lcdPrintf(i, i%64, clBlack, clBlack, "Hello");
        
        lcdcircle(63, 31, i%32, clInvert);
        lcdline (0, 0, i, 64, clInvert);
        lcdline (0, 64, i, 0, clInvert);
        
        osDelay(100);
    }    
}
//------------------------------------------------------------------------------

static void lcdWriteArray(uint8_t *data, int len)
{
    for (int i = 0; i < len; i++) {
       lcdData(data[i]); 
    }
}
//------------------------------------------------------------------------------

static void softSPIWrite(uint32_t data)
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

