#ifndef JOIN_GAME_H
#define JOIN_GAME_H

#include "./touch.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum JOIN_GAME_BUTTONS {
  JGB_NONE    = 0,
  JGB_RESET   = 1,
  JGB_CONFIRM = 2
} JOIN_GAME_BUTTONS;

JOIN_GAME_BUTTONS joinGameScreen(void);
void failureScreen(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
