#include "../inc/main_menu.h"
#include "../inc/vga_utils.h"
#include "../inc/touch.h"

void mainMenuScreen(void) {
  clearScreen(0);

  drawStringPixels("BATTLESHIP!", 270, 200, WHITE);
  drawString("Touch the screen to continue", 205, 300);

  getTouch();
}