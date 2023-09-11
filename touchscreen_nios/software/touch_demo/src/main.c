/* 
 * "Small Hello World" example. 
 * 
 * This example prints 'Hello from Nios II' to the STDOUT stream. It runs on
 * the Nios II 'standard', 'full_featured', 'fast', and 'low_cost' example 
 * designs. It requires a STDOUT  device in your system's hardware. 
 *
 * The purpose of this example is to demonstrate the smallest possible Hello 
 * World application, using the Nios II HAL library.  The memory footprint
 * of this hosted application is ~332 bytes by default using the standard 
 * reference design.  For a more fully featured Hello World application
 * example, see the example titled "Hello World".
 *
 * The memory footprint of this example has been reduced by making the
 * following changes to the normal "Hello World" example.
 * Check in the Nios II Software Developers Manual for a more complete 
 * description.
 * 
 * In the SW Application project (small_hello_world):
 *
 *  - In the C/C++ Build page
 * 
 *    - Set the Optimization Level to -Os
 * 
 * In System Library project (small_hello_world_syslib):
 *  - In the C/C++ Build page
 * 
 *    - Set the Optimization Level to -Os
 * 
 *    - Define the preprocessor option ALT_NO_INSTRUCTION_EMULATION 
 *      This removes software exception handling, which means that you cannot 
 *      run code compiled for Nios II cpu with a hardware multiplier on a core 
 *      without a the multiply unit. Check the Nios II Software Developers 
 *      Manual for more details.
 *
 *  - In the System Library page:
 *    - Set Periodic system timer and Timestamp timer to none
 *      This prevents the automatic inclusion of the timer driver.
 *
 *    - Set Max file descriptors to 4
 *      This reduces the size of the file handle pool.
 *
 *    - Check Main function does not exit
 *    - Uncheck Clean exit (flush buffers)
 *      This removes the unneeded call to exit when main returns, since it
 *      won't.
 *
 *    - Check Don't use C++
 *      This builds without the C++ support code.
 *
 *    - Check Small C library
 *      This uses a reduced functionality C library, which lacks  
 *      support for buffering, file IO, floating point and getch(), etc. 
 *      Check the Nios II Software Developers Manual for a complete list.
 *
 *    - Check Reduced device drivers
 *      This uses reduced functionality drivers if they're available. For the
 *      standard design this means you get polled UART and JTAG UART drivers,
 *      no support for the LCD driver and you lose the ability to program 
 *      CFI compliant flash devices.
 *
 *    - Check Access device drivers directly
 *      This bypasses the device file system to access device drivers directly.
 *      This eliminates the space required for the device file system services.
 *      It also provides a HAL version of libc services that access the drivers
 *      directly, further reducing space. Only a limited number of libc
 *      functions are available in this configuration.
 *
 *    - Use ALT versions of stdio routines:
 *
 *           Function                  Description
 *        ===============  =====================================
 *        alt_printf       Only supports %s, %x, and %c ( < 1 Kbyte)
 *        alt_putstr       Smaller overhead than puts with direct drivers
 *                         Note this function doesn't add a newline.
 *        alt_putchar      Smaller overhead than putchar with direct drivers
 *        alt_getchar      Smaller overhead than getchar with direct drivers
 *
 */

#include "../inc/globals.h"
#include "../inc/vga_utils.h"
#include "../inc/main.h"
#include "../inc/touch.h"
#include "../inc/wifi.h"
#include "../inc/grid.h"
#include "../inc/multiplayer.h"
#include "../inc/game_mode.h"
#include "../inc/join_game.h"
#include "../inc/win_lose.h"
#include "../inc/main_menu.h"
#include "../inc/game_ai.h"

#include "sys/alt_stdio.h"
#include "sys/alt_irq.h"
#include <stdbool.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "system.h"
#include "altera_avalon_uart_regs.h"
#include "altera_up_avalon_video_pixel_buffer_dma.h"
#include "altera_up_avalon_video_character_buffer_with_dma.h"
#include "altera_up_avalon_rs232_regs.h"
#include "altera_up_avalon_rs232.h"

typedef enum GUI {
  NONE,
  MAIN_MENU,
  GAME_MODE,
  JOIN_GAME,
  CREATE_GAME,
  SINGLE_PLAYER,
  MULTI_PLAYER,
  SHIP_PLACEMENT,
  PLAYER_STRIKE,
  OPPONENT_STRIKE,
  WIN_LOSE
} GUI;

alt_up_pixel_buffer_dma_dev * g_pixel_buf_dma_dev;
alt_up_char_buffer_dev * g_char_buf_dma_dev;
alt_up_rs232_dev * g_wifi_dev;

int ** g_player_grid;       
int ** g_player_strikes;    
int ** g_opponent_grid;     
int ** g_opponent_strikes;  
Ship * g_player_ships;  
Ship * g_opponent_ships;

int ** g_player_ai_grid;    
int ** g_player_ai_strikes; 
int ** g_ai_grid;           
int ** g_ai_strikes;   

Ship * g_player_ai_ships;   
Ship * g_ai_ships;          
Hit * g_ai_hits;            
int * g_ai_ship_hits;       
int * g_player_ship_hits;  

int g_ai_total_moves;
bool g_ai_has_hit;    

int main()
{ 
  alt_putstr("Hello from Nios II!\n");

  GUI state = MAIN_MENU;
  bool single_player = false;
  bool game_owner = false;
  int game_status = ONGOING;

  Init_Touch();

  // Globals init
  g_player_grid       = initGrid();
  g_player_strikes    = initGrid();
  g_opponent_grid     = initGrid();
  g_opponent_strikes  = initGrid();
  g_player_ships      = (Ship *) malloc(NUM_SHIPS * sizeof(Ship));
  g_opponent_ships    = (Ship *) malloc(NUM_SHIPS * sizeof(Ship));

  g_player_ai_grid    = initGrid();
  g_player_ai_strikes = initGrid();
  g_ai_grid           = initGrid();
  g_ai_strikes        = initGrid();

  g_player_ai_ships = (Ship *) malloc(NUM_SHIPS * sizeof(Ship)); 
  g_ai_ships = (Ship *) malloc(NUM_SHIPS * sizeof(Ship));
  g_ai_hits = NULL;
  g_ai_ship_hits = (int *) malloc(NUM_SHIPS * sizeof(int));
  g_player_ship_hits = (int *) malloc(NUM_SHIPS * sizeof(int));

  g_ai_total_moves = 0;
  g_ai_has_hit = false;    

  g_pixel_buf_dma_dev = alt_up_pixel_buffer_dma_open_dev("/dev/video_pixel_buffer_dma_0");
  g_char_buf_dma_dev = alt_up_char_buffer_open_dev("/dev/video_character_buffer_with_dma_0");
  g_wifi_dev = alt_up_rs232_open_dev("/dev/wifi_module");

  if (g_pixel_buf_dma_dev == NULL)
	  printf ("Error: could not open pixel buffer device \n");
  else
	  printf ("Opened pixel buffer device \n");

  if (g_char_buf_dma_dev == NULL)
	  printf ("Error: could not open character buffer device \n");
  else
	  printf ("Opened character buffer device \n");

  if(g_wifi_dev == NULL)
	  printf("could not opeen rs232 connection\n");
  else
	  printf("opened rs232 connection\n");
  
  // Clear the screen
  alt_up_pixel_buffer_dma_clear_screen (g_pixel_buf_dma_dev, 1);
  clearScreen(0);
  usleep(1000000);// 1sec

  while (1) {
    switch(state) {
      case MAIN_MENU: {
        game_status = ONGOING;
        mainMenuScreen();
        state = GAME_MODE;
        break;
      }

      case GAME_MODE: {
        GAME_MODE_BUTTONS gmb_button = gameModeMenuScreen();
        if (gmb_button == GMB_SINGLE_PLAYER) {
          state = SINGLE_PLAYER;
        } else if (gmb_button == GMB_MULTI_PLAYER) {
          state = MULTI_PLAYER;
        }
        break;
      }

      case SINGLE_PLAYER: {
        single_player = true;
        state = SHIP_PLACEMENT;
        break;
      }

      case MULTI_PLAYER: {
        single_player = false;
        establishingConnectionScreen();
        usleep(1000000); // 1sec
        MULTI_PLAYER_BUTTONS mpb_button = multiplayerScreen();
        if (mpb_button == MPB_CREATE_GAME) {
          state = CREATE_GAME;
        } else if (mpb_button == MPB_JOIN_GAME) {
          state = JOIN_GAME;
        } 
        break;
      }

      case CREATE_GAME: {
        game_owner = true;
        char game_code[5];
        createGameScreen(game_code);
        state = SHIP_PLACEMENT;
        break;
      }

      case JOIN_GAME: {
        game_owner = false;
        JOIN_GAME_BUTTONS jgb_button = joinGameScreen();
        if (jgb_button == JGB_CONFIRM) {
          state = SHIP_PLACEMENT;
        }
        break;
      }

      case SHIP_PLACEMENT: {
        SHIP_PLACEMENT_BUTTONS spb_button = shipPlacementScreen(single_player);
        if (spb_button == SPB_CONFIRM_SEL) {
          if (game_owner) {
            state = PLAYER_STRIKE;
          } else {
            state = OPPONENT_STRIKE;
          }
        }
        break;        
      }

      case PLAYER_STRIKE: {
        PLAYER_STRIKES_BUTTONS psb_button = populatePlayerStrikesScreen(single_player, &game_status);
        
        if (game_status != ONGOING) {
          state = WIN_LOSE;
          break;
        } 
        
        if (psb_button == PSB_CONFIRM) {
          state = OPPONENT_STRIKE;
        }
        break;
      }

      case OPPONENT_STRIKE: {
        OPPONENT_STRIKES_BUTTONS osb_button = populateOpponentStrikesScreen(single_player, &game_status);
        
        if (game_status != ONGOING) {
          state = WIN_LOSE;
          break;
        } 

        if (osb_button == OSB_SWAP) {
          state = PLAYER_STRIKE;
        }
        break;
      }

      case WIN_LOSE: {
        if (game_status == WIN) {
          winLoseScreen(true);
        } else {
          winLoseScreen(false);
        }
        state = MAIN_MENU;
        break;
      }
    }
  }

  return 0;
}
