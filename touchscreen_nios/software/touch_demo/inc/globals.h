#ifndef GLOBALS_H
#define GLOBALS_H

#include "./game_ai.h"
#include <stdbool.h>

#include "altera_avalon_uart_regs.h"
#include "altera_up_avalon_video_pixel_buffer_dma.h"
#include "altera_up_avalon_video_character_buffer_with_dma.h"
#include "altera_up_avalon_rs232_regs.h"
#include "altera_up_avalon_rs232.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern alt_up_pixel_buffer_dma_dev * g_pixel_buf_dma_dev;
extern alt_up_char_buffer_dev * g_char_buf_dma_dev;
extern alt_up_rs232_dev * g_wifi_dev;

// Player and UI related globals
extern int ** g_player_grid;        // player's ship placement
extern int ** g_player_strikes;     // player's strikes
extern int ** g_opponent_grid;      // opponent's ship placement
extern int ** g_opponent_strikes;   // opponent's strikes
extern Ship * g_player_ships;       // player's ships
extern Ship * g_opponent_ships;     // opponent's ships

// AI related globals
extern int ** g_player_ai_grid;     // player's ship placement
extern int ** g_player_ai_strikes;  // player's ship placement
extern int ** g_ai_grid;            // AI's ships placement
extern int ** g_ai_strikes;         // AI's strikes
extern Ship * g_player_ai_ships;    // player's ships
extern Ship * g_ai_ships;           // AI's ships
extern Hit * g_ai_hits;             // AI's hits (linked list)
extern int * g_ai_ship_hits;        // number of hits on each AI ship   
extern int * g_player_ship_hits;    // number of hits on each AI ship

extern int g_ai_total_moves;        // count of ai moves 
extern bool g_ai_has_hit;            // boolean whether ai has succesfully hit a ship

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif