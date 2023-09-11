#include "../inc/join_game.h"
#include "../inc/vga_utils.h"
#include "../inc/touch.h"
#include "../inc/wifi.h"

#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>

// Button offsets
#define BTN_WIDTH 100
#define BTN_TOP   350
#define BTN_BOT   420
#define MARGIN_LR 130

// Keyboard offsets
#define KBD_TOP   220
#define KBD_BOT   290
#define KBD_LEFT  30
#define KBD_RIGHT 610
#define KBD_CHAR_X_OFFSET 25
#define KBD_CHAR_Y_OFFSET 30

#define CODE_DISP_TOP   100
#define CODE_DISP_BOT   180
#define CODE_DISP_LEFT  215
#define CODE_DISP_RIGHT 425
#define CODE_CHAR_X_OFFSET 23
#define CODE_CHAR_Y_OFFSET 35

// Public functions ------------------------------------------------------------
JOIN_GAME_BUTTONS joinGameScreen(void);
void failureScreen(void);

// Private functions -----------------------------------------------------------
static void drawKeyboard(void);
static int getKeyboardTouch(Point * p, char * ret);
static void drawOptionButtons(void);
static JOIN_GAME_BUTTONS getButtonPress(Point * p);
static void drawGameCodeDisplay(void);
static void drawGameCode(char c, int char_position);
static void resetScreen(void);


JOIN_GAME_BUTTONS joinGameScreen(void) {
  resetScreen();

  char c;
  char game_code[5];
  int char_count = 0;
  Point touch;
  JOIN_GAME_BUTTONS button;
  int keyboard_touch;

  game_code[4] = '\0';

  while (true) {
    touch = getTouch();
    keyboard_touch = getKeyboardTouch(&touch, &c);
    button = getButtonPress(&touch);

    if (button == JGB_RESET && char_count != 0) {
      resetScreen();
      char_count = 0;
      continue;
    } else if (keyboard_touch == 0 && char_count != 4) {
      game_code[char_count] = c;
      drawGameCode(c, char_count);
      char_count++;
      continue;
    } 

    // Got the full game code (4 digits)
    if (button == JGB_CONFIRM && char_count == 4) {
      game_code[4] = '\0';
      int game_code_int = atoi(game_code);
      char response[25];
      printf("game code = %d\n", game_code_int);

      //Send digits over wifi and wait for server response
      joinRoom(g_wifi_dev, game_code_int);
      readWIFIResponse(g_wifi_dev, response, RES_COMPLETE);
      if (response[OPCODE] != START_SETUP) {
        failureScreen();
        resetScreen();
        char_count = 0;
        continue;
      } else {
        return JGB_CONFIRM;
      }
    }
  }
}

void failureScreen(void) {
  clearScreen(0);

  drawStringPixels("INCORRECT CODE!", 245, 200, WHITE);
  drawString("Touch the screen to try again", 205, 300);
  usleep(200000);

  getTouch();
}

static void drawKeyboard(void) {
  int kbd_width = KBD_RIGHT - KBD_LEFT;
  
  drawHline(KBD_LEFT, KBD_RIGHT, KBD_TOP, WHITE, 0);
  drawHline(KBD_LEFT, KBD_RIGHT, KBD_BOT, WHITE, 0);

  for (int i = 0; i <= 10; i++) {
    char num = '0' + i;
    char str[2];
    str[0] = num;
    str[1] = '\0';

    drawVline(KBD_LEFT + i * kbd_width/10, KBD_TOP, KBD_BOT, WHITE, 0);

    if (i != 10) {
      drawString(str, KBD_LEFT + i * kbd_width/10 + KBD_CHAR_X_OFFSET, KBD_TOP + KBD_CHAR_Y_OFFSET);
    }
  }  
}

int getKeyboardTouch(Point * p, char * ret) {
  int kbd_width = KBD_RIGHT - KBD_LEFT;

  for (int i = 0; i < 10; i++) {
    if (p->x >= KBD_LEFT + i * kbd_width/10 && p->x <= KBD_LEFT + (i+1) * kbd_width/10 &&
        p->y >= KBD_TOP && p->y <= KBD_BOT) {

        *ret = '0' + i;
        return 0;
    }
  }

  return 1;
}

static void drawOptionButtons(void) {
  // Reset button
  drawString("Reset Input", MARGIN_LR + 5, BTN_TOP + 30);
  drawFilledBox(MARGIN_LR, BTN_TOP, MARGIN_LR + BTN_WIDTH, BTN_BOT, RED, 0);

  // Confirm button
  drawString("Confirm", SCREEN_X - MARGIN_LR - BTN_WIDTH + 20, BTN_TOP + 30);
  drawFilledBox(SCREEN_X - MARGIN_LR - BTN_WIDTH, BTN_TOP, SCREEN_X - MARGIN_LR, BTN_BOT, GREEN, 0);
}

static JOIN_GAME_BUTTONS getButtonPress(Point * p) {
  // Reset button
  if (p->x >= MARGIN_LR && p->x <= MARGIN_LR + BTN_WIDTH &&
      p->y >= BTN_TOP && p->y <= BTN_BOT) {

    return JGB_RESET;
  }

  // Confirm button
  if (p->x >= SCREEN_X - MARGIN_LR - BTN_WIDTH && p->x <= SCREEN_X - MARGIN_LR &&
      p->y >= BTN_TOP && p->y <= BTN_BOT) {

    return JGB_CONFIRM;
  }

  return JGB_NONE;
}

static void drawGameCodeDisplay(void) {
  int code_disp_width = CODE_DISP_RIGHT - CODE_DISP_LEFT;
  
  drawHline(CODE_DISP_LEFT, CODE_DISP_RIGHT, CODE_DISP_TOP, WHITE, 0);
  drawHline(CODE_DISP_LEFT, CODE_DISP_RIGHT, CODE_DISP_BOT, WHITE, 0);

  for (int i = 0; i <= 4; i++) {
    drawVline(CODE_DISP_LEFT + i * code_disp_width/4, CODE_DISP_TOP, CODE_DISP_BOT, WHITE, 0);
  }
}

static void drawGameCode(char c, int char_position) {
  int code_disp_width = CODE_DISP_RIGHT - CODE_DISP_LEFT;
  drawChar(c, CODE_DISP_LEFT + char_position * code_disp_width/4 + CODE_CHAR_X_OFFSET,
            CODE_DISP_TOP + CODE_CHAR_Y_OFFSET);
}

static void resetScreen(void) {
  clearScreen(0);
  usleep(100000); //0.1sec

  drawStringPixels("Enter Game Code", 247, 50, WHITE);
  drawKeyboard();
  drawOptionButtons();
  drawGameCodeDisplay();
}
