#include "../inc/globals.h"
#include "../inc/multiplayer.h"
#include "../inc/vga_utils.h"
#include "../inc/touch.h"
#include "../inc/wifi.h"

#include <stdbool.h>
#include <stdio.h>

// Public Functions
MULTI_PLAYER_BUTTONS multiplayerScreen(void);
void establishingConnectionScreen(void);

// Private Functions
static void drawMultiplayerMenu(void);
static MULTI_PLAYER_BUTTONS getButtonPress(Point * p);

MULTI_PLAYER_BUTTONS multiplayerScreen(void) {
  clearScreen(0);
  drawMultiplayerMenu();

  while (true) {
    Point touch = getTouch();
    MULTI_PLAYER_BUTTONS button = getButtonPress(&touch);

    if (button == MPB_CREATE_GAME) {
      return MPB_CREATE_GAME;
    }

    if (button == MPB_JOIN_GAME) {
      return MPB_JOIN_GAME;
    }
  }
}

void establishingConnectionScreen(void) {
  clearScreen(0);
  drawStringPixels("Establishing WIFI Connection...", 150, 220, WHITE);
  char response[25];

  while (true) {
    checkConnection(g_wifi_dev);
    readWIFIResponse(g_wifi_dev, response, RES_COMPLETE);
    // printf("Response: %s\n", response);
    if (response[OPCODE] == CHECK_WSCON) {
      // printf("Got correct OPCODE: %d\n", CHECK_WSCON);
      if (response[IS_CONNECTED] == '1') {
        printf("Connection Established\n");
        break;
      }
    }
  }

  leaveRoom(g_wifi_dev);
}

static void drawMultiplayerMenu(void) {
  drawStringPixels("Multiplayer", 257, 50, WHITE);
  
  drawFilledBox(122, 208, 229, 287, LIGHT_BLUE, 0);
  drawString("Create Game", 128, 240);

  drawFilledBox(392, 208, 500, 287, LIGHT_BLUE, 0);
  drawString("Join Game", 408, 240);
}

static MULTI_PLAYER_BUTTONS getButtonPress(Point * p) {
  if (p->y >= 208 && p->y <= 287) {
    // Create Room Button
    if (p->x >= 122 && p->x <= 229) {
      return MPB_CREATE_GAME;
    }

    // Join Room Button
    else if (p->x >= 392 && p->x <= 500) {
      return MPB_JOIN_GAME;
    }
  }

  return MPB_NONE;
}

