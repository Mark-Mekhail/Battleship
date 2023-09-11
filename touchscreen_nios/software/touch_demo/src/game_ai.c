#include "../inc/globals.h"
#include "../inc/game_ai.h"
#include "../inc/touch.h"
#include "../inc/grid.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
 

// Public Functions
int nextMove (Hit * curHit, int ** aiStrikes, Ship * playerShips, Point *nextMove);
void placeAIShips(int **aiShipsGrid, Ship *aiShips);
void initPlayerShips(Point * shipCoords, Ship * playerShips, int ** playerShipsGrid);
int aiStrike(Point * strike, int ** playerShipsGrid, int ** aiStrikes, Ship * playerShips, Hit ** head);
int playerStrike(int ** aiShipsGrid, int ** playersStrikes, Ship * aiShips, Point * target, Ship ** retShip);
void clearAI(void);

// Private Functions
static int nextMoveOpenHit(Hit * curHit, int ** aiStrikes, Ship * playerShips, Point *nextMove);
static int nextMoveNoHit(int ** aiStrikes, Ship * playerShips, Point *nextMove);
static int nextMoveRandomSearch(int ** aiStrikes, Point *nextMove);
static Hit * createHit(Point * pos);
static Hit * deleteHitsWithSunkShip(Hit * head, Ship * sunkPlayerShip, int ** aiStrikes);
static void insertHit(Hit ** head, Hit * newHit);
static void deleteAllHits(Hit * head);


// random placement for pieces? ... investigate other options for piece placement
// maintain list of hits. if hit only check neighboring location for hits

// iterates through entire board for each ship
// if open space check if placement valid for ship size

int nextMove (Hit * curHit, int ** aiStrikes, Ship * playerShips, Point *nextMove) {
  if (curHit->next != NULL) {
    // printf("curHit: x: %d, y: %d\n", curHit->pos.x, curHit->pos.y);
    return nextMoveOpenHit(curHit, aiStrikes, playerShips, nextMove);
  }
  else {
    // printf("curHit is NULL\n");
    return nextMoveNoHit(aiStrikes, playerShips, nextMove);   
  }
}

/**
 * @brief Randomly places ships on the grid
 * 
 * @param aiShipsGrid a 2d matrix for the AI ships grid
 * @param aiShips returns the ships placed
 * @return the ships array is populated with the ships placed
 */
void placeAIShips(int **aiShipsGrid, Ship *aiShips) {
  int shipSizes[] = {2, 3, 3, 4};

  for (int i = 0; i < NUM_SHIPS; i++) {
    bool placed = false;
    int size = shipSizes[i];

    while (!placed) {
      int x = rand() % GRID_SIZE;
      int y = rand() % GRID_SIZE;
      int dir = rand() % 2;

      // horizontal
      if (dir == 0) {
        if (x + size > GRID_SIZE) continue;
        bool valid = true;
        for (int j = 0; j < size; j++) {
          if (aiShipsGrid[y][x + j] != 0) {
            valid = false;
            break;
          }
        }
        if (valid) {
          aiShips[i].startAddress.x = x;
          aiShips[i].startAddress.y = y;
          aiShips[i].endAddress.x = x + size - 1;
          aiShips[i].endAddress.y = y;
          aiShips[i].id = i + 1;
          aiShips[i].size = size;
          aiShips[i].sunk = false;

          for (int j = 0; j < size; j++) {
            aiShipsGrid[y][x + j] = i + 1;
          }
          placed = true;
        }
      } 
      
      // vertical
      else {
        if (y + size > GRID_SIZE) continue;
        bool valid = true;
        for (int j = 0; j < size; j++) {
          if (aiShipsGrid[y + j][x] != 0) {
            valid = false;
            break;
          }
        }
        if (valid) {
          aiShips[i].startAddress.x = x;
          aiShips[i].startAddress.y = y;
          aiShips[i].endAddress.x = x;
          aiShips[i].endAddress.y = y + size - 1;
          aiShips[i].id = i + 1;
          aiShips[i].size = size;
          aiShips[i].sunk = false;

          for (int j = 0; j < size; j++) {
            aiShipsGrid[y + j][x] = i + 1;
          }
          placed = true;
        }
      }
    }
  }
}

/**
 * @brief Fills the player's grid with the ships they have placed
 * 
 * @param shipCoords Player's ship coordinates (mem allocated array of NUM_SHIPS*2 points)
 * @param playerShips Player's ships (mem allocated array of NUM_SHIPS ships)
 * @param playerShipsGrid a mem allocated 2d grid to store the player's ships
 * @return ships and grid are populated
 */
void initPlayerShips(Point * shipCoords, Ship * playerShips, int ** playerShipsGrid) {
  for (int i = 0; i < NUM_SHIPS; i++) {
    int shipID = i + 1;
    bool vert = shipCoords[2*i].x == shipCoords[2*i + 1].x;

    if (vert) {
      int startY = shipCoords[2*i].y < shipCoords[2*i + 1].y ? shipCoords[2*i].y : shipCoords[2*i + 1].y;
      int endY = shipCoords[2*i].y > shipCoords[2*i + 1].y ? shipCoords[2*i].y : shipCoords[2*i + 1].y;
      playerShips[i].size = endY - startY + 1;

      for (int j = startY; j <= endY; j++) {
        playerShipsGrid[j][shipCoords[2*i].x] = shipID;
      }
    } else {
      int startX = shipCoords[2*i].x < shipCoords[2*i + 1].x ? shipCoords[2*i].x : shipCoords[2*i + 1].x;
      int endX = shipCoords[2*i].x > shipCoords[2*i + 1].x ? shipCoords[2*i].x : shipCoords[2*i + 1].x;
      playerShips[i].size = endX - startX + 1;

      for (int j = startX; j <= endX; j++) {
        playerShipsGrid[shipCoords[2*i].y][j] = shipID;
      }
    }

    playerShips[i].startAddress.x = shipCoords[2*i].x;
    playerShips[i].startAddress.y = shipCoords[2*i].y;
    playerShips[i].endAddress.x = shipCoords[2*i+1].x;
    playerShips[i].endAddress.y = shipCoords[2*i+1].y;
    playerShips[i].id = shipID;
    playerShips[i].sunk = false;
  }
}

/**
 * @brief Checks if the AI has hit a ship or not
 * 
 * @param strike coordinates of the strike
 * @param playerShipsGrid the player's ship placement grid
 * @param aiStrikes the AI's strike tracking grid
 * @param head the head of the linked list of hits
 * @return HIT if hit, MISS if miss
 */
int aiStrike(Point * strike, int ** playerShipsGrid, int ** aiStrikes, Ship * playerShips, Hit ** head) {
  if (playerShipsGrid[strike->y][strike->x] != 0) {
    int shipID = playerShipsGrid[strike->y][strike->x];

    Ship * ship;
    for (int i = 0; i < NUM_SHIPS; i++) {
      if (playerShips[i].id == shipID) {
        ship = &playerShips[i];
        break;
      }
    }   

    g_player_ship_hits[shipID - 1]++;
    bool sunk = g_player_ship_hits[shipID - 1] == ship->size;

    if (sunk) {
      aiStrikes[strike->y][strike->x] = SUNK;
      *head = deleteHitsWithSunkShip(*head, ship, aiStrikes);
      return SUNK;
    } else {
      aiStrikes[strike->y][strike->x] = HIT;
      Hit * hit = createHit(strike);
      insertHit(head, hit);
      g_ai_has_hit = true;
      return HIT;
    }    
  }

  aiStrikes[strike->y][strike->x] = MISS;
  return MISS;
}

// Returns HIT if hit ship, MISS if hit water, or -1 if the location was invalid
// aiShipsGrid = aiGrid, ships = aiShips, target returns ship coordinates and id when sunk, x = x coord, y = y coord
int playerStrike(int ** aiShipsGrid, int ** playersStrikes, Ship * aiShips, Point * target, Ship ** retShip) {
  if (playersStrikes[target->y][target->x] == 0) {
    playersStrikes[target->y][target->x] = 1;

    int shipID = aiShipsGrid[target->y][target->x];

    if (shipID != 0) {
      Ship * ship;
      for (int i = 0; i < NUM_SHIPS; i++) {
        if (aiShips[i].id == shipID) {
          ship = &aiShips[i];
          break;
        }
      }    

      g_ai_ship_hits[shipID - 1]++;
      bool sunk = g_ai_ship_hits[shipID - 1] == ship->size;

      if (sunk) {
        ship->sunk = true;
        *retShip = &aiShips[shipID - 1];
      } else {
        *retShip = NULL;
      }
    }    

    return shipID == 0 ? MISS : HIT;
  }

  return -1;
}

void clearAI(void) {
  srand(time(NULL));
  resetGrid(g_player_ai_grid);
  resetGrid(g_player_ai_strikes);
  resetGrid(g_ai_grid);
  resetGrid(g_ai_strikes);

  deleteAllHits(g_ai_hits);
  g_ai_total_moves = 0;
  g_ai_has_hit = 0;

  for (int i = 0; i < NUM_SHIPS; i++) {
    g_ai_ship_hits[i] = 0;
    g_player_ship_hits[i] = 0;
  }  

  Point dummyHead;
  dummyHead.x = GRID_SIZE;  
  dummyHead.y = GRID_SIZE;  
  Hit * initDummyHead = createHit(&dummyHead);
  insertHit(&g_ai_hits, initDummyHead);
  initDummyHead->next = NULL;
}

/**
 * @brief Determine the AI's next move based on the current state of the game
 * 
 * @param curHit pointer to the Hits tracking array
 * @param aiStrikes AI's strike tracking grid
 * @param playerShips Player's ship placement grid
 * @param nextMove return the next move
 * @return 0 on success, -1 if no move is found
 */
static int nextMoveOpenHit(Hit * curHit, int ** aiStrikes, Ship * playerShips, Point *nextMove) {
  int prob [GRID_SIZE][GRID_SIZE] = {0};
  int highestCount = 0;
  int bestMoveX = GRID_SIZE;
  int bestMoveY = GRID_SIZE;
  int row;
  int col;
  int colOff;
  int rowOff;
  int shipSize;
  
  //check possible ship positioning inline with hit
  while(curHit->next != NULL) {
    row = curHit->pos.y; 
    col = curHit->pos.x; 
    for (int i = 0; i < NUM_SHIPS; i ++) {
      if (playerShips[i].sunk) {
        continue;
      }

      shipSize = playerShips[i].size;

      // check hor
      colOff = col - (shipSize - 1); // lower limit to check
      while(colOff <= col) {
        if(colOff < 0) {
          colOff++;
          continue;
        }
        for(int j = 0; j < shipSize; j++) {
          if (colOff + j >= GRID_SIZE) break;
          if (aiStrikes[row][colOff + j] != EMPTY && aiStrikes[row][colOff + j] != HIT) break;
          if (j == shipSize - 1) {
            int size = j;
            while (size >= 0) {
              if(aiStrikes[row][colOff + size] == HIT) {
                size--;
                continue;
              }
              int probPos = ++prob[row][colOff + size];
              if(probPos > highestCount) {
                bestMoveX = colOff + size;
                bestMoveY = row;
                highestCount = probPos;
              }
              size--;
            }
            break;
          }
        }
        colOff++;
      }

      //check vert
      rowOff = row - (shipSize - 1); // lower limit to check
      while(rowOff <= row) {
        if(rowOff < 0) {
          rowOff++;
          continue;
        }
        for(int j = 0; j < shipSize; j++) {
          if (rowOff + j >= GRID_SIZE) break;
          if (aiStrikes[rowOff + j][col] != EMPTY && aiStrikes[rowOff + j][col] != HIT) break;
          if (j == shipSize - 1) {
            int size = j;
            while (size >= 0) {
              if(aiStrikes[rowOff + size][col] == HIT) {
                size--;
                continue;
              }
              int probPos = ++prob[rowOff + size][col];
              if(probPos > highestCount) {
                bestMoveX = col;
                bestMoveY = rowOff + size;
                highestCount = probPos;
              }
              size--;
            }
            break;
          }
        }
        rowOff++;
      }
    }
    curHit = curHit->next;
  }

  nextMove->x = bestMoveX;
  nextMove->y = bestMoveY;

  if (bestMoveX == GRID_SIZE && bestMoveY == GRID_SIZE) {
    return -1;
  }

  return 0; 
}

/**
 * @brief Determines the AI's next when there are no ships hit (or hit and sunk)
 * 
 * @param aiStrikes AI's strike tracking grid
 * @param playerShips Player's ship placement
 * @param nextMove returns the next move
 * @return 0 on success, -1 if no move is found
 */
static int nextMoveNoHit(int ** aiStrikes, Ship * playerShips, Point *nextMove) {
  int prob [GRID_SIZE][GRID_SIZE] = {0};
  int highestCount = 0;
  int bestMoveX = GRID_SIZE;
  int bestMoveY = GRID_SIZE;
  int shipSize;

  // intital plays use random search
  if ( !g_ai_has_hit && (g_ai_total_moves < MAX_RANDOM_MOVES)) {
    return nextMoveRandomSearch(aiStrikes, nextMove);
  }

  for (int i = 0; i < NUM_SHIPS; i++) {
    if (playerShips[i].sunk) {
      continue;
    }

    shipSize = playerShips[i].size;

    for(int row = 0; row < GRID_SIZE; row++) {
      for(int col = 0; col < GRID_SIZE; col ++) {
        if (aiStrikes[row][col] != EMPTY) continue;

        //check pos x
        for(int j = 0; j < shipSize; j++) {
          if (col + j >= GRID_SIZE) break;
          if (aiStrikes[row][col + j] != EMPTY) break;
          if (j == shipSize - 1) {
            int size = j;
            while (size >= 0) {
              int probPos = ++prob[row][col + size];
              if(probPos > highestCount) {
                bestMoveX = col + size;
                bestMoveY = row;
                highestCount = probPos;
              }
              size--;
            }
            break;
          }
        }

        //check neg x
        for(int j = 0; j < shipSize; j++) {
          if (col - j < 0) break;
          if (aiStrikes[row][col - j] != EMPTY) break;
          if (j == shipSize - 1) {
            int size = j;
            while (size >= 0) {
              int probPos = ++prob[row][col - size];
              if(probPos > highestCount) {
                bestMoveX = col - size;
                bestMoveY = row;
                highestCount = probPos;
              }
              size--;
            }
            break;
          }
        }

        //check neg y
        for(int j = 0; j < shipSize; j++) {
          if (row + j >= GRID_SIZE) break;
          if (aiStrikes[row + j][col] != EMPTY) break;
          if (j == shipSize - 1) {
            int size = j;
            while (size >= 0) {
              int probPos = ++prob[row + size][col];
              if(probPos > highestCount) {
                bestMoveX = col;
                bestMoveY = row + size;
                highestCount = probPos;
              }
              size--;
            }
            break;
          }
        }
        //check pos y
        for(int j = 0; j < shipSize; j++) {
          if (row - j < 0) break;
          if (aiStrikes[row - j][col] != EMPTY) break;
          if (j == shipSize - 1) {
            int size = j;
            while (size >= 0) {
              int probPos = ++prob[row - size][col];
              if(probPos > highestCount) {
                bestMoveX = col;
                bestMoveY = row - size;
                highestCount = probPos;
              }
              size--;
            }
            break;
          }
        }        
      }
    }
  }

  nextMove->x = bestMoveX;
  nextMove->y = bestMoveY;

  if (bestMoveX == GRID_SIZE && bestMoveY == GRID_SIZE) {
    return -1;
  }

  return 0; 
}

static int nextMoveRandomSearch(int ** aiStrikes, Point *nextMove) {
  int validMove = 0;
  int x;
  int y;

  while(!validMove) {
    x = rand() % GRID_SIZE;
    y = rand() % GRID_SIZE;
    if ((x + y) % 2 != 0) continue;
    if (aiStrikes[y][x] != EMPTY) continue;
    validMove = 1;
  }
  nextMove->x = x;
  nextMove->y = y;
  g_ai_total_moves++;

  return 0;
}

static Hit * createHit(Point * pos) {
  Hit * newHit = (Hit *) malloc(sizeof(Hit));
  newHit->pos.x = pos->x;
  newHit->pos.y = pos->y;
  newHit->next = NULL;
  return newHit;
}

static Hit * deleteHitsWithSunkShip(Hit * head, Ship * sunkPlayerShip, int ** aiStrikes) {
  Hit * newHead = head;
  Hit * curHit = head;
  Hit * prevHit = NULL;

  while (curHit->next != NULL) {
    int startX = sunkPlayerShip->startAddress.x < sunkPlayerShip->endAddress.x ? sunkPlayerShip->startAddress.x : sunkPlayerShip->endAddress.x;
    int endX = sunkPlayerShip->startAddress.x > sunkPlayerShip->endAddress.x ? sunkPlayerShip->startAddress.x : sunkPlayerShip->endAddress.x;
    int startY = sunkPlayerShip->startAddress.y < sunkPlayerShip->endAddress.y ? sunkPlayerShip->startAddress.y : sunkPlayerShip->endAddress.y;
    int endY = sunkPlayerShip->startAddress.y > sunkPlayerShip->endAddress.y ? sunkPlayerShip->startAddress.y : sunkPlayerShip->endAddress.y;

    if (curHit->pos.x >= startX && curHit->pos.x <= endX && 
        curHit->pos.y >= startY && curHit->pos.y <= endY) {
      
      aiStrikes[curHit->pos.y][curHit->pos.x] = SUNK;
      
      if (prevHit == NULL) {
        head = curHit->next;
        free(curHit);
        curHit = head;
        newHead = curHit;
      } else {
        prevHit->next = curHit->next;
        free(curHit);
        curHit = prevHit->next;
      }
    } else {
        prevHit = curHit;
        curHit = curHit->next;
    }
  }

  return newHead;
}

static void insertHit(Hit ** head, Hit * newHit) {
  newHit->next = *head;
  *head = newHit;
}

static void deleteAllHits(Hit * head) {
  Hit * curHit = head;
  Hit * nextHit = NULL;

  while (curHit != NULL) {
    nextHit = curHit->next;
    free(curHit);
    curHit = nextHit;
  }
}
