#ifndef GRID_H
#define GRID_H

#include "./touch.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define NUM_SHIPS   4
#define GRID_SIZE   8

#define BATTLESHIP  4
#define CRUISER     3
#define SUBMARINE   3
#define DESTROYER   2

#define MAX_HITS  12

#define EMPTY   0
#define HIT     1
#define MISS    2
#define SUNK    3

#define ONGOING 0
#define WIN     1
#define LOSE    2

#define SHIP_MARGIN 10
#define STRIKE_MARGIN 18

typedef enum SHIP_PLACEMENT_BUTTONS {
  SPB_NONE        = 0,
  SPB_RESET_ALL   = 1,
  SPB_RESET_CURR  = 2,
  SPB_CONFIRM_SEL = 3
} SHIP_PLACEMENT_BUTTONS;

typedef enum PLAYER_STRIKES_BUTTONS {
  PSB_NONE    = 0,
  PSB_RESET   = 1,
  PSB_CONFIRM = 2
} PLAYER_STRIKES_BUTTONS;

typedef enum OPPONENT_STRIKES_BUTTONS {
  OSB_NONE  = 0,
  OSB_SWAP  = 1,
} OPPONENT_STRIKES_BUTTONS;

SHIP_PLACEMENT_BUTTONS shipPlacementScreen(bool single_player);
OPPONENT_STRIKES_BUTTONS populateOpponentStrikesScreen(bool single_player, int * game_status);
PLAYER_STRIKES_BUTTONS populatePlayerStrikesScreen(bool single_player, int * game_status);
int ** initGrid(void);
void resetGrid(int ** grid);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif