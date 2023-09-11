#ifndef MULTIPLAYER_H
#define MULTIPLAYER_H

#ifdef __cplusplus
extern "C" 
#endif /* __cplusplus */

typedef enum MULTI_PLAYER_BUTTONS {
  MPB_NONE          = 0,
  MPB_CREATE_GAME   = 1,
  MPB_JOIN_GAME     = 2
} MULTI_PLAYER_BUTTONS;

MULTI_PLAYER_BUTTONS multiplayerScreen(void);
void establishingConnectionScreen(void);

#ifdef __cplusplus

#endif /* __cplusplus */

#endif