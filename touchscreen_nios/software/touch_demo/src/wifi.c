#include "../inc/wifi.h"
#include "../inc/main.h"
#include "../inc/globals.h"
#include "../inc/vga_utils.h"
#include "../inc/touch.h"

#include <stdio.h>
#include <string.h>

#include "system.h"
#include "altera_up_avalon_rs232_regs.h"
#include "altera_up_avalon_rs232.h"
#include "sys/alt_stdio.h"

/**
 * @brief Check if websocket connection to server is open
 * 
 * @param rs232_dev rs232 device structure
 * @return 0 on success, -1 if there's an error
 */
int checkConnection(alt_up_rs232_dev* wifi_rs232_dev) {
	// printf("check connect called\n");
	return writeJsonWIFI(wifi_rs232_dev, "{'type': 'CHECK_WSCON'}");
}

/**
 * @brief Creates new game room
 * 
 * @param rs232_dev rs232 device structure
 * @return 0 on success, -1 if there's an error
 */
int createRoom(alt_up_rs232_dev* wifi_rs232_dev) {
	return writeJsonWIFI(wifi_rs232_dev, "{ 'type': 'CREATE_ROOM', 'isPublic': true}");
}

/**
 * @brief Player will join a room with given roomId
 * 
 * @param rs232_dev rs232 device structure
 * @param roomId Player's ships (mem allocated array of NUM_SHIPS ships)
 * @return 0 on success, -1 if there's an error
 */
int joinRoom(alt_up_rs232_dev* wifi_rs232_dev, int roomId) {
	  char buffer [50];
	  sprintf (buffer, "{ 'type': 'JOIN_ROOM', 'roomId': %d}", roomId);
		printf(buffer);
		printf("\n");
	  return writeJsonWIFI(wifi_rs232_dev, buffer);
}

/**
 * @brief Leaves current room
 * 
 * @param rs232_dev rs232 device structure
 * @return 0 on success, -1 if there's an error
 */
int leaveRoom(alt_up_rs232_dev* wifi_rs232_dev) {
	return writeJsonWIFI(wifi_rs232_dev, "{ 'type': 'LEAVE_ROOM' }");
}

/**
 * @brief Sets players ship placement for current game
 * 
 * @param rs232_dev rs232 device structure
 * @param playerShips Player's ships (mem allocated array of NUM_SHIPS ships)
 * @return 0 on success, -1 if there's an error
 */
int setShips(alt_up_rs232_dev* wifi_rs232_dev, Ship *playerShips) {
	char buffer [100];
	Ship battleship = playerShips[0];
	Ship cruiser = playerShips[1];
	Ship sub = playerShips[2];
	Ship destroyer = playerShips[3];
	sprintf (buffer, "{'type':'SET_SHIPS',"
			"'b': [%d,%d,%d,%d],"
			"'c':[%d,%d,%d,%d],"
			"'s':[%d,%d,%d,%d],"
			"'d':[%d,%d,%d,%d]}",
			battleship.startAddress.x, battleship.startAddress.y, battleship.endAddress.x, battleship.endAddress.y,
			cruiser.startAddress.x, cruiser.startAddress.y, cruiser.endAddress.x, cruiser.endAddress.y,
			sub.startAddress.x, sub.startAddress.y, sub.endAddress.x, sub.endAddress.y,
			destroyer.startAddress.x, destroyer.startAddress.y, destroyer.endAddress.x, destroyer.endAddress.y
			);
	return writeJsonWIFI(wifi_rs232_dev, buffer);
}

/**
 * @brief Sends players current move to the server
 * 
 * @param rs232_dev rs232 device structure
 * @param target coordinates of move
 * @return 0 on success, -1 if there's an error
 */
int fire(alt_up_rs232_dev* wifi_rs232_dev, Point *target) {
	  char buffer [50];
	  sprintf (buffer, "{ 'type': 'FIRE', 'x': %d, 'y': %d}", target->x, target->y);
	  return writeJsonWIFI(wifi_rs232_dev, buffer);
}

/**
 * @brief Writes json to wifi controller 
 * 
 * @param rs232_dev rs232 device structure
 * @param json json to be written to wifi controller
 * @return 0 on success, -1 if there's an error
 */
int writeJsonWIFI(alt_up_rs232_dev* rs232_dev, char* json){
	alt_u8 readb = 0;
	alt_u8 error;
	alt_up_rs232_disable_read_interrupt(rs232_dev);

	// empty read buffer - additional bytes sometimes read after '>' new line char
	while(alt_up_rs232_get_used_space_in_read_FIFO(rs232_dev) > 0) {
		  int success = alt_up_rs232_read_data(rs232_dev, &readb, &error);
	  }

	// no buffered instructions
	while (alt_up_rs232_get_available_space_in_write_FIFO(rs232_dev) != WRITE_FIFO_EMPTY);

	for (int i = 0; i < strlen(json); i++) {
		char instrb = *(json + i);;
		int success = alt_up_rs232_write_data(rs232_dev, instrb);
		usleep(70);
		if(success)
			return -1;
	}

	alt_up_rs232_enable_read_interrupt(rs232_dev);
	return 0;
}

/**
 * @brief Reads wifi controller response from rs232 read buffer
 * 
 * @param rs232_dev rs232 device structure
 * @param response response from wifi controller
 * @param stop_char character used to indicate end of response from wifi controller
 * @return 0 on success, -1 if there's an error
 */
int readWIFIResponse(alt_up_rs232_dev* rs232_dev, char* response, alt_u8 stop_char) {
	alt_u8 readb = 0;
	alt_u8 error;
	unsigned avail;

	int res_len = 0;
	while(readb != stop_char) {
		avail = alt_up_rs232_get_used_space_in_read_FIFO(rs232_dev);
		if(avail > 0) {
			int success = alt_up_rs232_read_data(rs232_dev, &readb, &error);
			alt_printf("%c",readb);
			response[res_len] = readb;
			res_len++;
		}
	}

	avail = alt_up_rs232_get_used_space_in_read_FIFO(rs232_dev);
	alt_printf("\nused read fifo after write: %x\n", avail);
  // empty unwanted characters from read buffer
  while(alt_up_rs232_get_used_space_in_read_FIFO(rs232_dev) > 0) {
	  int success = alt_up_rs232_read_data(rs232_dev, &readb, &error);
	  alt_printf("%c", readb);
  }
	return 0;
}