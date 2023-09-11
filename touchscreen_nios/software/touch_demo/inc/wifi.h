#ifndef WIFI_H
#define WIFI_H

#include "altera_up_avalon_rs232_regs.h"
#include "altera_up_avalon_rs232.h"
#include "altera_up_avalon_video_character_buffer_with_dma.h"
#include "game_ai.h"
#include "touch.h"

#define WRITE_FIFO_EMPTY 128
#define NEW_LINE 0x3e
#define RES_COMPLETE 0x24

// response opcodes
#define ROOM_CREATED 0x31     // 1
#define START_SETUP 0x32      // 2
#define START_GAME 0x33       // 3
#define FIRED 0x34            // 4
#define OPPONENT_FIRED 0x35   // 5
#define OPPONENT_LEFT 0x36    // 6
#define GAME_RESET 0x37       // 7
#define CHECK_WSCON 0x38      // 8
#define ERROR 0x39            // 9
#define SHIPS_SET 0x40        // @

// response string indices
#define OPCODE 0
#define IS_CONNECTED 1
#define YOUR_TURN 1
#define FIRED_HIT 1
#define FIRED_X 2
#define FIRED_Y 3
#define FIRED_SUNK 4
#define FIRED_SX 5
#define FIRED_SY 6
#define FIRED_EX 7
#define FIRED_EY 8

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


int writeJsonWIFI(alt_up_rs232_dev* rs232_dev, char* json);
int readWIFIResponse(alt_up_rs232_dev* rs232_dev, char* response, alt_u8 stop_char);
int checkConnection(alt_up_rs232_dev* wifi_rs232_dev);
int createRoom(alt_up_rs232_dev* wifi_rs232_dev);
int joinRoom(alt_up_rs232_dev* wifi_rs232_dev, int roomId);
int leaveRoom(alt_up_rs232_dev* wifi_rs232_dev);
int setShips(alt_up_rs232_dev* wifi_rs232_dev, Ship *playerShips);
int fire(alt_up_rs232_dev* wifi_rs232_dev, Point *target);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
