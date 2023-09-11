#ifndef TOUCH_H
#define TOUCH_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define TOUCH_RESOLUTION 4096

/* a data type to hold a point/coord */
typedef struct { int x, y; } Point ;

void Init_Touch(void);
int ScreenTouched(void);
void WaitForTouch(void);
Point GetPress(void);
Point GetRelease(void);
Point getTouch(void);
void WaitForPacket(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif