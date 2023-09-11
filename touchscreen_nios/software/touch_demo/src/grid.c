#include "../inc/globals.h"
#include "../inc/vga_utils.h"
#include "../inc/grid.h"
#include "../inc/touch.h"
#include "../inc/game_ai.h"
#include "../inc/wifi.h"

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#define LEFT_OFFSET 20
#define TOP_OFFSET  10
#define BOT_OFFSET  10

int player_hits = 0;
int opponent_hits = 0;    
int opponent_sunk_ships = 0;

// Public functions ------------------------------------------------------------
SHIP_PLACEMENT_BUTTONS shipPlacementScreen(bool single_player);
OPPONENT_STRIKES_BUTTONS populateOpponentStrikesScreen(bool single_player, int * game_status);
PLAYER_STRIKES_BUTTONS populatePlayerStrikesScreen(bool single_player, int * game_status);
int ** initGrid(void);
void resetGrid(int ** grid);
void clearGrids(void); 

// Private functions -----------------------------------------------------------
// Grid UI
static void drawGrid(void);

static void drawSPBButtons(void);
static SHIP_PLACEMENT_BUTTONS getSPBButtonPress(Point * p);
static void drawPSBButtons(void);
static PLAYER_STRIKES_BUTTONS getPSBButtonPress(Point * p);
static void drawOSBButtons(void);
static OPPONENT_STRIKES_BUTTONS getOSBButtonPress(Point * p);

static void drawShips(int **grid, Ship * ships);
static void fillGridBox(int x, int y, int left_mar, int right_mar, int top_mar, int bot_mar, int color, int backbuffer);
static void drawStrike(int ** strikes_grid, int x, int y, int backbuffer);
static void drawAllStrikes(int ** strikes_grid);
static void drawWaitingMessageOpponent(int color);
static void drawWaitingMessagePlayerStrike(int color);
static void drawStrikeInstructions(int color);
static void drawPlayerHitOrMiss(int strike, int color);
static void drawOpponentHitOrMiss(int strike, int color);
static void waitingForOpponentScreen(void);
static bool updateOpponentGridAndShips(Ship * sunk_ship);

static Point getGridTouch(Point * touch);
static void fillPossiblePlacements(Point * possible_placements, int color);
static void resetScreen(int ** grid);

// Ship placement logic
static Point * initialPlacement(int ** grid, Point * grid_select, unsigned int ship_size);
static int placeShip(int ** grid, Point * possible_placements, Point * initial_placement, Point * final_placement, int ship_id, int ship_size);
static bool isShipPlacementPossible(int ** grid, Point * grid_select, Point * possible_placement);
static bool possiblePlacementsValid(Point * possible_placements);
static bool pointsAreEqual(Point * p1, Point * p2);


/**
 * @brief Draws a grid with option buttons to allow a user to place ships
 * 
 */
SHIP_PLACEMENT_BUTTONS shipPlacementScreen(bool single_player) {
  clearScreen(0);
  if (single_player) {
    clearAI();
  }
  clearGrids();

  player_hits = 0;
  opponent_hits = 0;
  opponent_sunk_ships = 0;

  int ** grid = g_player_grid;
  resetScreen(grid);
  drawGrid();
  drawSPBButtons();

  int confirm_selection = 0;
  int num_ships = NUM_SHIPS;
  int ships [] = {BATTLESHIP, CRUISER, SUBMARINE, DESTROYER};
  int ship_select = 0;
  
  Point coordinates [num_ships*2];

  Point touch;
  SHIP_PLACEMENT_BUTTONS button;

  while (confirm_selection != 1) {

    while(ship_select != num_ships) {
      touch = getTouch();
      button = getSPBButtonPress(&touch); 
      Point grid_touch_initial = getGridTouch(&touch);

      if (button == SPB_RESET_ALL) {
        resetScreen(grid);
        ship_select = 0;
        continue;
      }
      
      if (grid_touch_initial.x == GRID_SIZE && grid_touch_initial.y == GRID_SIZE) {
        continue;
      }

      if (grid[grid_touch_initial.y][grid_touch_initial.x] != 0) {
        continue;
      }

      Point * possible_placements = initialPlacement(grid, &grid_touch_initial, ships[ship_select]);

      if (possiblePlacementsValid(possible_placements)) {
        fillGridBox(grid_touch_initial.x, grid_touch_initial.y, 1, 1, 1, 1, RED, 0);
        coordinates[2*ship_select] = grid_touch_initial;

        fillPossiblePlacements(possible_placements, GREEN);

        while (true) {
          touch = getTouch();
          button = getSPBButtonPress(&touch); 
          Point grid_touch_final = getGridTouch(&touch);

          if (button == SPB_RESET_CURR) {
            // reset drawings for ship placement
            fillGridBox(grid_touch_initial.x, grid_touch_initial.y, 1, 1, 1, 1, BLACK, 0);
            fillPossiblePlacements(possible_placements, BLACK);
            break;
          } else if (button == SPB_RESET_ALL) {
            // reset drawings for ship placement
            fillGridBox(grid_touch_initial.x, grid_touch_initial.y, 1, 1, 1, 1, BLACK, 0);
            fillPossiblePlacements(possible_placements, BLACK);
            
            resetScreen(grid);
            ship_select = 0;
            break;
          }

          if (placeShip(grid, possible_placements, &grid_touch_initial, &grid_touch_final, ship_select + 1, ships[ship_select]) == 0) {
            // reset drawings for ship placement
            fillPossiblePlacements(possible_placements, BLACK);
            fillGridBox(grid_touch_initial.x, grid_touch_initial.y, 1, 1, 1, 1, BLACK, 0);
            
            drawShips(grid, g_player_ships);
            coordinates[2*ship_select + 1] = grid_touch_final;
            ship_select++;
            break;
          }
        }
      }
      
      free(possible_placements);
    }

    touch = getTouch();
    button = getSPBButtonPress(&touch);

    if (button == SPB_RESET_ALL) {
      resetScreen(grid);
      ship_select = 0;
    } else if (button == SPB_CONFIRM_SEL) {
      confirm_selection = 1;

      if (single_player) {
        initPlayerShips(coordinates, g_player_ai_ships, g_player_ai_grid);
        break;
      } else {
        char response[100];
        setShips(g_wifi_dev, g_player_ships);
        readWIFIResponse(g_wifi_dev, response, RES_COMPLETE);
        if (response[OPCODE] == SHIPS_SET) {
          printf("Player ships set\n");
        } else if (response[OPCODE] == ERROR) {
          printf("Something went wrong setting ships\n");
        }
        break;
      }
    }
  }

  if (single_player) {
    placeAIShips(g_ai_grid, g_ai_ships);
  } else {
    waitingForOpponentScreen();
  }

  return SPB_CONFIRM_SEL;
}

OPPONENT_STRIKES_BUTTONS populateOpponentStrikesScreen(bool single_player, int * game_status) {
  clearScreen(0);
  drawGrid();
  drawShips(g_player_grid, g_player_ships);
  drawAllStrikes(g_opponent_strikes);
  drawOSBButtons();

  // Draw waiting message
  drawWaitingMessageOpponent(MAGENTA);

  usleep(1500000); // simulate some wait time

  Point opponent_strike;
  Point touch;
  OPPONENT_STRIKES_BUTTONS button;

  if (single_player) {
    int ret = nextMove(g_ai_hits, g_ai_strikes, g_player_ai_ships, &opponent_strike);
    if (ret != 0) {
      printf("ERROR: Something went wrong with the AI next move calculation\n");
    }
    aiStrike(&opponent_strike, g_player_ai_grid, g_ai_strikes, g_player_ai_ships, &g_ai_hits);
  } else {
    char response[100];
    readWIFIResponse(g_wifi_dev, response, RES_COMPLETE);
    if (response[OPCODE] == OPPONENT_FIRED) {
      opponent_strike.x = response[FIRED_X] - '0';
      opponent_strike.y = response[FIRED_Y] - '0';
    }
  }

  // Clear waiting message
  drawWaitingMessageOpponent(BLACK);
  usleep(200000);

  if (g_player_grid[opponent_strike.y][opponent_strike.x] != 0) {
    g_opponent_strikes[opponent_strike.y][opponent_strike.x] = HIT;
    opponent_hits++;
    drawOpponentHitOrMiss(HIT, MAGENTA);
  } else {
    g_opponent_strikes[opponent_strike.y][opponent_strike.x] = MISS;
    drawOpponentHitOrMiss(MISS, MAGENTA);
  }

  usleep(100000);
  drawStrike(g_opponent_strikes, opponent_strike.x, opponent_strike.y, 0);

  if (opponent_hits == MAX_HITS) {
    usleep(1500000); // wait before win/lose screen
    leaveRoom(g_wifi_dev);
    *game_status = LOSE;

    return OSB_NONE;
  }

  while (true) {
    touch = getTouch();
    button = getOSBButtonPress(&touch);

    if (button == OSB_SWAP) {
      return OSB_SWAP;
    }
  }
}

PLAYER_STRIKES_BUTTONS populatePlayerStrikesScreen(bool single_player, int * game_status) {
  clearScreen(0);
  drawGrid();
  drawShips(g_opponent_grid, g_opponent_ships);
  drawAllStrikes(g_player_strikes);
  drawPSBButtons();
  drawStrikeInstructions(MAGENTA);

  bool strike_selected = false;
  Point touch;
  Point grid_touch;
  PLAYER_STRIKES_BUTTONS button;

  while (true) {
    // Get user input
    touch = getTouch();

    if (strike_selected == false) {
      grid_touch = getGridTouch(&touch);

      if (g_player_strikes[grid_touch.y][grid_touch.x] != EMPTY) {
        continue;
      }

      fillGridBox(grid_touch.x, grid_touch.y, 1, 1, 1, 1, RED, 0);
      strike_selected = true;
    } 
    
    else {
      button = getPSBButtonPress(&touch);

      if (button == PSB_RESET) {
        fillGridBox(grid_touch.x, grid_touch.y, 1, 1, 1, 1, BLACK, 0);
        strike_selected = false;
      } else if (button == PSB_CONFIRM) {
        int strike;

        // clear the instructions
        drawStrikeInstructions(BLACK);

        // Draw waiting message
        drawWaitingMessagePlayerStrike(MAGENTA);
        usleep(600000); // simulate some wait time

        // Get hit or miss from wifi or AI
        if (single_player) {
          Ship * sunk_ship;          
          strike = playerStrike(g_ai_grid, g_player_ai_strikes, g_ai_ships, &grid_touch, &sunk_ship);
          
          if (strike == HIT) {
            g_player_strikes[grid_touch.y][grid_touch.x] = HIT;
            player_hits++;
          } else if (strike == MISS) {
            g_player_strikes[grid_touch.y][grid_touch.x] = MISS;
          }

          fillGridBox(grid_touch.x, grid_touch.y, 1, 1, 1, 1, BLACK, 0);

          if (strike == HIT) {
            bool update = updateOpponentGridAndShips(sunk_ship);
            if (update) {
              drawShips(g_opponent_grid, g_opponent_ships);
              drawAllStrikes(g_player_strikes);
            } else {
              drawStrike(g_player_strikes, grid_touch.x, grid_touch.y, 0);
            }
          } else {
            drawStrike(g_player_strikes, grid_touch.x, grid_touch.y, 0);
          }

          // Clear waiting message
          drawWaitingMessagePlayerStrike(BLACK);

          drawPlayerHitOrMiss(strike, MAGENTA);
          usleep(1000000); // simulate some wait time

        } else {
          Ship sunk_ship;
          fire(g_wifi_dev, &grid_touch);
          
          char response[100];
          readWIFIResponse(g_wifi_dev, response, RES_COMPLETE);

          if (response[OPCODE] == FIRED) {
            if (response[FIRED_HIT] == '1') {
              strike = HIT;
              g_player_strikes[grid_touch.y][grid_touch.x] = HIT;
              player_hits++;

              if (response[FIRED_SUNK] == '1') {
                sunk_ship.startAddress.x = response[FIRED_SX] - '0';
                sunk_ship.startAddress.y = response[FIRED_SY] - '0';
                sunk_ship.endAddress.x = response[FIRED_EX] - '0';
                sunk_ship.endAddress.y = response[FIRED_EY] - '0';
                sunk_ship.id = opponent_sunk_ships + 1;
                sunk_ship.size = 0; // don't need ship size for multiplayer
                sunk_ship.sunk = true;

                opponent_sunk_ships++;
              } else {
                sunk_ship.sunk = false;
              }
            } else {
              strike = MISS;
              g_player_strikes[grid_touch.y][grid_touch.x] = MISS;
            }
          }

          fillGridBox(grid_touch.x, grid_touch.y, 1, 1, 1, 1, BLACK, 0);

          if (strike == HIT) {
            if (response[FIRED_SUNK] == '1') {              
              bool update = updateOpponentGridAndShips(&sunk_ship);
              if (update) {
                drawShips(g_opponent_grid, g_opponent_ships);
                drawAllStrikes(g_player_strikes);
              } 
            } else {
              drawStrike(g_player_strikes, grid_touch.x, grid_touch.y, 0);
            }
          } else {
            drawStrike(g_player_strikes, grid_touch.x, grid_touch.y, 0);
          }

          // Clear waiting message
          drawWaitingMessagePlayerStrike(BLACK);

          drawPlayerHitOrMiss(strike, MAGENTA);
          usleep(1000000); // simulate some wait time
        }

        break;
      }
    }
  }

  if (player_hits == MAX_HITS) {
    usleep(1500000); // wait before win/lose screen
    leaveRoom(g_wifi_dev);
    *game_status = WIN;

    return OSB_NONE;
  }

  return PSB_CONFIRM;
}

/**
 * @brief Return a memory allocated 2d array of GRID_SIZE x GRID_SIZE
 * 
 * @return int** pointer to the 2d array (grid)
 */
int ** initGrid(void) {
  int ** grid = (int **) malloc(GRID_SIZE * sizeof(int *));

  for (int i = 0; i < GRID_SIZE; i++) {
    grid[i] = (int *) malloc(GRID_SIZE * sizeof(int));
  }

  return grid;
}

/**
 * @brief set all the elements in the grid matrix to 0 
 * 
 * @param grid pointer to the grid matrix
 */
void resetGrid(int ** grid) {
  for (int i = 0; i < GRID_SIZE; i++) {
    for (int j = 0; j < GRID_SIZE; j++) {
      grid[i][j] = 0;
    }
  }
}

void clearGrids(void) {
  resetGrid(g_player_grid);
  resetGrid(g_player_strikes);
  resetGrid(g_opponent_grid);
  resetGrid(g_opponent_strikes);
}

/**
 * @brief Draws a grid with size GRID_SIZE x GRID_SIZE
 * 
 */
static void drawGrid(void) {
  int y_top = TOP_OFFSET;
  int y_bot = SCREEN_Y - BOT_OFFSET;
  int grid_height = y_bot - y_top;
  int grid_width = grid_height * SCREEN_X_SCALE;

  //draw vertical lines
	for (int i = 0; i <= GRID_SIZE; i++) {
    int x = LEFT_OFFSET + i * grid_width/GRID_SIZE;
		drawVline(x, y_top, y_bot, WHITE, 0);
	}

	//draw horizontal lines
	for (int i = 0; i <= GRID_SIZE; i++) {
    int y = y_top + i * grid_height/GRID_SIZE;
		drawHline(LEFT_OFFSET, LEFT_OFFSET + grid_width, y, WHITE, 0);
	} 
}

static void drawSPBButtons(void) {
  drawStringPixels("Place 4 Ships by", 440, 50, WHITE);
  drawStringPixels("tapping on the grid", 420, 70, WHITE);

  // Reset grid
  drawString("Reset Grid", 510, 170);
  drawFilledBox(480, 150, 620, 200, RED, 0);

  // Reset current placement
  drawString("Reset Current", 500, 255);
  drawString("Placement", 515, 290);
  drawEmptyBox(480, 240, 620, 310, WHITE, 0);

  // Confirm placement
  drawString("Confirm", 520, 370);
  drawFilledBox(480, 350, 620, 400, GREEN, 0);
}

static SHIP_PLACEMENT_BUTTONS getSPBButtonPress(Point * p) {
  if (p->x >= 480 && p->x <= 620) {
    // Reset grid button
    if (p->y >= 150 && p->y <= 200) {
      return SPB_RESET_ALL;
    }

    // Reset current ship button
    else if (p->y >= 240 && p->y <= 310) {
      return SPB_RESET_CURR;
    }

    // Confirm selection button
    else if (p->y >= 350 && p->y <= 400) {
      return SPB_CONFIRM_SEL;
    }
  }

  return SPB_NONE;
}

static void drawPSBButtons(void) {
  // Reset Strike
  drawString("Reset Strike", 505, 235);
  drawEmptyBox(480, 200, 620, 280, WHITE, 0);

  // Confirm Strike
  drawString("Confirm Strike", 500, 355);
  drawFilledBox(480, 320, 620, 400, GREEN, 0);
}

static PLAYER_STRIKES_BUTTONS getPSBButtonPress(Point * p) {
  if (p->x >= 480 && p->x <= 620) {
    // Reset Strike
    if (p->y >= 200 && p->y <= 280) {
      return PSB_RESET;
    }

    // Confirm Strike
    else if (p->y >= 320 && p->y <= 400) {
      return PSB_CONFIRM;
    }
  }

  return PSB_NONE;
}

static void drawOSBButtons(void) {
  drawString("Go to Strike", 505, 320);
  drawString("Screen", 530, 350);
  drawEmptyBox(480, 290, 620, 390, WHITE, 0);
}

static OPPONENT_STRIKES_BUTTONS getOSBButtonPress(Point * p) {
  if (p->x >= 480 && p->x <= 620 && p->y >= 290 && p->y <= 390) {
    return OSB_SWAP;
  }
  return OSB_NONE;
}

static void drawShips(int **grid, Ship * ships) {
  for (int y = 0; y < GRID_SIZE; y++) {
    for (int x = 0; x < GRID_SIZE; x++) {
      if (grid[y][x] != 0) {
        int ship_id = grid[y][x];

        // check if ship is vertical
        Ship ship = ships[ship_id - 1];
        bool vert = ship.startAddress.x == ship.endAddress.x;

        if (vert) {
          // detect the vertical ends of the ship
          int start_y = ship.startAddress.y < ship.endAddress.y ? ship.startAddress.y : ship.endAddress.y;
          int end_y = ship.startAddress.y > ship.endAddress.y ? ship.startAddress.y : ship.endAddress.y;

          if (y == start_y) {
            fillGridBox(x, y, SHIP_MARGIN*SCREEN_X_SCALE, SHIP_MARGIN*SCREEN_X_SCALE, SHIP_MARGIN, 1, GREY, 0);
          } else if (y == end_y) {
            fillGridBox(x, y, SHIP_MARGIN*SCREEN_X_SCALE, SHIP_MARGIN*SCREEN_X_SCALE, 1, SHIP_MARGIN, GREY, 0);
          } else {
            fillGridBox(x, y, SHIP_MARGIN*SCREEN_X_SCALE, SHIP_MARGIN*SCREEN_X_SCALE, 1, 1, GREY, 0);
          }
        } else {
          // detect the horizontal ends of the ship
          int start_x = ship.startAddress.x < ship.endAddress.x ? ship.startAddress.x : ship.endAddress.x;
          int end_x = ship.startAddress.x > ship.endAddress.x ? ship.startAddress.x : ship.endAddress.x;
          
          if (x == start_x) {
            fillGridBox(x, y, SHIP_MARGIN, 1, SHIP_MARGIN, SHIP_MARGIN, GREY, 0);
          } else if (x == end_x) {
            fillGridBox(x, y, 1, SHIP_MARGIN, SHIP_MARGIN, SHIP_MARGIN, GREY, 0);
          } else {
            fillGridBox(x, y, 1, 1, SHIP_MARGIN, SHIP_MARGIN, GREY, 0);
          }
        }
      }
    }
  }
}

/**
 * @brief Draw a filled box inside a grid square
 * 
 * @param x x-coordinate
 * @param y y-coordinate
 * @param left_mar left margin for box infill
 * @param right_mar right margin for box infill
 * @param top_mar top margin for box infill
 * @param bot_mar bottom margin for box infill
 * @param color 
 * @param backbuffer 
 */
static void fillGridBox(int x, int y, int left_mar, int right_mar, int top_mar, int bot_mar, int color, int backbuffer) {
  int y_top = TOP_OFFSET;
  int y_bot = SCREEN_Y - BOT_OFFSET;
  int grid_height = y_bot - y_top;
  int grid_width = grid_height * SCREEN_X_SCALE;

  int left_bound   = LEFT_OFFSET + x * grid_width/GRID_SIZE + left_mar;
  int right_bound  = LEFT_OFFSET + (x + 1) * grid_width/GRID_SIZE - right_mar;
  int top_bound    = TOP_OFFSET + y * grid_height/GRID_SIZE + top_mar;
  int bot_bound    = TOP_OFFSET + (y + 1) * grid_height/GRID_SIZE - bot_mar;

  drawFilledBox(left_bound, top_bound, right_bound, bot_bound, color, backbuffer);
}

static void drawStrike(int ** strikes_grid, int x, int y, int backbuffer) {
  if (strikes_grid[y][x] == HIT) {
    fillGridBox(x, y, STRIKE_MARGIN*SCREEN_X_SCALE, STRIKE_MARGIN*SCREEN_X_SCALE, STRIKE_MARGIN, STRIKE_MARGIN, LIGHT_RED, backbuffer);
  } else if (strikes_grid[y][x] == MISS) {
    fillGridBox(x, y, STRIKE_MARGIN*SCREEN_X_SCALE, STRIKE_MARGIN*SCREEN_X_SCALE, STRIKE_MARGIN, STRIKE_MARGIN, LIGHT_BLUE, backbuffer);
  }
}

static void drawAllStrikes(int ** strikes_grid) {
  for (int y = 0; y < GRID_SIZE; y++) {
    for (int x = 0; x < GRID_SIZE; x++) {
      drawStrike(strikes_grid, x, y, 0);
    }
  }
}

static void drawWaitingMessageOpponent(int color) {
  drawStringPixels("Waiting for", 450, 120, color);
  drawStringPixels("opponent...", 450, 150, color);
}

static void drawWaitingMessagePlayerStrike(int color) {
  drawStringPixels("Waiting for", 450, 90, color);
  drawStringPixels("Confirmation", 445, 120, color);
}

static void drawStrikeInstructions(int color) {
  drawStringPixels("Click on a square", 440, 90, color);
  drawStringPixels("to strike the ", 440, 120, color);
  drawStringPixels("opponent", 440, 150, color);
}

static void drawPlayerHitOrMiss(int strike, int color) {
  if (strike == HIT) {
    drawStringPixels("You HIT a ship!", 450, 120, color);
  } else if (strike == MISS) {
    drawStringPixels("You MISSED!", 470, 120, color);
  }
}

static void drawOpponentHitOrMiss(int strike, int color) {
  if (strike == HIT) {
    drawStringPixels("The Opponent HIT", 440, 120, color);
    drawStringPixels("your ship!", 440, 150, color);
  } else if (strike == MISS) {
    drawStringPixels("The Opponent MISSED!", 415, 120, color);
  }
}

static void waitingForOpponentScreen(void) {
  clearScreen(0);
  usleep(200000);

  drawStringPixels("Waiting for the", 230, 180, WHITE);
  drawStringPixels("opponent to place", 230, 210, WHITE);
  drawStringPixels("their ships", 230, 240, WHITE);

  char response[100];
  readWIFIResponse(g_wifi_dev, response, RES_COMPLETE);
  if (response[OPCODE] == START_GAME) {
    printf("Received start game signal from opponent\n");
  } else if (response[OPCODE] == ERROR) {
    printf("Error: %s\n", response);
  }
}

static bool updateOpponentGridAndShips(Ship * sunk_ship) {
  if (sunk_ship == NULL) {
    return false;
  }

  Point start = sunk_ship->startAddress;
  Point end = sunk_ship->endAddress;
  int ship_id = sunk_ship->id;

  bool vert = start.x == end.x;

  if (vert) {
    int start_y = start.y < end.y ? start.y : end.y;
    int end_y = start.y > end.y ? start.y : end.y;

    for (int i = start_y; i <= end_y; i++) {
      g_opponent_grid[i][start.x] = sunk_ship->id;
    }
  } else {
    int start_x = start.x < end.x ? start.x : end.x;
    int end_x = start.x > end.x ? start.x : end.x;

    for (int i = start_x; i <= end_x; i++) {
      g_opponent_grid[start.y][i] = sunk_ship->id;
    }
  }

  g_opponent_ships[ship_id - 1].startAddress = sunk_ship->startAddress;
  g_opponent_ships[ship_id - 1].endAddress   = sunk_ship->endAddress;
  g_opponent_ships[ship_id - 1].size         = sunk_ship->size;
  g_opponent_ships[ship_id - 1].id           = sunk_ship->id;
  g_opponent_ships[ship_id - 1].sunk         = sunk_ship->sunk;

  return true;
}

/**
 * Function assumes that the player and opponent grid will be placed on the coordinates
 * on the screen.
 * 
 * @param touch the touchscreen x,y coordinates 
 * 
 * @returns the coordinates of the square in the grid that the player touched
 * Return constraints: 0 <= x <= GRID_SIZE, 0 <= y <= GRID_SIZE
*/
static Point getGridTouch(Point * touch) {
  Point ret = {GRID_SIZE, GRID_SIZE}; // return GRID_SIZE, GRID_SIZE to show invalid touch input for the grid

  int y_top = TOP_OFFSET;
  int y_bot = SCREEN_Y - BOT_OFFSET;
  int grid_height = y_bot - y_top;
  int grid_width = grid_height * SCREEN_X_SCALE;

  for (int row = 0; row < GRID_SIZE; row++) {
    for (int col = 0; col < GRID_SIZE; col++) {
      int left_bound   = LEFT_OFFSET + col * grid_width/GRID_SIZE;
      int right_bound  = LEFT_OFFSET + (col + 1) * grid_width/GRID_SIZE;
      int top_bound    = TOP_OFFSET + row * grid_height/GRID_SIZE;
      int bot_bound    = TOP_OFFSET + (row + 1) * grid_height/GRID_SIZE;

      if (touch->x >= left_bound && touch->x <= right_bound) {
        if (touch->y >= top_bound && touch->y <= bot_bound) {
          ret.x = col;
          ret.y = row;
          break;
        }
      }
    }
  }

  return ret;
}

static void fillPossiblePlacements(Point * possible_placements, int color) {
  for (int i = 0; i < 4; i++) {
    if (possible_placements[i].x == GRID_SIZE && possible_placements[i].y == GRID_SIZE) {
      continue;
    }
    fillGridBox(possible_placements[i].x, possible_placements[i].y, 1, 1, 1, 1, color, 0);
  }
}

/**
 * @brief set the grid matrix and re-draw the grid screen
 * 
 * @param grid pointer to the grid matrix
 */
static void resetScreen(int ** grid) {
  for (int x = 0; x < GRID_SIZE; x++) {
    for (int y = 0; y < GRID_SIZE; y++) {
      if (grid[y][x])
        fillGridBox (x, y, 1, 1, 1, 1, BLACK, 0);
    }
  }

  resetGrid(grid);
}

/**
 * @brief Get the initial placement of a ship and return the surrounding possible endpoints
 * 
 * @param grid Pointer to a grid
 * @param grid_select initial placement selection by the user
 * @param ship_size size of the ship being placed
 * @return an array of Points that represent possible placements for the ship's other end
 */
static Point * initialPlacement(int ** grid, Point * grid_select, unsigned int ship_size) {
  Point * possible_placements = (Point *) malloc(sizeof(Point) * 4);

  Point up    = {grid_select->x, grid_select->y - ship_size + 1};
  Point down  = {grid_select->x, grid_select->y + ship_size - 1};
  Point left  = {grid_select->x - ship_size + 1, grid_select->y};
  Point right = {grid_select->x + ship_size - 1, grid_select->y};

  if (isShipPlacementPossible(grid, grid_select, &up))
    possible_placements[0] = up;
  else {
    possible_placements[0].x = GRID_SIZE;
    possible_placements[0].y = GRID_SIZE;
  }

  if (isShipPlacementPossible(grid, grid_select, &down))
    possible_placements[1] = down;
  else {
    possible_placements[1].x = GRID_SIZE;
    possible_placements[1].y = GRID_SIZE;
  }

  if (isShipPlacementPossible(grid, grid_select, &left))
    possible_placements[2] = left;
  else {
    possible_placements[2].x = GRID_SIZE;
    possible_placements[2].y = GRID_SIZE;
  }

  if (isShipPlacementPossible(grid, grid_select, &right))
    possible_placements[3] = right;
  else {
    possible_placements[3].x = GRID_SIZE;
    possible_placements[3].y = GRID_SIZE;
  }

  return possible_placements;
}

/**
 * @brief Place the ship on the grid if it is possible to do so. The grid is mutated on 
 * successful ship placement
 * 
 * @param grid pointer to the grid
 * @param possible_placements possible ship placements in comparison to initial_placement
 * @param initial_placement initial placement of the ship (one end of the ship)
 * @param final_placement the final placemnet of the ship (the other end of the ship)
 * @return 0 on successful ship placement, 1 on failure
 */
static int placeShip(int ** grid, Point * possible_placements, Point * initial_placement, Point * final_placement, int ship_id, int ship_size) {
  if (final_placement->x == GRID_SIZE && final_placement->y == GRID_SIZE) {
    return -1;
  }

  for (int i = 0; i < 4; i++) {
    if (pointsAreEqual(&possible_placements[i], final_placement)) {
      // Left
      if (final_placement->x < initial_placement->x) {
        for (int i = final_placement->x; i <= initial_placement->x; i++) {
          grid[initial_placement->y][i] = ship_id;
        }
      }

      // Right
      else if (final_placement->x > initial_placement->x) {
        for (int i = initial_placement->x; i <= final_placement->x; i++) {
          grid[initial_placement->y][i] = ship_id;
        }
      }

      // Up
      else if (final_placement->y < initial_placement->y) {
        for (int i = final_placement->y; i <= initial_placement->y; i++) {
          grid[i][initial_placement->x] = ship_id;
        }
      }

      // Down
      else if (final_placement->y > initial_placement->y) {
        for (int i = initial_placement->y; i <= final_placement->y; i++) {
          grid[i][initial_placement->x] = ship_id;
        }
      }

      g_player_ships[ship_id - 1].startAddress = *initial_placement;
      g_player_ships[ship_id - 1].endAddress = *final_placement;
      g_player_ships[ship_id - 1].size = ship_size;
      g_player_ships[ship_id - 1].id = ship_id;
      g_player_ships[ship_id - 1].sunk = false;

      return 0;
    }
  }

  return -1;
}

/**
 * @brief Check if a give possible_placement can be placed in the grid or not
 * 
 * @param grid Current grid configuration
 * @param grid_select First point of the ship (1st endpoint)
 * @param possible_placement Second endpoint of the ship
 * @return true if possible_placement is valid
 * @return false otherwise
 */
static bool isShipPlacementPossible(int ** grid, Point * grid_select, Point * possible_placement) {
  if (possible_placement->x < 0 || possible_placement->x >= GRID_SIZE) return false;
  if (possible_placement->y < 0 || possible_placement->y >= GRID_SIZE) return false;
  
  // Left
  if (possible_placement->x < grid_select->x) {
    for (int i = possible_placement->x; i <= grid_select->x; i++) {
      if (grid[grid_select->y][i]) 
        return false;
    }
  }

  // Right
  else if (possible_placement->x > grid_select->x) {
    for (int i = grid_select->x; i <= possible_placement->x; i++) {
      if (grid[grid_select->y][i]) 
        return false;
    }
  }

  // Up
  else if (possible_placement->y < grid_select->y) {
    for (int i = possible_placement->y; i <= grid_select->y; i++) {
      if (grid[i][grid_select->x])
        return false;
    }
  }

  // Down
  else if (possible_placement->y > grid_select->y) {
    for (int i = grid_select->y; i <= possible_placement->y; i++) {
      if (grid[i][grid_select->x])
        return false;
    }
  }

  return true;
}

/**
 * @brief Check if the possible_placements array has any valid ship placement points 
 * 
 * @param possible_placements an array of 4 possible ship placement points
 * @return true if there is at least 1 placement that is possible
 * @return false if all placements are not possible
 */
static bool possiblePlacementsValid(Point * possible_placements) {
  int badPlacementCount = 0;

  for (int i = 0; i < 4; i++) {
    if (possible_placements[i].x == GRID_SIZE && possible_placements[i].y == GRID_SIZE) {
      badPlacementCount++;
    }
  }

  if (badPlacementCount == 4) {
    return false;
  }

  return true;
}

/**
 * @brief Check if 2 Points are equal
 * 
 * @param p1 
 * @param p2 
 * @return true if equal
 * @return false otherwise
 */
static bool pointsAreEqual(Point * p1, Point * p2) {
  if (p1->x == p2->x && p1->y == p2->y) {
    return true;
  }

  return false;
}

