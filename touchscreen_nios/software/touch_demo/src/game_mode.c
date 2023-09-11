#include "../inc/globals.h"
#include "../inc/vga_utils.h"
#include "../inc/game_mode.h"
#include "../inc/touch.h"

#include <stdbool.h>

// Public functions ------------------------------------------------------------
GAME_MODE_BUTTONS gameModeMenuScreen(void);

// Private functions -----------------------------------------------------------
static void drawGameModeMenu(void);
static GAME_MODE_BUTTONS getButtonPress(Point * p);


GAME_MODE_BUTTONS gameModeMenuScreen(void) {
  clearScreen(0);
  drawGameModeMenu();

  while (true) {
    Point touch = getTouch();
    GAME_MODE_BUTTONS button = getButtonPress(&touch);

    if (button == GMB_SINGLE_PLAYER) {
      return GMB_SINGLE_PLAYER;
    }

    if (button == GMB_MULTI_PLAYER) {
      return GMB_MULTI_PLAYER;
    }
  }
}

/**
 * @brief Draw the game mode menu screen
 * 
 */
static void drawGameModeMenu(void) {
  drawStringPixels("Select a Game Mode", 225, 50, WHITE);
  
  // Singleplayer
  drawFilledBox(122, 208, 229, 287, LIGHT_BLUE, 0);
  drawString("Singleplayer", 128, 240);

  // Multiplayer
  drawFilledBox(392, 208, 500, 287, LIGHT_BLUE, 0);
  drawString("Multiplayer", 408, 240);
}

static GAME_MODE_BUTTONS getButtonPress(Point * p) {
  if (p->y >= 208 && p->y <= 287) {
    // Singleplayer button
    if (p->x >= 122 && p->x <= 229) {
      return GMB_SINGLE_PLAYER;
    }

    // Multiplayer button
    else if (p->x >= 392 && p->x <= 500) {
      return GMB_MULTI_PLAYER;
    }
  }

  return GMB_NONE;
}
