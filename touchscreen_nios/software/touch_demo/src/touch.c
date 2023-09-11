#include "../inc/touch.h"
#include "../inc/main.h"
#include "../inc/grid.h"
#include "../inc/vga_utils.h"

#include <unistd.h>
#include <stdio.h>

#include "system.h"
#include "altera_avalon_uart_regs.h"

/*****************************************************************************
** Initialise touch screen controller
*****************************************************************************/
void Init_Touch(void)
{
  // Program 6850 and baud rate generator to communicate with touchscreen
  // send touchscreen controller an "enable touch" command
  IOWR_ALTERA_AVALON_UART_TXDATA(TOUCHSCREEN_BASE, 0x55);
  IOWR_ALTERA_AVALON_UART_TXDATA(TOUCHSCREEN_BASE, 0x01);
  IOWR_ALTERA_AVALON_UART_TXDATA(TOUCHSCREEN_BASE, 0x12);
}
/*****************************************************************************
** test if screen touched
*****************************************************************************/
int ScreenTouched( void )
{
  if (IORD_ALTERA_AVALON_UART_RXDATA(TOUCHSCREEN_BASE) == 0x81) 
    return 1;
  else
    return 0;

  return 0;
}
/*****************************************************************************
** wait for screen to be touched
*****************************************************************************/
void WaitForTouch(void)
{
  while(!ScreenTouched());
}

Point getTouch(void) {
  Point p1;
  int x = 0;
  int y = 0;

  // Wait for pen down
  while(IORD_ALTERA_AVALON_UART_RXDATA(TOUCHSCREEN_BASE) != 0x81);

  // Wait for pen up
  while(IORD_ALTERA_AVALON_UART_RXDATA(TOUCHSCREEN_BASE)!= 0x80);

  WaitForPacket();
  WaitForPacket();
  char x1 = IORD_ALTERA_AVALON_UART_RXDATA(TOUCHSCREEN_BASE);
  x1 &= 0x7F;
  WaitForPacket();
  char x2 = IORD_ALTERA_AVALON_UART_RXDATA(TOUCHSCREEN_BASE);
  x2 &= 0x1F;
  WaitForPacket();  
  WaitForPacket();  
  char y1 = IORD_ALTERA_AVALON_UART_RXDATA(TOUCHSCREEN_BASE);
  y1 &= 0x7F;
  WaitForPacket();  
  char y2 = IORD_ALTERA_AVALON_UART_RXDATA(TOUCHSCREEN_BASE);
  y1 &= 0x1F;

  x = (x2 << 7);
  x |= x1;
  y = (y2 << 7);
  y |= y1;

  p1.x = x * SCREEN_X/TOUCH_RESOLUTION;
  p1.y = y * SCREEN_Y/TOUCH_RESOLUTION;

  return p1;
}

void WaitForPacket(void) {
  // corresponding wait time for 1 data packet (1 byte) to arrive for the touchscreen at 9600 baud rate
  usleep(834);
}