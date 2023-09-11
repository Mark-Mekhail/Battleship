#include "../inc/win_lose.h"
#include "../inc/vga_utils.h"
#include "../inc/touch.h"

#include <stdbool.h>

void winLoseScreen(bool win) {
  clearScreen(0);

  if (win) {
    drawStringPixels("YOU WIN! :)", 270, 200, YELLOW);
  } else {    
    drawStringPixels("YOU LOSE! :(", 265, 200, YELLOW);
  }

  drawString("Tap anywhere to go back to the main menu", 150, 300);

  getTouch();
}