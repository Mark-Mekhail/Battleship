#ifndef GAME_MODE_H
#define GAME_MODE_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum GAME_MODE_BUTTONS {
  GMB_NONE          = 0,
  GMB_SINGLE_PLAYER = 1,
  GMB_MULTI_PLAYER  = 2
} GAME_MODE_BUTTONS;

GAME_MODE_BUTTONS gameModeMenuScreen(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif