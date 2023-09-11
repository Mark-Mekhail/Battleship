#include "../inc/create_game.h"
#include "../inc/touch.h"
#include "../inc/vga_utils.h"
#include "../inc/wifi.h"
#include "../inc/globals.h"

#include <string.h>

static void waitingForOpponentScreen(const char * game_code);

void createGameScreen(const char * game_code) {
  clearScreen(0);
  drawStringPixels("Generating Game Code...", 150, 220, WHITE);

  //wait for Wifi to receive confirmation
  char response[25];
  createRoom(g_wifi_dev);
  readWIFIResponse(g_wifi_dev, response, RES_COMPLETE);
  if (response[OPCODE] == ROOM_CREATED) {
    response[5] = '\0';
    strcpy(game_code, &response[1]);
  }

  waitingForOpponentScreen(game_code);
}

static void waitingForOpponentScreen(const char * game_code) {
  clearScreen(0);

  drawStringPixels("Game Code:", 260, 100, WHITE);
  drawStringPixels(game_code, 290, 150, YELLOW);
  drawStringPixels("Waiting for an opponent to join...", 130, 250, WHITE);

  char response[25];
  readWIFIResponse(g_wifi_dev, response, RES_COMPLETE);
  if (response[OPCODE] != START_SETUP) {
    printf("ERROR: %c\n", response[OPCODE]);
  }
}