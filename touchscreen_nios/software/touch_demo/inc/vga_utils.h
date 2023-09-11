#ifndef VGA_UTILS_H
#define VGA_UTILS_H

#include "./globals.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define RED         0xF800
#define GREEN       0x07E0
#define BLUE        0x001F
#define WHITE       0xFFFF
#define BLACK       0x0000
#define YELLOW      0xFFE0
#define MAGENTA     0xF81F
#define CYAN        0x07FF
#define GREY        0x94B2 
#define LIGHT_RED   0xdaeb
#define LIGHT_BLUE  0x1B7B

#define SCREEN_X 640
#define SCREEN_Y 480
#define CHAR_BUF_X 80
#define CHAR_BUF_Y 60

#define SCREEN_X_SCALE 0.8

void drawEmptyBox(int x1, int y1, int x2, int y2, int color, int backbuffer);
void drawFilledBox(int x1, int y1, int x2, int y2, int color, int backbuffer);
void drawLine(int x1, int y1, int x2, int y2, int color, int backbuffer);
void drawVline(int x, int y1, int y2, int color, int backbuffer);
void drawHline(int x1, int x2, int y, int color, int backbuffer);
void clearScreen(int backbuffer);
void clearCharacters(void);
void drawStringPixels(const char * string, int x, int y, int color);
void drawString(const char* string, int x, int y);
void drawChar(char c, int x, int y);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif