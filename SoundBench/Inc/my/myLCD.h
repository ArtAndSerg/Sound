// file myLCD.h

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MYLCD_H__
#define __MYLCD_H__

void lcdUpdate(void);
void lcdRectangle(int left, int up, int right, int down);
int  lcdPixel(int x, int y, int color); // color = 0 / 1 / 2(invert)
void lcdLine(int x0, int y0, int x1, int y1, int color);
void lcdCircle(int x, int y, int R, int color);
void lcdPrintfGraphicMode(int x0, int y0, const char *format, ...);
  
#endif
