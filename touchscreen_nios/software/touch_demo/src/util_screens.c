#include "../inc/vga_utils.h"

void waitForConfirmationScreen(void) {
  clearScreen(0);
  drawStringPixels("Waiting for confirmation...", 200, 200, WHITE);
}