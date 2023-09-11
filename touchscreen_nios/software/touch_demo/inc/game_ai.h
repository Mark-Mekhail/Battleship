#ifndef GAME_AI_H
#define GAME_AI_H

#include "./touch.h"
#include "./grid.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct Hit {
  Point pos;
  struct Hit *next;
} Hit;

typedef struct Ship {
  Point startAddress;
  Point endAddress;
  int size;
  int id;
  bool sunk;
} Ship;

#define MAX_RANDOM_MOVES 10

int nextMove (Hit * curHit, int ** aiStrikes, Ship * playerShips, Point *nextMove);
void placeAIShips(int **aiShipsGrid, Ship *aiShips);
void initPlayerShips(Point * shipCoords, Ship * playerShips, int ** playerShipsGrid);
int aiStrike(Point * strike, int ** playerShipsGrid, int ** aiStrikes, Ship * playerShips, Hit ** head);
int playerStrike(int ** aiShipsGrid, int ** playersStrikes, Ship * aiShips, Point * target, Ship ** retShip);
void clearAI(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
