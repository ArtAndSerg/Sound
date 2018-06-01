// file myLCD.h

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MYLCD_H__
#define __MYLCD_H__

#define clNone   0
#define clBlack  1
#define clWhite  2
#define clInvert 3

#define LCD_WITDTH   128
#define LCD_HEIHGT   64

void lcdUpdate(void);
void lcdRectangle(int left, int top, int right, int bottom, int colorOutline, int colorFill, int thicknessOutline);
void lcdLine(int x0, int y0, int x1, int y1, int color);
void lcdCircle(int x, int y, int R, int color);
int lcdPrintf(int xStart, int yStart, int colorFont, int colorBack, const char *format, ...);
void lcdShowImage(unsigned char *data, int x0, int y0, int width, int height, int colorFront, int colorBack);
void lcdMessageBox(char *msg);
void lcdClearAll(void);
void lcdScreenSaver(void);

#endif
