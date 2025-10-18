/**
 * This version is specifically created for SGDK as an example of a game using SEGA Link Cable!
 * You MUST set MODULE_LINK_CABLE to 1 in config.h and rebuild the library to build this project!
 *
 * Super Turbo MEGA Pac-Man v2.11
 * Pac-Man for Sega Mega Drive / Sega Genesis
 *
 * Two-player game, including network play capability on two consoles
 * via SEGA Link Cable (Zero Tolerance Link Cable), connected to the second port.
 *
 * You can see what SEGA Link Cable is using Zero Tolerance and Super Turbo MEGA Pac-Man as examples here:
 * https://youtu.be/aLk8tDrjZ50
 *
 * For this game, SEGA Link Cable Protocol (LCP) was implemented - a protocol for data exchange via SEGA Link Cable
 * between two Sega Mega Drive / Sega Genesis consoles. This is a C programming language library for SGDK.
 *
 * Game description:
 * 1st player controls Pac-Man, who is chased by 1 red-colored ghost.
 * If the ghost catches Pac-Man - the game will end.
 * If Pac-Man or Pac-Girl eat a green dot, the ghost will become
 * purple-colored and start running away from Pac-Man, as now it can be eaten itself.
 * 2nd player controls Pac-Girl. She helps collect dots on the level, while
 * neither Pac-Man nor the ghost can eat Pac-Girl, and she can't eat them (immortal).
 * The game will end in victory if players can collect all food (white and green dots) on the level.
 * In the ghost house, a cherry appears after some time - it just gives points,
 * to win, you don't have to eat it!
 * In the player count selection menu, you can choose which character each player will control
 * when playing together. To do this, press right in the '2 PLAYERS' menu so the first player
 * controls Pac-Girl, and the second player controls Pac-Man, or left to return to default settings.
 *
 * More about the project here (In Russian)
 * https://www.youtube.com/watch?v=DN2u24B_XL0
 *
 * Ports of this game for different platforms:
 *
 * DOS, Windows, GNU/Linux, MacOS
 * https://github.com/blodtor/pacman
 *
 * NES / Famicom / Dendy
 * https://github.com/blodtor/npacman
 *
 * Sega Mega Drive / Sega Genesis
 * https://github.com/blodtor/spacman
 *
 * BlodTor 2025
 */

#include <genesis.h>
#include "main.h"
#include "resources.h"

#if (MODULE_LINK_CABLE == 0)
#error "Set MODULE_LINK_CABLE to 1 in config.h and rebuild the library"
#endif

/**
 * Create synchronization object for consoles via Link Cable Protocol: OBJECT_TYPE_MASTER
 * in byte array transferObject
 *
 * The phrase can be any, if another console receives it, then it
 * will become slave, and the screen will display "LINK  SLAVE" for the other player.
 *
 * Ours will become master when working via Link Cable Protocol,
 * the screen will display "LINK MASTER", in turn we receive from the other console
 * another synchronization object OBJECT_TYPE_SLAVE with a different phrase "Pac-Man!"
 */
void masterToTransferObject() {
	memcpy(transferObject, "Pac-Girl", MASTER_OBJECT_LENGTH);
}


/**
 * Create synchronization object for consoles via Link Cable Protocol: OBJECT_TYPE_SLAVE
 * in byte array transferObject
 *
 * The phrase can be any, if another console receives it, then it
 * will become master, and the screen will display "LINK MASTER" for the other player.
 *
 * Ours will become slave when working via Link Cable Protocol,
 * the screen will display "LINK  SLAVE", in turn we receive from the other console
 * another synchronization object OBJECT_TYPE_MASTER with a different phrase "Pac-Girl"
 */
void slaveToTransferObject() {
	memcpy(transferObject, "Pac-Man!", SLAVE_OBJECT_LENGTH);
}


/**
 * Save dx, dy, pacmanX, pacmanY, oldX, oldY in byte array transferObject
 * for transmission to another console as OBJECT_TYPE_PAC_MAN_STATE object
 */
void pacManStateToTransferObject() {
	transferObject[0] = 0;
	if (dx > 0) {
		// if moving right, bit 6 = 1
		transferObject[0] = 0b01000000;
	} else if (dx < 0) {
		// if moving left, bit 7 = 1
		transferObject[0] = 0b10000000;
	}

	// pacmanX has value from 0 to 31 i.e. 11111 in binary
	transferObject[0]+= pacmanX;

	transferObject[1] = 0;
	if (dy > 0) {
		// if moving down, bit 6 = 1
		transferObject[1] = 0b01000000;
	} else if (dy < 0) {
		// if moving up, bit 7 = 1
		transferObject[1] = 0b10000000;
	}

	// pacmanY has value from 0 to 22 i.e. 10100 in binary
	transferObject[1]+= pacmanY;

	// old Pac-Man x coordinates
	transferObject[2] = oldX;

	// old Pac-Man y coordinates
	transferObject[3] = oldY;
}


/**
 * Get dx, dy, pacmanX, pacmanY, oldX, oldY from byte array transferObject
 * in case we received OBJECT_TYPE_PAC_MAN_STATE object from another console
 */
void pacManStateFromTransferObject() {
	if (transferObject[0] & 0b01000000) {
		// if bit 6 = 1, then moving right
		dx = 1;
	} else if (transferObject[0] & 0b10000000) {
		// if bit 7 = 1, then moving left
		dx = -1;
	} else {
		// otherwise standing still
		dx = 0;
	}

	// in bits 0-5 value where PAC-MAN is located by X
	pacmanX = transferObject[0] & 0b00111111;

	if (transferObject[1] & 0b01000000) {
		// if bit 6 = 1, then moving down
		dy = 1;
	} else if (transferObject[1] & 0b10000000) {
		// if bit 7 = 1, then moving up
		dy = -1;
	} else {
		// otherwise standing still
		dy = 0;
	}

	// in bits 0-5 value where PAC-MAN is located by Y
	pacmanY = transferObject[1] & 0b00111111;

	// old Pac-Man x coordinates
	oldX = transferObject[2];

	// old Pac-Man y coordinates
	oldY = transferObject[3];


    if (map[oldY][oldX] != PACGIRL && map[oldY][oldX] != DOOR) {
    	// if in old coordinates there was no door, then clear value in array
    	map[oldY][oldX] = EMPTY;
    }

	if(map[pacmanY][pacmanX] != PACGIRL) {
		// if in current coordinates there is no PACGIRL,
		// then occupy this cell with PACMAN
		map[pacmanY][pacmanX] = PACMAN;
	}
}


/**
 * Save dxPacGirl, dyPacGirl, pacGirlX, pacGirlY, oldPacGirlX, oldPacGirlY in byte array transferObject
 * for transmission to another console as OBJECT_TYPE_PAC_GIRL_STATE object
 */
void pacGirlStateToTransferObject() {
	transferObject[0] = 0;
	if (dxPacGirl > 0) {
		// if moving right, bit 6 = 1
		transferObject[0] = 0b01000000;
	} else if (dxPacGirl < 0) {
		// if moving left, bit 7 = 1
		transferObject[0] = 0b10000000;
	}

	// pacGirlX has value from 0 to 31, i.e. 11111 in binary
	transferObject[0]+= pacGirlX;

	transferObject[1] = 0;
	if (dyPacGirl > 0) {
		// if moving down, bit 6 = 1
		transferObject[1] = 0b01000000;
	} else if (dyPacGirl < 0) {
		// if moving up, bit 7 = 1
		transferObject[1] = 0b10000000;
	}

	// pacGirlY has value from 0 to 22, i.e. 10100 in binary
	transferObject[1]+= pacGirlY;

	// old Pac-Girl x coordinates
	transferObject[2] = oldPacGirlX;

	// old Pac-Girl y coordinates
	transferObject[3] = oldPacGirlY;
}


/**
 * Get dxPacGirl, dyPacGirl, pacGirlX, pacGirlY from byte array transferObject
 * in case we received OBJECT_TYPE_PAC_GIRL_STATE object from another console
 */
void pacGirlStateFromTransferObject() {
	if (transferObject[0] & 0b01000000) {
		// if bit 6 = 1, then moving right
		dxPacGirl = 1;
	} else if (transferObject[0] & 0b10000000) {
		// if bit 7 = 1, then moving left
		dxPacGirl = -1;
	} else {
		// otherwise standing still
		dxPacGirl = 0;
	}

	// in bits 0-5 value where PAC-GIRL is located by X
	pacGirlX = transferObject[0] & 0b00011111;

	if (transferObject[1] & 0b01000000) {
		// if bit 6 = 1, then moving down
		dyPacGirl = 1;
	} else if (transferObject[1] & 0b10000000) {
		// if bit 7 = 1, then moving up
		dyPacGirl = -1;
	} else {
		// otherwise standing still
		dyPacGirl = 0;
	}

	// in bits 0-5 value where PAC-GIRL is located by Y
	pacGirlY = transferObject[1] & 0b00011111;

	// old PAC-GIRL x coordinates
	oldPacGirlX = transferObject[2];

	// old PAC-GIRL y coordinates
	oldPacGirlY = transferObject[3];

    if (map[oldPacGirlY][oldPacGirlX] != DOOR) {
    	// if in old coordinates there is no door, clear the cell
    	map[oldPacGirlY][oldPacGirlX] = EMPTY;
    }

    // in new coordinates display PACGIRL
	map[pacGirlY][pacGirlX] = PACGIRL;
}


/**
 * Save dxRed, dyRed, redX, redY, redFlag in byte array transferObject
 * for transmission to another console as OBJECT_TYPE_RED_STATE object
 */
void redStateToTransferObject() {
	transferObject[0] = 0;
	if (dxRed > 0) {
		// if moving right, bit 6 = 1
		transferObject[0] = 0b01000000;
	} else if (dxRed < 0) {
		// if moving left, bit 7 = 1
		transferObject[0] = 0b10000000;
	}

	if (redFlag) {
		// if redFlag == 1, then bit 5 = 1
		transferObject[0] |= 0b00100000;
	}

	// redX has value from 0 to 31, i.e. 11111 in binary
	transferObject[0]+= redX;

	transferObject[1] = 0;
	if (dyRed > 0) {
		// if moving down, bit 6 = 1
		transferObject[1] = 0b01000000;
	} else if (dyRed < 0) {
		// if moving up, bit 7 = 1
		transferObject[1] = 0b10000000;
	}

	// redY has value from 0 to 22, i.e. 10100 in binary
	transferObject[1]+= redY;
}


/**
 * Get dxRed, dyRed, redX, redY, redFlag from byte array transferObject
 * in case we received OBJECT_TYPE_RED_STATE object from another console
 */
void redStateFromTransferObject() {
	if (transferObject[0] & 0b01000000) {
		// if bit 6 = 1, then moving right
		dxRed = 1;
	} else if (transferObject[0] & 0b10000000) {
		// if bit 7 = 1, then moving left
		dxRed = -1;
	} else {
		// otherwise standing still
		dxRed = 0;
	}

	// in bits 0-5 value where RED is located by X
	redX = transferObject[0] & 0b00011111;

	if (transferObject[1] & 0b01000000) {
		// if bit 6 = 1, then moving down
		dyRed = 1;
	} else if (transferObject[1] & 0b10000000) {
		// if bit 7 = 1, then moving up
		dyRed = -1;
	} else {
		// otherwise standing still
		dyRed = 0;
	}

	// in bits 0-5 value where RED is located by Y
	redY = transferObject[1] & 0b00011111;

	if (transferObject[0] & 0b00100000) {
		// if bit 5 == 1, then ghost is not edible
		redFlag = 1;
		if (map[redY][redX] != PACGIRL) {
			map[redY][redX] = RED;
		}
	} else {
		// otherwise edible
		redFlag = 0;
		if (map[redY][redX] != PACGIRL) {
			map[redY][redX] = SHADOW;
		}
	}
}


/**
 * In transferObject save object containing information about what was pressed on the first controller
 * of our console for transmission to another console as OBJECT_TYPE_JOY object
 *
 * pad - information about what was pressed on the controller
 */
void padToTransferObject(u16 pad) {
	transferObject[0] = pad >> 8;     // high byte of pad shifted to low byte, i.e. 8 bits to the right
	transferObject[1] = pad & 0x00FF; // low byte from pad to 2nd array element that we will transmit
}

/**
 * From object stored in transferObject transmitted via Link Cable get information about what was pressed
 * on the first controller of another console in case we received OBJECT_TYPE_JOY object from another console
 *
 * return information about what was pressed on the controller
 */
u16 getPadFromTransferObject() {
	u16 pad = transferObject[0]; // high byte
	pad <<= 8;                   // moved high byte to its place (left by 8 bits)
	pad |= transferObject[1];    // low byte
	return pad;
}


/**
 * Determine console operation mode when trying to play together,
 * check what is connected to controller port 2.
 * If playing via SEGA Link Cable, determine which
 * console is master and which is slave
 */
void initControllerPort2() {
	// unknown if there is connection between consoles
    controllerPort2Mode = MODE_PORT2_UNKNOWN;

    // fill game type text with zeros
    memset(gameModeText, 0, GAME_MODE_TEXT_SIZE + 1);

	// determine what is connected to the second console port
	u8 pad2type = JOY_getJoypadType(JOY_2);
	if (pad2type == JOY_TYPE_PAD3 || pad2type == JOY_TYPE_PAD6) {
		// 3 or 6 button controller connected to port 2 of the console
		controllerPort2Mode = MODE_MULTI_PLAYER;

		// display game type '2 ИГРОКА НЕ СЕТЕВАЯ! ' - 2 players playing on controller connected
		// to port 2 of our same console, NOT via SEGA Link Cable
		memcpy(gameModeText, TEXT_2P_NO_LINK, GAME_MODE_TEXT_SIZE);

		return;
	} else if (pad2type == JOY_TYPE_UNKNOWN) {
		// reset data transmission errors to 0 that we show on screen via Link cable
		linkCableErrors = 0;
		// reset count of data transmission errors via Link cable
		linkCableErrorsCount = 0;
		// reset count of rendered frames since connection establishment via Link cable
		linkCableFrameCount = 0;

		memcpy(gameModeText, TEXT_TRY_MASTER, GAME_MODE_TEXT_SIZE);
		drawText();
		SYS_doVBlankProcess();

		// Link Cable Protocol initialization
		LCP_init();

		// put OBJECT_TYPE_MASTER data type object with phrase "Pac-Girl" into buffer variable
		masterToTransferObject();
		// add created object from buffer variable to packet for data transmission
		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_MASTER, LINK_TYPES_LENGTH);

		do {
			// try to send LCP_sendPacket packet to another console
			// and receive LCP_ReceivePacke packet with data from it
			// as master console
			LCP_masterCycle();

			// if no errors, i.e. LCP_error == 0, means successfully sent and received data from another console,
			// exit the loop! Need to check what was received in LCP_ReceivePacke! Possibly our console will be master
			// if error 0x1A - means another console didn't try to receive and send data at all,
			// exit the loop and try to become slave console
			// if any other error - means second console tried to send and receive data, need
			// to try to exchange data again!

			linkCableErrors = LCP_getError();
		} while (!( linkCableErrors == 0x1A || linkCableErrors == 0));

		// if there were no errors in packet transmission and reception,
		// then LCP_getNextObjectFromReceivePacket() will return object received from another console
		while ((objectType = LCP_getNextObjectFromReceivePacket(transferObject, LINK_TYPES_LENGTH))) {

			// Check that we received OBJECT_TYPE_SLAVE with phrase 'Pac-Man!' from another console
			if (objectType == OBJECT_TYPE_SLAVE && LCP_getError() == 0) {
				// Our console becomes master
				// this means further during interaction we call external interrupt - External interrupt (EX-INT)
				// on another console, since it will be slave we send packet, then receive packet.
				// now packet sending and reception on our console using LCP_masterCycle() method
				// on our console will not trigger external interrupt, we work synchronously from main game code
				// in controls() method, where we form packet, send, and receive packet from another console
				controllerPort2Mode = MODE_PORT2_MASTER;

				// display game type 'Pac-Man!' - 2 players playing on controller connected
				// to port 1 of another console via SEGA Link Cable connected to port 2 of both consoles

				memcpy(gameModeText, TEXT_LINK_MASTER, GAME_MODE_TEXT_SIZE);

				// sound that connection via Link cable was successfully created
				XGM_startPlayPCM(SFX_SOUND_CONNECT_LINK_CABLE, 15, SOUND_PCM_CH4);

				// need to wait a bit so slave console can process received packet
				SYS_doVBlankProcess();

				// exit function
				return;
			}
		}

		memcpy(gameModeText, TEXT_TRY_SLAVE, GAME_MODE_TEXT_SIZE);

		// since we failed to receive OBJECT_TYPE_SLAVE with phrase 'Pac-Man!' from another console
		// try to receive data as slave console

		// put OBJECT_TYPE_SLAVE data type object with phrase "Pac-Man!" into buffer variable
		slaveToTransferObject();
		// add created object from buffer variable to packet for data transmission
		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_SLAVE , LINK_TYPES_LENGTH);
		// now we have 2 objects in packet: both OBJECT_TYPE_MASTER and OBJECT_TYPE_SLAVE
		// and in case our console receives packet from another, we will send both objects in packet to it
		// but master console will expect only OBJECT_TYPE_SLAVE, OBJECT_TYPE_MASTER object will be
		// ignored by it, see code above

		// open console port 2, which means now our console will process external interrupts
		// External interrupt (EX-INT) and first receive packets when this interrupt occurs, then send.
		// This will happen asynchronously with main game code, because essentially another console
		// remotely calls our console's method LCP_slaveCycle(), stopping current program execution on our console
		// at any random moment
		LCP_open();

		// timer after which we consider there is no SEGA Link Cable connection
		u16 timer = 0xFFFF;
		while (timer > 0) {
			drawText();
			SYS_doVBlankProcess();

			pad1 = JOY_readJoypad(JOY_1);
			if (pad1 & BUTTON_START) {
				// if Start is pressed on controller 1, exit connection waiting loop with another console
				break;
			}

			// try to get object from received packet
			objectType = LCP_getNextObjectFromReceivePacket(transferObject, LINK_TYPES_LENGTH);

			// Check that we received OBJECT_TYPE_MASTER with phrase 'Pac-Girl' from another console
			linkCableErrors = LCP_getError();
			if (objectType == OBJECT_TYPE_MASTER && linkCableErrors == 0) {
				// Our console becomes slave
				// this means that further during interaction master console of other player
				// will call external interrupt - External interrupt (EX-INT) on our console as a result
				// of which main game code on our slave console will be suspended for the time
				// of external interrupt handler function execution.
				// now packet reception and sending on our console will happen asynchronously
				// through calling LCP_slaveCycle() method at any random moment for us by another console,
				// and packet object parsing will be in main game code in controls() method, also packet
				// preparation for sending by our console
				controllerPort2Mode = MODE_PORT2_SLAVE;

				// display game type 'Pac-Girl' - 1 player playing on controller connected
				// to port 1 of another console via SEGA Link Cable connected to port 2 of both consoles
				memcpy(gameModeText, TEXT_LINK_SLAVE, GAME_MODE_TEXT_SIZE);

				// sound that connection via Link cable was successfully created
				XGM_startPlayPCM(SFX_SOUND_CONNECT_LINK_CABLE, 15, SOUND_PCM_CH4);

				// exit function
				return;
			}

			timer--;
		}

		// failed to create connection via SEGA Link Cable
		// close port, which means we no longer process external interrupts - External interrupt (EX-INT)
		// in LCP_slaveCycle() handler
		LCP_close();
	}

	memcpy(gameModeText, TEXT_1P_NO_LINK, GAME_MODE_TEXT_SIZE);

	// No SEGA Link cable connection and no 3 or 6 button controller connected to 2nd port
	// only single player game is possible
	controllerPort2Mode = MODE_SINGLE_PLAYER;
}


/**
 * Calculate score taking into account all bonuses
 */
void calcScore() {
	u16 i;
	score100 = food100;
	score010 = food010;
	score001 = food001;

	if (cherryBonus) {
		// 200 points for cherry
		score100 += 2;
	}

	for (i = 0; i < powerBonus; i++) {
		// 25 points for power-up
		score001 += 5;

		if (score001 >= 10) {
			score001 -= 10;
			++score010;
		}

		score010 += 2;

		if (score010 >=10) {
			score010 -= 10;
			++score100;
		}

	}


	for (i = 0; i < redBonus; i++) {
		// 50 points for eaten ghost
		score010 += 5;

		if (score010 >=10) {
			score010 -= 10;
			++score100;
		}
	}
}


/**
 * Cell at given coordinates is not a wall (WALL)
 * y - Y coordinate on map (map[][])
 * x - X coordinate on map (map[][])
 * return 1 - not a wall, 0 - wall
 */
u8 isNotWall(s16 y, s16 x) {
	if (map[y][x] == PACMAN || map[y][x] == PACGIRL || map[y][x] == RED
			|| map[y][x] == CHERRY || map[y][x] == FOOD
			|| map[y][x] == POWER_FOOD || map[y][x] == EMPTY
			|| map[y][x] == SHADOW) {
		return 1;

	}
	return 0;
}


/**
 * Cell at given coordinates is not a wall and not a door (WALL, DOOR)
 * y - Y coordinate on map (map[][])
 * x - X coordinate on map (map[][])
 * return 1 - not a wall and not a door, 0 - wall or door
 */
u8 isNotWallOrDoor(s16 y, s16 x) {
	if (isNotWall(y, x) && map[y][x] != DOOR) {
		return 1;

	}
	return 0;
}


/**
 * Coordinate correction for PAC-MAN, PAC-GIRL or Ghost
 * if went beyond the field (appearing from the other side of the field)
 * x - X coordinate on map (map[][])
 * y - Y coordinate on map (map[][])
 * values are passed by reference, so they change
 */
void moveBound(s16 *x, s16 *y) {
	if (*x < 0) {
		*x = MAP_SIZE_X - 2;
	} else if (*x > MAP_SIZE_X - 2) {
		*x = 0;
	}

	if (*y < 0) {
		*y = MAP_SIZE_Y - 1;
	} else if (*y > MAP_SIZE_Y - 1) {
		*y = 0;
	}
}


/**
 * Open doors to cherry and ghost house
 */
void openDoors() {
	map[doorY][doorX] = EMPTY;
	if (!cherryBonus) {
		map[cherryY][cherryX] = CHERRY;
	}

	cherryFlag = 1;
	refreshCherry = 1;
}


/**
 * Close doors to ghost house
 * if PACMAN didn't eat the cherry, it will appear again
 */
void closeDoors(void) {
	map[doorY][doorX] = DOOR;

	cherryFlag = 0;
	refreshDoor = 1;
	refreshCherry = 0;
}


/**
 * Reset everything to initial map settings:
 * initial cycle counter values,
 * initial character positions,
 * where food and power-ups will be
 */
void resetGame() {
	u16 i, j;
	// cycle counters start with different values,
	// so each character's rendering is in different global cycle
	pacmanLastUpdateTime = PACMAN_SPEED;
	redLastUpdateTime = 4;     //RED_SPEED;
	pacGirlLastUpdateTime = 6; //PACGIRL_SPEED;
	cherryTime = CHERRY_TIME;

	cherryBonus = 0;
	powerBonus = 0;
	redBonus = 0;
	redBonusVal = 0;

	food001 = 1;
	food010 = 0;
	food100 = 0;

	pacmanX = 15;
	pacmanY = 17;

	pacGirlX = 15;
	pacGirlY = 3;

	oldX = 15;
	oldY = 17;

	dx = 0;
	dy = 0;

	dxPacGirl = 0;
	dyPacGirl = 0;

	oldPacGirlX = 15;
	oldPacGirlY = 3;

	dxRed = 1;
	dyRed = 0;

	redX = 22;
	redY = 10;

	oldXRed = 22;
	oldYRed = 10;

	redFlag = 1;

	redTime = 0;

	cherryFlag = 0;
	refreshCherry = 0;
	refreshDoor = 1;

	oldRedVal = '.';
	oldPacGirlVal = '.';

	// place food on the map (gray dots)
	for (i = 0; i < MAP_SIZE_X; i++) {
		for (j = 0; j < MAP_SIZE_Y; j++) {
			val = map[j][i];
			if (val == EMPTY || val == PACGIRL || val == PACMAN || val == RED || val == SHADOW) {
				map[j][i] = FOOD;
			}
		}
	}

	// place power-ups
	map[2][1] = POWER_FOOD;
	map[2][29] = POWER_FOOD;
	map[17][1] = POWER_FOOD;
	map[17][29] = POWER_FOOD;

	// Pac-Man to starting position
	map[pacmanY][pacmanX] = PACMAN;

	// Red to starting position
	map[redY][redX] = RED;

    // Pac-Girl to starting position
	map[pacGirlY][pacGirlX] = PACGIRL;

	// door to ghost house,
	// cherry and surrounding cells
	// to initial state on the map
	map[doorY][doorX] = DOOR;
	map[doorY + 1][doorX] = EMPTY;
	map[cherryY][cherryX - 2] = EMPTY;
	map[cherryY][cherryX - 1] = EMPTY;
	map[cherryY][cherryX] = EMPTY;
	map[cherryY][cherryX + 1] = EMPTY;
	map[cherryY][cherryX + 2] = EMPTY;

	// Need to reset because when starting a new game, we may not have time to receive
	// packet with game state from master console, and slave console will think the game is over
	gameStateMaster = STATE_GAME;
}


/**
 * Food was eaten
 * recalculate counter values
 * food001 food010 food100
 */
void incFood() {
	// sound of eating a dot
	XGM_startPlayPCM(SFX_SOUND_EAT, 1, SOUND_PCM_CH2);

	++food001;
	if (food001 >= 10) {
		food001 -= 10;
		++food010;
	}
	if (food010 >= 10) {
		food010 -= 10;
		++food100;
	}
}


/**
 * Whether PACMAN lost or could eat the ghost
 * and what was eaten at the ghost's location: PACMAN or PACGIRL
 */
u8 pacmanLooser() {
	// If RED and PACMAN are on the same cell
	if (redY == pacmanY && redX == pacmanX) {
		// RED is not edible
		if (redFlag) {
			// End of game - PACMAN was eaten

			map[pacmanY][pacmanX] = RED;

			// remove Pac-Man sprite from screen
			SPR_setPosition(pacmanSprite, -90, 90);

			// immobilize Pac-Girl
			dxPacGirl = 0;
			dyPacGirl = 0;

			// stop RED
			dxRed = 0;
			dyRed = 0;

	        calcScore();

	    	if (MODE_PORT2_MASTER == controllerPort2Mode) {
	    		// send ghost state to slave console
	    		redStateToTransferObject();
	    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_RED_STATE, LINK_TYPES_LENGTH);

	    		// send to slave console that game ended
	    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_END_GAME, LINK_TYPES_LENGTH);
	    	}

			return 1;
		} else {
			// sound of eating ghost
			XGM_startPlayPCM(SFX_SOUND_EAT_SHADOW, 15, SOUND_PCM_CH3);

			// RED is edible at the moment
			// Send it to the Ghost House
			redY = 10;
			redX = 15;
			// immobilize
			dyRed = 0;
			dxRed = 0;
			// close door to ghost house
			closeDoors();

	    	// hide cherry
	    	SPR_setPosition(cherrySprite, -90, 100);

			// display RED on map as edible
			map[redY][redX] = RED;

	        redFlag = 1;

	       	// let it stay in the house for additional time
	        redTime = RED_TIME;

			// give bonus for eating RED
			++redBonus;

	    	if (MODE_PORT2_MASTER == controllerPort2Mode) {
	    		// send ghost state to slave console
	    		redStateToTransferObject();
	    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_RED_STATE, LINK_TYPES_LENGTH);

	    		// send to slave console that ghost was eaten
	    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_EAT_SHADOW, LINK_TYPES_LENGTH);
	    	}

			// check what Pac-Man ate together with RED
			if (oldRedVal == FOOD) {
				// food
				incFood();

		    	if (MODE_PORT2_MASTER == controllerPort2Mode) {
		    		// send to slave console that white dot (food) was eaten
		    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_EAT_POINT, LINK_TYPES_LENGTH);
		    	}
			} else if (oldRedVal == POWER_FOOD) {
				// power-up
				++powerBonus;

				// sound of eating power-up
				XGM_startPlayPCM(SFX_SOUND_POWERUP, 15, SOUND_PCM_CH2);

				// update time when RED became edible
				redTime = RED_TIME;

		    	if (MODE_PORT2_MASTER == controllerPort2Mode) {
		    		// send to slave console that power-up (green dot) was eaten
		    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_EAT_POWERUP, LINK_TYPES_LENGTH);
		    	}

			} else if (oldRedVal == CHERRY) {
				// cherry
				++cherryBonus;

				// hide cherry
				SPR_setPosition(cherrySprite, -90, 100);

				// sound of eating cherry
				XGM_startPlayPCM(SFX_SOUND_CHERRY, 15, SOUND_PCM_CH2);

		    	if (MODE_PORT2_MASTER == controllerPort2Mode) {
		    		// send to slave console that cherry was eaten
		    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_EAT_CHERRY, LINK_TYPES_LENGTH);
		    	}

			}

			if (cherryBonus) {
				oldRedVal = EMPTY;
			} else {
				oldRedVal = CHERRY;
			}
		}
	} else if (redY == pacGirlY && redX == pacGirlX) {
		// check what Pac-Girl ate at RED's location
		if (oldRedVal == FOOD) {
			// food
			incFood();
	    	if (MODE_PORT2_MASTER == controllerPort2Mode) {
	    		// send to slave console that white dot (food) was eaten
	    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_EAT_POINT, LINK_TYPES_LENGTH);
	    	}
		} else if (oldRedVal == POWER_FOOD) {
			// power-up
			++powerBonus;

			// sound of eating power-up
			XGM_startPlayPCM(SFX_SOUND_POWERUP, 15, SOUND_PCM_CH2);

			// update time when RED became edible
			redTime = RED_TIME;

			// RED becomes edible
			redFlag = 0;

	    	if (MODE_PORT2_MASTER == controllerPort2Mode) {
	    		// send to slave console that power-up (green dot) was eaten
	    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_EAT_POWERUP, LINK_TYPES_LENGTH);
	    	}
		} else if (oldRedVal == CHERRY) {
			// cherry
			++cherryBonus;

			// hide cherry
			SPR_setPosition(cherrySprite, -90, 100);
			refreshCherry = 0;

			// sound of eating cherry
			XGM_startPlayPCM(SFX_SOUND_CHERRY, 15, SOUND_PCM_CH2);

	    	if (MODE_PORT2_MASTER == controllerPort2Mode) {
	    		// send to slave console that cherry was eaten
	    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_EAT_CHERRY, LINK_TYPES_LENGTH);
	    	}
		}

		map[pacGirlY][pacGirlX] = RED;

		oldRedVal = EMPTY;
	}
	return 0;
}


/**
 * PAC-MAN movement processing algorithm on the map
 * return 0 - End of game
 *        1 - PACMAN is still alive
 */
u8 pacManState() {
	// check if PACMAN has movement direction set
	if (dx != 0 || dy != 0) {

		// should PACMAN move to a new cell
		if (pacmanLastUpdateTime == 0) {
			oldX = pacmanX;
			oldY = pacmanY;
			pacmanX = pacmanX + dx;
			pacmanY = pacmanY + dy;

			// reset time counter
			pacmanLastUpdateTime = PACMAN_SPEED;

			// adjust PACMAN coordinates if needed (to prevent leaving the field)
			// if went beyond the field (appearing from the other side of the field)
			moveBound(&pacmanX, &pacmanY);

			// if current cell has food, increase eaten counter
			val = map[pacmanY][pacmanX];
			if (val == FOOD) {
				incFood();
		    	if (MODE_PORT2_MASTER == controllerPort2Mode) {
		    		// send to slave console that white dot (food) was eaten
		    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_EAT_POINT, LINK_TYPES_LENGTH);
		    	}
			} else if (val == POWER_FOOD) {
				// RED becomes edible
				redFlag = 0;
				// runs in the opposite direction
				dxRed = -dxRed;
				dyRed = -dyRed;

				// RED became edible
				redTime = RED_TIME;

				// and give additional bonus
				++powerBonus;

				// sound of eating power-up
				XGM_startPlayPCM(SFX_SOUND_POWERUP, 15, SOUND_PCM_CH2);

		    	if (MODE_PORT2_MASTER == controllerPort2Mode) {
		    		// send to slave console that power-up (green dot) was eaten
		    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_EAT_POWERUP, LINK_TYPES_LENGTH);
		    	}

			} else if (val == CHERRY) {
				++cherryBonus;

				// hide cherry
				SPR_setPosition(cherrySprite, -90, 100);
				refreshCherry = 0;

				// sound of eating cherry
				XGM_startPlayPCM(SFX_SOUND_CHERRY, 15, SOUND_PCM_CH2);

		    	if (MODE_PORT2_MASTER == controllerPort2Mode) {
		    		// send to slave console that cherry was eaten
		    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_EAT_CHERRY, LINK_TYPES_LENGTH);
		    	}
			}


			if (isNotWallOrDoor(pacmanY, pacmanX)) {
				// if new cell is not a door, make old cell empty
				map[oldY][oldX] = EMPTY;
				drawBlackBox(oldY, oldX);
			} else {
				// if new cell is wall WALL or door DOOR
				// stay on previous cell
				pacmanY = oldY;
				pacmanX = oldX;
				// reset movement vector (PACMAN stops)
				dx = 0;
				dy = 0;
			}

			// draw pacman at current map cell coordinates
			map[pacmanY][pacmanX] = PACMAN;

			if (MODE_PORT2_MASTER == controllerPort2Mode) {
				// send Pac-Man state to slave console
				pacManStateToTransferObject();
				LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_PAC_MAN_STATE, LINK_TYPES_LENGTH);
			}

			// if all FOOD and POWER_FOOD are eaten - PACMAN won
			if (winner()) {

				// victory sound
				XGM_startPlay(victory_vgm);


				dxPacGirl = 0;
				dyPacGirl = 0;
				dx = 0;
				dy = 0;
				dxRed = 0;
				dyRed = 0;

				calcScore();

		    	if (MODE_PORT2_MASTER == controllerPort2Mode) {
		    		// send to slave console that game ended
		    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_END_GAME, LINK_TYPES_LENGTH);
		    	}

				return 0;
			}

			// did PACMAN eat the ghost (or did it eat us)
			if (pacmanLooser()) {

				// game over sound
				XGM_startPlay(fatality_vgm);
				return 0;
			}

		 }

	}
	return 1;
}


/**
 * PAC-GIRL movement processing algorithm on the map
 * return 0 - End of game
 *        1 - PACMAN is still alive
 */
u8 pacGirlState() {
	// check if pacGirl has movement direction set
	if (dxPacGirl != 0 || dyPacGirl != 0) {

		// if 2nd player connected
		if (players == 1) {
			players = 2;
			if (map[pacGirlY][pacGirlX] == FOOD) {
				incFood();

		    	if (MODE_PORT2_MASTER == controllerPort2Mode) {
		    		// send to slave console that white dot (food) was eaten
		    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_EAT_POINT, LINK_TYPES_LENGTH);
		    	}
			}
		}

		if (pacGirlLastUpdateTime == 0) {
			oldPacGirlX = pacGirlX;
			oldPacGirlY = pacGirlY;
			pacGirlX = pacGirlX + dxPacGirl;
			pacGirlY = pacGirlY + dyPacGirl;

			pacGirlLastUpdateTime = PACGIRL_SPEED;

			// if went beyond the field (appearing from the other side of the field)
			moveBound(&pacGirlX, &pacGirlY);

			// if current cell has food, increase eaten counter
			val = map[pacGirlY][pacGirlX];
			if (val == FOOD) {
				incFood();
		    	if (MODE_PORT2_MASTER == controllerPort2Mode) {
		    		// send to slave console that white dot (food) was eaten
		    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_EAT_POINT, LINK_TYPES_LENGTH);
		    	}
			} else if (val == POWER_FOOD) {
				// RED becomes edible
				redFlag = 0;
				// runs in the opposite direction
				dxRed = -dxRed;
				dyRed = -dyRed;

				// RED became edible
				redTime = RED_TIME;

				// and give additional bonus
				++powerBonus;

				// sound of eating power-up
				XGM_startPlayPCM(SFX_SOUND_POWERUP, 15, SOUND_PCM_CH2);

		    	if (MODE_PORT2_MASTER == controllerPort2Mode) {
		    		// send to slave console that power-up (green dot) was eaten
		    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_EAT_POWERUP, LINK_TYPES_LENGTH);
		    	}

			} else if (val == CHERRY) {
				++cherryBonus;

				// hide cherry
				SPR_setPosition(cherrySprite, -90, 100);
				refreshCherry = 0;

				// sound of eating cherry
				XGM_startPlayPCM(SFX_SOUND_CHERRY, 15, SOUND_PCM_CH2);

		    	if (MODE_PORT2_MASTER == controllerPort2Mode) {
		    		// send to slave console that cherry was eaten
		    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_EAT_CHERRY, LINK_TYPES_LENGTH);
		    	}
			}

			if (isNotWallOrDoor(pacGirlY, pacGirlX)) {
				// if new cell is not a door, make old cell empty
				oldPacGirlVal = val;
				map[oldPacGirlY][oldPacGirlX] = EMPTY;
				drawBlackBox(oldPacGirlY, oldPacGirlX);
			} else {
				// if new cell is wall WALL or door DOOR
				// stay on previous cell
				pacGirlY = oldPacGirlY;
				pacGirlX = oldPacGirlX;
				// reset movement vector (PAC-GIRL stops)
				dxPacGirl = 0;
				dyPacGirl = 0;
			}

			// draw PAC-GIRL at current map cell coordinates
			map[pacGirlY][pacGirlX] = PACGIRL;

			if (MODE_PORT2_MASTER == controllerPort2Mode) {
				// send Pac-Girl state to slave console
				pacGirlStateToTransferObject();
				LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_PAC_GIRL_STATE, LINK_TYPES_LENGTH);
			}

			// if all FOOD and POWER_FOOD are eaten - PACMAN won
			if (winner()) {

				// victory sound
				XGM_startPlay(victory_vgm);

				dxPacGirl = 0;
				dyPacGirl = 0;
				dx = 0;
				dy = 0;
				dxRed = 0;
				dyRed = 0;

				calcScore();

		    	if (MODE_PORT2_MASTER == controllerPort2Mode) {
		    		// send to slave console that game ended
		    		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_END_GAME, LINK_TYPES_LENGTH);
		    	}

				return 0;
			}

			// did PACMAN eat the ghost (or did it eat us)
			if (pacmanLooser()) {
				// game over sound
				XGM_startPlay(fatality_vgm);
				return 0;
			}
		}
	}

	return 1;
}


/**
 * Ghost chasing PAC-MAN algorithm
 * return 0 - End of game
 *        1 - PACMAN is still alive
 */
u8 redState() {

	// should RED switch to chase mode
	if (redTime == 0 ) {
		redFlag = 1;
		// if not moving, let it go up
		if (dyRed == 0 && dxRed == 0) {
			dyRed = -1;
			dxRed = 0;
		}
	} else if (redLastUpdateTime == 0 && dyRed == 0 && dxRed == 0) {
		// ghost cannot move yet
		redLastUpdateTime = RED_SPEED;
	}

	// check if RED has movement direction set
	if (dxRed != 0 || dyRed != 0) {
		if (redLastUpdateTime == 0) {
			// can update ghost coordinates
			// remember old ghost coordinates
			oldXRed = redX;
			oldYRed = redY;

			// change ghost coordinates (dxRed and dyRed cannot be greater than 0 simultaneously!)
			redX = redX + dxRed;
			redY = redY + dyRed;

			redLastUpdateTime = RED_SPEED;

			// went beyond boundaries
			moveBound(&redX, &redY);

			if (isNotWall(redY, redX)) {
				// current coordinates are not an obstacle
				map[oldYRed][oldXRed] = oldRedVal;
				oldRedVal = map[redY][redX];

				if (redX == 15 && redY >= 7 && redY <= 10) {
					// ghost always exits ghost house upwards
					// and cannot go back!
					dyRed = -1;
					dxRed = 0;
				} else if (dxRed != 0) {
					// ghost is moving along X axis
					if (redFlag && redY != pacmanY) {
						// ghost is not edible and hasn't caught Pac-Man
						if (isNotWallOrDoor(redY + 1, redX)	&& isNotWallOrDoor(redY - 1, redX)) {
							// there is an alternative path along x axis
							if (abs(redY + 1 - pacmanY) < abs(redY - 1 - pacmanY)) {
								// path to Pac-Man downwards is shorter to Pac-Man by Y
								dyRed = 1;
							} else {
								// path to Pac-Man upwards is shorter to Pac-Man by Y
								dyRed = -1;
							}
						} else if (isNotWallOrDoor(redY + 1, redX)) {
							// there is a path down
							if (abs(redY + 1 - pacmanY) < abs(redY - pacmanY)) {
								// path down is shorter to Pac-Man by Y
								dyRed = 1;
							}
						} else if (isNotWallOrDoor(redY - 1, redX)) {
							// there is a path up
							if (abs(redY - 1 - pacmanY) < abs(redY - pacmanY)) {
								// path up is shorter to Pac-Man by Y
								dyRed = -1;
							}
						}
					} else {
						// Ghost is edible
						if (isNotWallOrDoor(redY + 1, redX)) {
							// if there is another path, choose randomly where ghost will go
							dyRed = random() % 2;
						}

						if (isNotWallOrDoor(redY - 1, redX)) {
							// if there is another path, choose randomly where ghost will go
							dyRed = -1 * (random() % 2);
						}
					}

					if (dyRed != 0) {
						// if movement direction changes from x to y, need to reset x offset
						dxRed = 0;
					}

				} else if (dyRed != 0) {
					// ghost is moving along Y axis
					if (redFlag && redX != pacmanX) {
						// ghost is not edible and hasn't caught Pac-Man
						if (isNotWallOrDoor(redY, redX + 1)	&& isNotWallOrDoor(redY, redX - 1)) {
							// there is an alternative path, choose which is shorter by X
							if (abs(redX + 1 - pacmanX) < abs(redX - 1 - pacmanX)) {
								// path to the right is shorter to Pac-Man by X
								dxRed = 1;
							} else {
								// path to the left is shorter to Pac-Man by X
								dxRed = -1;
							}
						} else if (isNotWallOrDoor(redY, redX + 1)) {
							// there is an alternative path to the right
							if (abs(redX + 1 - pacmanX) < abs(redX - pacmanX)) {
								// path to the right is shorter to Pac-Man by X
								dxRed = 1;
							}
						} else if (isNotWallOrDoor(redY, redX - 1)) {
							// there is an alternative path to the left for ghost
							if (abs(redX - 1 - pacmanX) < abs(redX - pacmanX)) {
								// path to the left is shorter to Pac-Man by X
								dxRed = -1;
							}

						}
					} else {
						// Ghost is edible
						if (isNotWallOrDoor(redY, redX + 1)) {
							// if there is another path, choose randomly where ghost will go
							dxRed = random() % 2;
						}

						if (isNotWallOrDoor(redY, redX - 1)) {
							// if there is another path, choose randomly where ghost will go
							dxRed = -1 * (random() % 2);
						}

					}

					if (dxRed != 0) {
						// if movement direction changes from y to x, need to reset y offset
						dyRed = 0;
					}
				}
			} else {
				// current ghost coordinates are an obstacle, need to change movement direction
				if (redX == 15 && redY >= 7 && redY <= 10) {
					// from ghost house it always exits upwards
					dyRed = -1;
					dxRed = 0;
				} else {
					// return ghost to previous coordinates
					redX = oldXRed;
					redY = oldYRed;

					if (dxRed != 0) {
						// if ghost was moving along x axis, need to stop it
						dxRed = 0;
						if (isNotWallOrDoor(redY + 1, redX)) {
							// can move down
							dyRed = 1;
						} else if (isNotWallOrDoor(redY - 1, redX)) {
							// can move up
							dyRed = -1;
						}
					} else {
						// if was moving along y axis
						dyRed = 0;
						if (isNotWallOrDoor(redY, redX + 1)) {
							// can move right
							dxRed = 1;
						} else if (isNotWallOrDoor(redY, redX - 1)) {
							// can move left
							dxRed = -1;
						}
					}
				}

			}

			if (MODE_PORT2_MASTER == controllerPort2Mode) {
				// send ghost state to slave console
				redStateToTransferObject();
				LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_RED_STATE, LINK_TYPES_LENGTH);
			}

			// did PACMAN eat the ghost (or did it eat us)
			if (pacmanLooser()) {
				// game over music
				XGM_startPlay(fatality_vgm);
				// Pac-Man was eaten, game over
				return 0;
			}

		}
	}

	if (redFlag) {
		// display red ghost sprite
		map[redY][redX] = RED;
	} else {
		// display fleeing ghost sprite
		map[redY][redX] = SHADOW;
	}

	// Pac-Man not eaten, game continues
	return 1;
}


/**
 *  Redraw background tile when a dot was eaten
 *  draw black square 8x8
 */
void drawBlackBox(s16 y, s16 x) {
	// display empty square
	VDP_setTileMapXY(BG_A, 1, x + 4, y);
}


/**
 * For debugging, put number value as characters in text buffer
 * 4 char type characters, number is displayed in hexadecimal format
 * for example, decimal number 65535 will be displayed as FFFF
 *
 * val - number that we want to display on screen
 */
void printU16(u16 val) {
	u8 ch = '0';

	for (s16 j = 3; j >= 0; j--) {
		ch = (val & LCP_LO_BITS);
		switch (ch) {
		case 10:
			ch = 'A';
			break;
		case 11:
			ch = 'B';
			break;
		case 12:
			ch = 'C';
			break;
		case 13:
			ch = 'D';
			break;
		case 14:
			ch = 'E';
			break;
		case 15:
			ch = 'F';
			break;
		default:
			ch = ch + '0';
		}
		text[j] = ch;

		// shift 4 bits to the right
		val >>= 4;
	}
}


/**
 *  Draw bonuses, score,
 *  game result (GAME OVER or YOU WINNER)
 */
void drawText() {

	switch (showLinkCableErrors) {
		case SHOW_LINK_CABLE_LAST_ERROR:
			// display errors during transmission via Link Cable
			printU16(linkCableErrors);
			VDP_drawText(text, 28, 25);
		break;

		case SHOW_LINK_CABLE_ERRORS_COUNT:
			// display number of errors during transmission via Link Cable
			printU16(linkCableErrorsCount);
			VDP_drawText(text, 28, 25);
		break;

		case SHOW_LINK_CABLE_FRAME_COUNT:
			// number of rendered frames since connection establishment via Link Cable
			printU16(linkCableFrameCount);
			VDP_drawText(text, 28, 25);
		break;
	}

	if (STATE_SCREENSAVER != gameState) {
		// set text color for VDP_drawText() function to WHITE
		PAL_setColor(15,RGB24_TO_VDPCOLOR(0xffffff));

		// are we playing via Link Cable on two consoles or playing on one console with one or two controllers
		// "LINK MASTER" - (TEXT_LINK_MASTER) playing via Link Cable on two consoles and our console is master
		// "LINK  SLAVE" - (TEXT_LINK_SLAVE) playing via Link Cable on two consoles and our console is slave
		// "2P NO Link!" - (TEXT_2P_NO_LINK) playing not via Link Cable and controller connected to port 2, 2 players playing on one console
		// "1P NO LINK!" - (TEXT_1P_NO_LINK) playing not via Link Cable and no controller in port 2, 1 player playing
		// "TRY MASTER!" - (TEXT_TRY_MASTER) attempt to create connection with another console as master
		// "TRY  SLAVE!" - (TEXT_TRY_SLAVE) attempt to create connection with another console as slave
		VDP_drawText(gameModeText, 14, 25);
	}

	memset(text, 0, 4);

	if (STATE_GAME == gameState || STATE_RESULT == gameState) {
		// game is in progress or displaying game result

		// set text color for VDP_drawText() function to WHITE
		PAL_setColor(15,RGB24_TO_VDPCOLOR(0xffffff));

		// number of eaten cherries
		text[0] = cherryBonus + '0';
		text[1] = 0;
		VDP_drawText(text, 10, 26);

		// number of eaten ghosts
		text[0] = redBonus + '0';
		VDP_drawText(text, 10, 24);

        // number of eaten power-ups
		text[0] = powerBonus + '0';
		VDP_drawText(text, 27, 26);

		// number of eaten gray dots (food)
		text[0] = food100 + '0';
		text[1] = food010 + '0';
		text[2] = food001 + '0';
		VDP_drawText(text, 27, 24);
		VDP_drawText("/271", 30, 24);
	}

	if (STATE_RESULT == gameState) {
		SYS_doVBlankProcess();
		// display game result
		if (winner()) {
			// set text color for VDP_drawText() function to GREEN
			PAL_setColor(15,RGB24_TO_VDPCOLOR(0x00ff00));

			// if won, write 'YOU WINNER'
			VDP_drawText("YOU WINNER", 14, 24);
		} else {
			// set text color for VDP_drawText() function to RED
			PAL_setColor(15,RGB24_TO_VDPCOLOR(0xff0000));

			// if lost, write 'GAME OVER'
			VDP_drawText("GAME OVER", 14, 24);
		}
		// write 'SCORE'
		VDP_drawText("SCORE ", 14, 26);

		// display number of points earned
		// taking into account all bonuses
		// cherry 200 points
		// eaten ghost 50 points
		// power-up 25 points
		// gray dot (food) 1 point

		text[0] = score100 + '0';
		text[1] = score010 + '0';
		text[2] = score001 + '0';
		VDP_drawText(text, 20, 26);
	}
}


/**
 * Draw sprites
 */
void drawSprites() {
	switch (gameState) {
		case STATE_SCREENSAVER:
			// screensaver
			screensaver();
		break;
		case STATE_SELECT:
			// start screen
			if (players == 1) {
				// hide Pac-Girl
				SPR_setPosition(pacGirlSprite, -100, 90);

				// single player game (only Pac-Man)
				// draw PAC-MAN sprite before 1 PLAYER
				SPR_setAnim(pacmanSprite, 0);
				SPR_setHFlip(pacmanSprite, FALSE);
				SPR_setPosition(pacmanSprite, 100, 100);
			} else {
				// two players game mode
				if (P1_PACGIRL__P2_PACMAN == switchPlayers) {
					// (player 1: Pac-Girl, player 2: Pac-Man)

					// draw PAC-Girl sprite before 2 PLAYERS
					SPR_setAnim(pacGirlSprite, 0);
					SPR_setHFlip(pacGirlSprite, FALSE);
					SPR_setPosition(pacGirlSprite, 100, 113);

					// draw PAC-MAN sprite after 2 PLAYERS
					SPR_setAnim(pacmanSprite, 0);
					SPR_setHFlip(pacmanSprite, TRUE);
					SPR_setPosition(pacmanSprite, 190, 113);
				} else {
					// (player 1: Pac-Man, player 2: Pac-Girl)

					// draw PAC-MAN sprite before 2 PLAYERS
					SPR_setAnim(pacmanSprite, 0);
					SPR_setHFlip(pacmanSprite, FALSE);
					SPR_setPosition(pacmanSprite, 100, 113);

					// draw PAC-Girl sprite after 2 PLAYERS
					SPR_setAnim(pacGirlSprite, 0);
					SPR_setHFlip(pacGirlSprite, TRUE);
					SPR_setPosition(pacGirlSprite, 190, 113);
				}
			}
		break;
		case STATE_GAME:
		case STATE_RESULT:
			// game in progress or showing results

			// draw game sprites:
			// PAC-MAN, Pac-Girl, RED or SHADOW, door, cherry
			refreshGame();
		break;
		case STATE_PAUSE:
			// game paused

			// draw sprite with PAUSE text
			SPR_setAnim(pauseSprite, 0);
			SPR_setPosition(pauseSprite, pauseX, pauseY);

			// draw running Sonic sprite
			SPR_setAnim(sonicSprite, 3);
			SPR_setPosition(sonicSprite, sonicX, sonicY);
		break;
	}
}


/**
 * Draw only the object from map[i][j]
 * i - row in the map array
 * j - column in the map array
 */
void draw(s16 i, s16 j) {
	drawSprite(i, j,  map[i][j]);
}


/**
 * Draw the passed object at coordinates
 * i - row in the map array
 * j - column in the map array
 * val - What to draw at coordinates
 */
void drawSprite(s16 i, s16 j, u8 val) {
	// x = j * 8 and 29 pixels to the right
    x = (j << 3) + 29;

    // y = i * 8 and 2 pixels up
    y = (i << 3) - 2;

    if (val == PACMAN) {
		if (dx < 0) {
			// PACMAN moving left
			SPR_setAnim(pacmanSprite, 0);
			SPR_setHFlip(pacmanSprite, TRUE);
			SPR_setPosition(pacmanSprite, x, y);
		} else if (dx > 0) {
			// PACMAN moving right
			SPR_setAnim(pacmanSprite, 0);
			SPR_setHFlip(pacmanSprite, FALSE);
			SPR_setPosition(pacmanSprite, x, y);
		} else if (dy < 0) {
			// PACMAN moving up
			SPR_setAnim(pacmanSprite, 1);
			SPR_setVFlip(pacmanSprite, FALSE);
			SPR_setPosition(pacmanSprite, x, y);
		} else if (dy > 0) {
			// PACMAN moving down
			SPR_setAnim(pacmanSprite, 1);
			SPR_setVFlip(pacmanSprite, TRUE);
			SPR_setPosition(pacmanSprite, x, y);
		} else {
			// PACMAN standing still
			SPR_setAnim(pacmanSprite, 2);
			SPR_setPosition(pacmanSprite, x, y);
		}
    } else if (val == RED) {
        if (dxRed < 0) {
          	// RED moving left
        	SPR_setAnim(redSprite, 0);
        	SPR_setHFlip(redSprite, TRUE);
        	SPR_setPosition(redSprite, x, y);
		} else if (dxRed > 0) {
			// RED moving right
        	SPR_setAnim(redSprite, 0);
        	SPR_setHFlip(redSprite, FALSE);
        	SPR_setPosition(redSprite, x, y);
		} else if (dyRed > 0) {
			// RED moving down
        	SPR_setAnim(redSprite, 2);
        	SPR_setPosition(redSprite, x, y);
		} else if (dyRed < 0) {
			// RED moving up
			SPR_setAnim(redSprite, 1);
        	SPR_setPosition(redSprite, x, y);
		} else {
			// RED standing still
			SPR_setAnim(redSprite, 4);
        	SPR_setPosition(redSprite, x, y);
		}

    } else if (val == SHADOW) {
		// logic for playing sound when ghost can be eaten
    	if (shadowLastSoundTime == 0) {
    		// sound when ghost is edible
    		XGM_startPlayPCM(SFX_SOUND_SHADOW, 15, SOUND_PCM_CH3);
    		shadowLastSoundTime = 20;
    	}

    	if (dxRed != 0 || dyRed != 0) {
    		// ghost moving
    		SPR_setAnim(redSprite, 3);
    		SPR_setPosition(redSprite, x, y);
    	} else {
    		// ghost standing still
    		SPR_setAnim(redSprite, 5);
    		SPR_setPosition(redSprite, x, y);
    	}
    } else if (val == PACGIRL) {
		if (dxPacGirl < 0) {
			// PACGIRL moving left
			SPR_setAnim(pacGirlSprite, 0);
			SPR_setHFlip(pacGirlSprite, TRUE);
			SPR_setVFlip(pacGirlSprite, FALSE);
			SPR_setPosition(pacGirlSprite, x, y);
		} else if (dxPacGirl > 0) {
			// PACGIRL moving right
			SPR_setAnim(pacGirlSprite, 0);
			SPR_setHFlip(pacGirlSprite, FALSE);
			SPR_setVFlip(pacGirlSprite, FALSE);
			SPR_setPosition(pacGirlSprite, x, y);
		} else if (dyPacGirl < 0) {
			// PACGIRL moving up
			SPR_setAnim(pacGirlSprite, 1);
			SPR_setVFlip(pacGirlSprite, FALSE);
			SPR_setHFlip(pacGirlSprite, FALSE);
			SPR_setPosition(pacGirlSprite, x, y);
		} else if (dyPacGirl > 0) {
			// PACGIRL moving down
			SPR_setAnim(pacGirlSprite, 1);
			SPR_setVFlip(pacGirlSprite, TRUE);
			SPR_setHFlip(pacGirlSprite, FALSE);
			SPR_setPosition(pacGirlSprite, x, y);
		} else {
			// PACGIRL standing still
			SPR_setAnim(pacGirlSprite, 2);
			SPR_setHFlip(pacGirlSprite, FALSE);
			SPR_setVFlip(pacGirlSprite, FALSE);
			SPR_setPosition(pacGirlSprite, x, y);
		}

    } else if (val == CHERRY) {
    	// cherry
    	SPR_setPosition(cherrySprite, x, y);
    } else if (val == DOOR) {
    	// door
    	SPR_setPosition(doorSprite, x, y);
    }
}


/**
 * Update map, characters, doors, cherry on screen
 */
void refreshGame() {
	if (!cherryFlag && redTime == 0 && !cherryBonus && cherryTime == 0) {
		// open doors
		openDoors();

		if (MODE_PORT2_MASTER == controllerPort2Mode) {
			transferObject[0] = 0xAF;
			transferObject[1] = 0xFA;
			LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_OPEN_DOOR, LINK_TYPES_LENGTH);
		}

	}

    if (refreshDoor) {
		if (map[doorY][doorX] != DOOR) {
			refreshDoor = 0;
			SPR_setPosition(doorSprite, -90, 100);

			if (MODE_PORT2_MASTER == controllerPort2Mode) {
				transferObject[0] = 0xAF;
				transferObject[1] = 0xFA;
				LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_OPEN_DOOR, LINK_TYPES_LENGTH);
			}

		} else {
			// draw door
			draw(doorY, doorX);
		}
    }

    if (refreshCherry) {
		if (map[cherryY][cherryX] != CHERRY) {
			SPR_setPosition(cherrySprite, -90, 100);
		} else {
		    // draw cherry
	    	drawSprite(cherryY, cherryX, CHERRY);
		}
    }

    // draw RED ghost
    draw(redY, redX);


    // draw Pac-Man
    draw(pacmanY, pacmanX);


    // draw Pac-Girl
    draw(pacGirlY, pacGirlX);
}


/**
 * Draw background
 */
void drawBackground() {
	switch(gameState) {
		case STATE_SCREENSAVER:
			// screensaver

			// draw SGDK logo background from sgdk.png
			VDP_drawImage(BG_A, &sgdk_image, 0, 0);
		break;
		case STATE_SELECT:
			// start screen background

			// draw player selection menu as background from menu.png
			VDP_drawImage(BG_A, &menu_image, 0, 0);
		break;
		default:
			// any other screen

			// level map with maze
			// draw level map as background from map.png
			VDP_drawImage(BG_A, &map_image, 0, 0);
	}
}


/**
 * Handle button presses by players during screensaver when they eat SGDK text
 * for gameState == STATE_SCREENSAVER
 */
void actionsStateScreensaver() {
	// screensaver
	// condition to end screensaver animation
	// animation simply ended: Pac-Girl X coordinate >= 380
	// or any button pressed on one of the joysticks (except Start)
	if ((pacGirlX >= 380)
			|| (pad1 & BUTTON_A)  || (pad1 & BUTTON_B) || (pad1 & BUTTON_C)
			|| (pad1 & BUTTON_X)  || (pad1 & BUTTON_Y) || (pad1 & BUTTON_Z)
			|| (pad2 & BUTTON_A)  || (pad2 & BUTTON_B) || (pad2 & BUTTON_C)
			|| (pad2 & BUTTON_X)  || (pad2 & BUTTON_Y) || (pad2 & BUTTON_Z)) {
		// need to remove all characters from screen
		// for this set them to initial state before screensaver start
		initScreensaver();
		// select number of players for game
		gameState = STATE_SELECT;
		// draw background with player selection menu
		drawBackground();
		// stop playing SEGA sound
		XGM_stopPlayPCM(SOUND_PCM_CH2);
		// music playing when displaying player count selection menu
		XGM_startPlay(contrah_vgm);
	}
}


/**
 * Handle button presses by players and game logic during player count selection screen display
 * for gameState == STATE_SELECT
 */
void actionsStateSelectPlayers() {
	// player count selection screen

	if (((pad1 & BUTTON_START) || (pad2 & BUTTON_START)) && playersTime == 0) {
		// Start pressed on 1st or 2nd joystick
		// stop playing music on player selection screen
		XGM_pausePlay();

		// reset game to initial state
		// initial character positions, reset scores, etc.
		resetGame();

		if (players != 2) {
			// 1 player game selected (1 PLAYER)
			// display message that there is no connection with another console, playing single player
			memcpy(gameModeText, TEXT_1P_NO_LINK, GAME_MODE_TEXT_SIZE);

			if ((MODE_PORT2_MASTER == controllerPort2Mode || MODE_PORT2_SLAVE == controllerPort2Mode)) {
				// set second port mode - not participating in game, player of our console
				// plays single player on their console with controller connected to first port
				controllerPort2Mode = MODE_SINGLE_PLAYER;

				// there is Link cable connection
				// close port, data will no longer be sent via Link cable
				LCP_close();

				// reset all inputs on 2nd controller
				pad2 = 0;

				// protection against double start press
				playersTime = 30;

				// connection disconnect sound
				XGM_startPlayPCM(SFX_SOUND_DISCONNECT_LINK_CABLE, 15, SOUND_PCM_CH3);

				// enable music when displaying player count selection menu
				XGM_startPlay(contrah_vgm);

				// exit, don't start game yet, need to press start again
				return;
			}
			// remove PAC-GIRL from map for player 1
			map[pacGirlY][pacGirlX] = FOOD;
		} else {
			// 2 player game selected (2 PLAYERS)

			if (!(MODE_PORT2_MASTER == controllerPort2Mode || MODE_PORT2_SLAVE == controllerPort2Mode)) {
				// if not yet determined what is plugged into console's 2nd port
				// or these are modes other than 2-player Link cable game
				// determine what is plugged in (controller, Link cable or nothing)
				// and which console will be which (master, slave)
				initControllerPort2();
			}

			if (controllerPort2Mode == MODE_SINGLE_PLAYER) {
				// failed to start 2-player game
				// no Link cable connection
				// and no 2nd controller inserted in SEGA port 2

			   // enable music when displaying player count selection menu
			   XGM_startPlay(contrah_vgm);

			   return;
			}

			// need to give points for the dot
			// that was in PAC-GIRL's place
			incFood();
		}

		// start game
		gameState = STATE_GAME;

		// draw background with maze for game
		drawBackground();

		// music playing during game
		XGM_startPlay(comicszone_vgm);

		// repeated press allowed only after 30 frames (to prevent immediate pause)
		playersTime = 30;
		return;
	}


	// 1 or 2 players will play - selection with arrow buttons
	if (((pad1 & BUTTON_DOWN) || (pad2 & BUTTON_DOWN)) && (players == 1)) {
		// Down button pressed on 1st or 2nd joystick
		players = 2;
		return;
	}


	if (((pad1 & BUTTON_UP) || (pad2 & BUTTON_UP)) && (players == 2)) {
		// Up button pressed on 1st or 2nd joystick
		players = 1;
		return;
	}

	if (((pad1 & BUTTON_RIGHT) || (pad2 & BUTTON_RIGHT)) && (switchPlayers == P1_PACMAN__P2_PACGIRL) && (players == 2)) {
		// Right button pressed on 1st or 2nd joystick
		switchPlayers = P1_PACGIRL__P2_PACMAN;
		return;
	}

	if (((pad1 & BUTTON_LEFT) || (pad2 & BUTTON_LEFT)) && (switchPlayers == P1_PACGIRL__P2_PACMAN) && (players == 2)) {
		// Left button pressed on 1st or 2nd joystick
		switchPlayers = P1_PACMAN__P2_PACGIRL;
		return;
	}
}


/**
 * Handle button presses by players and game logic during pause
 * for gameState == STATE_PAUSE
 */
void actionsStatePause() {
	if (((pad1 & BUTTON_START) || (pad2 & BUTTON_START)) && playersTime == 0 && MODE_PORT2_SLAVE != controllerPort2Mode) {
		resumeGame();
		if (MODE_PORT2_MASTER == controllerPort2Mode) {
			// send message to slave console to exit pause mode
			LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_RESUME_GAME, LINK_TYPES_LENGTH);
		}
		return;
	}

	sonicX += dxSonic;

	if (sonicX > 340) {
		dxSonic = 0;

		pauseX += pauseDX;
		pauseY += pauseDY;

		if (pauseY <= -18 || pauseY >= 200) {
			pauseDY = -pauseDY;
		}

		if (pauseX <= 1 || pauseX >= 280) {
			pauseDX = -pauseDX;
		}

	} else {
		pauseX += (dxSonic - 1);
	}
}


/**
 * Handle button presses by players and game logic during actual gameplay
 * for gameState == STATE_GAME
 */
void actionsStateGame() {
	if (MODE_PORT2_SLAVE != controllerPort2Mode) {
		// if not slave console, then game logic needs to be executed
		if (((pad1 & BUTTON_START) || (pad2 & BUTTON_START)) && playersTime == 0) {
			// start pressed on 1st or 2nd controller, with multi-press protection
			pause();
			if (MODE_PORT2_MASTER == controllerPort2Mode) {
				// send message to slave console to enter pause mode
				LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_PAUSE, LINK_TYPES_LENGTH);
			}

			return;
		}

		if (pad1 &  BUTTON_LEFT) {
			// left button pressed on 1st joystick
			dx = -1;
			dy = 0;
		}

		if (pad1 & BUTTON_RIGHT) {
			// right button pressed on 1st joystick
			dx = 1;
			dy = 0;

		}

		if (pad1 & BUTTON_UP) {
			// up button pressed on 1st joystick
			dy = -1;
			dx = 0;
		}

		if (pad1 & BUTTON_DOWN) {
			// down button pressed on 1st joystick
			dy = 1;
			dx = 0;
		}

		if (pad2 & BUTTON_UP) {
			// up button pressed on 2nd joystick
			dyPacGirl = -1;
			dxPacGirl = 0;
		}

		if (pad2 & BUTTON_DOWN) {
			// down button pressed on 2nd joystick
			dyPacGirl = 1;
			dxPacGirl = 0;
		}

		if (pad2 & BUTTON_LEFT) {
			// left button pressed on 2nd joystick
			dxPacGirl = -1;
			dyPacGirl = 0;
		}

		if (pad2 & BUTTON_RIGHT) {
			// right button pressed on 2nd joystick
			dxPacGirl = 1;
			dyPacGirl = 0;
		}

		// move Pac-Man
		if (!pacManState()) {
			// game over
			gameState = STATE_RESULT;
			return;
		}

		// move RED
		if (!redState()) {
			// game over
			gameState = STATE_RESULT;
			return;
		}


		// move Pac-Girl
		if (!pacGirlState()) {
			// game over
			gameState = STATE_RESULT;
			return;
		}


		if (redLastUpdateTime > 0) {
			// counter for RED and SHADOW animation
			--redLastUpdateTime;
		}


		if (cherryTime > 0) {
			// counter for when to show cherry
			// and open door to it
			--cherryTime;
		}

		if (redTime > 0) {
			// counter for when SHADOW becomes RED again
			--redTime;
		}
	}

	if (pacmanLastUpdateTime > 0 ) {
		// counter for Pac-Man animation
		--pacmanLastUpdateTime;
	}

	if (pacGirlLastUpdateTime > 0) {
		// counter for Pac-Girl animation
		--pacGirlLastUpdateTime;
	}

	if (shadowLastSoundTime > 0) {
		--shadowLastSoundTime;
	}
}


/**
 * Handle button presses by players and game logic when showing results screen after game ends
 * for gameState == STATE_RESULT
 */
void actionsStateResult() {
	if ((pad1 & BUTTON_START) || (pad2 & BUTTON_START)) {
		// showing game results on this screen
		// Start pressed on 1st or 2nd joystick
		// transition to screensaver screen
		gameState = STATE_SCREENSAVER;
		// protection against repeated Start button press
		playersTime = 30;
		// need to remove all characters from screen
		// for this set them to initial state before screensaver start
		initScreensaver();
		// draw SGDK on background
		drawBackground();
		// stop playing music
		XGM_stopPlay();
		// play SEGA sound!
		XGM_startPlayPCM(SFX_SOUND_SEGA, 15, SOUND_PCM_CH2);
	}
}


/**
 * Handle button presses by player and main game logic based on player actions
 */
void actions() {

	// delay for button press processing (if needed)
	if (playersTime > 0) {
		// protection against double press,
		// when playersTime becomes 0, handler will work again
		--playersTime;
	}


	if (((pad1 & BUTTON_A) && (pad1 & BUTTON_C)) || ((pad2 & BUTTON_A) && (pad2 & BUTTON_C))) {
		// display Link cable protocol errors on screen
		showLinkCableErrors = SHOW_LINK_CABLE_LAST_ERROR;
	}

	if (((pad1 & BUTTON_B) && (pad1 & BUTTON_C)) || ((pad2 & BUTTON_B) && (pad2 & BUTTON_C))) {
		// display error count for Link cable protocol on screen
		showLinkCableErrors = SHOW_LINK_CABLE_ERRORS_COUNT;
	}

	if (((pad1 & BUTTON_A) && (pad1 & BUTTON_X)) || ((pad2 & BUTTON_A) && (pad2 & BUTTON_X))) {
		// display error count for Link cable protocol on screen
		showLinkCableErrors = SHOW_LINK_CABLE_FRAME_COUNT;
	}

	if (((pad1 & BUTTON_A) && (pad1 & BUTTON_B)) || ((pad2 & BUTTON_A) && (pad2 & BUTTON_B))) {
		// reset data transfer errors to 0 on screen
		linkCableErrors = 0;
		// reset error count
		linkCableErrorsCount = 0;
		// reset frame count
		linkCableFrameCount = 0;
	}

	switch (gameState) {
		case STATE_SCREENSAVER:
				// screensaver
				actionsStateScreensaver();
		break;
		case STATE_SELECT:
				// player count selection screen
				actionsStateSelectPlayers();
		break;
		case STATE_PAUSE:
				// pause (when start pressed during game)
				actionsStatePause();
		break;
		case STATE_GAME:
				// game in progress
				actionsStateGame();
		break;
		case STATE_RESULT:
				// game result
				actionsStateResult();
		break;
	}
}


/**
 * Reset variable values for screensaver display
 * and draw them in new positions
 */
void initScreensaver() {
    sonicX = -95;
    sonicY = 83;
    dxSonic = 1;
    pacmanX = -100;
    pacmanY = 105;
    dx = 1;
    redX = 400;
    redY = 103;
    dxRed = 0;
    pacGirlX = -100;
    pacGirlY = 90;
    dxPacGirl = 0;
    doorVal = DOOR;
    cherryVal = EMPTY;

    SPR_setAnim(pacmanSprite, 0);
    SPR_setAnim(redSprite, 0);
    SPR_setAnim(pacGirlSprite, 0);

	SPR_setHFlip(redSprite, FALSE);
	SPR_setVFlip(redSprite, FALSE);
	SPR_setHFlip(pacmanSprite, FALSE);
	SPR_setVFlip(pacmanSprite, FALSE);
	SPR_setHFlip(pacGirlSprite, FALSE);
	SPR_setVFlip(pacGirlSprite, FALSE);

    // sonic walking - line 2 of animation in sonic.png file counting from 0
    SPR_setAnim(sonicSprite, 2);

    // change Sonic sprite position
    SPR_setPosition(sonicSprite, sonicX, sonicY);

    // change Pac-Man sprite position
    SPR_setPosition(pacmanSprite, pacmanX, pacmanY);

    // change RED sprite position
    SPR_setPosition(redSprite, redX, redY);

    // change Pac-Girl sprite position
    SPR_setPosition(pacGirlSprite, pacGirlX, pacGirlY);

	// hide cherry
	SPR_setPosition(cherrySprite, -90, 100);

	// hide door
	SPR_setPosition(doorSprite, -90, 100);
}


/**
 * display screensaver
 * Sonic running away from Pac-Man and SGDK text that will be eaten by Pac-Man and Pac-Girl
 * then Pac-Man runs away from ghost
 */
void screensaver() {
   SPR_setAnim(pacmanSprite, 0);
   SPR_setAnim(redSprite, 0);
   SPR_setAnim(pacGirlSprite, 0);

   // flip RED sprite horizontally
   SPR_setHFlip(redSprite, TRUE);

   if (dx < 0) {
	   SPR_setHFlip(pacmanSprite, TRUE);
   } else {
	   SPR_setHFlip(pacmanSprite, FALSE);
   }

   // set which animation to use
   if (sonicX <= 30)  {
	   // sonic walking - line 2 of animation in sonic.png file counting from 0
	   SPR_setAnim(sonicSprite, 2);
   } else if (sonicX <= 50)  {
	   dxSonic = 2;
	   // sonic walking - line 2 of animation in sonic.png file counting from 0
	   SPR_setAnim(sonicSprite, 2);
   } else if (sonicX <= 70) {
	   dxSonic = 3;
	   // sonic walking - line 2 of animation in sonic.png file counting from 0
	   SPR_setAnim(sonicSprite, 2);
	   XGM_startPlay(sonic_vgm);
   } else if (sonicX <= 120) {
	   dxSonic = 5;
	   // sonic running - line 3 of animation in sonic.png file
	   SPR_setAnim(sonicSprite, 3);
   } else {
	   dxSonic = 6;
	   // RED appears
	   dxRed = -1;
	   dxPacGirl = 1;
	   // sonic running - line 3 of animation in sonic.png file
	   SPR_setAnim(sonicSprite, 3);
   }

   if (pacmanX > 90 && pacmanX < 190 && dx > 0) {
	   // eating sound
	   if (pacmanLastUpdateTime <= 0) {
		   XGM_startPlayPCM(SFX_SOUND_EAT, 15, SOUND_PCM_CH2);
		   pacmanLastUpdateTime = 10;
	   } else {
		   pacmanLastUpdateTime--;
	   }
   }

   if (pacGirlX > 90 && pacGirlX < 190) {
	   if (pacGirlLastUpdateTime <= 0) {
		   XGM_startPlayPCM(SFX_SOUND_EAT, 15, SOUND_PCM_CH2);
		   pacGirlLastUpdateTime = 10;
	   } else {
		   pacGirlLastUpdateTime--;
	   }
   }

   if (sonicX < 320) {
	   // calculate new Sonic coordinates
	   sonicX += dxSonic;
   }

   if (pacmanX < 210) {
	   // calculate new Pac-Man coordinates
	   pacmanX += dx;
   } else {
	   // time to run away from RED
	   dx = -1;
	   pacmanX = 209;
   }

   if (redX > -16) {
	   // calculate new RED coordinates
	   redX += dxRed;
   }

   pacGirlX+= dxPacGirl;


   // change Sonic sprite position
   SPR_setPosition(sonicSprite, sonicX, sonicY);

   // change Pac-Man sprite position
   SPR_setPosition(pacmanSprite, pacmanX, pacmanY);

   // change RED sprite position
   SPR_setPosition(redSprite, redX, redY);

   // change Pac-Girl sprite position
   SPR_setPosition(pacGirlSprite, pacGirlX, pacGirlY);


   // erase on background what Pac-Man ate
   if (pacmanX >=0 && pacmanX <= 320) {
	   // draw tile on background at Pac-Man coordinates (he will eat SGDK)
	   VDP_setTileMapXY(BG_A, 1, (pacmanX/8), (pacmanY/8));
	   VDP_setTileMapXY(BG_A, 1, (pacmanX/8), (pacmanY/8) + 1);
   }

   if (pacGirlX >= 0 && pacGirlX <= 320) {
	   // draw tile on background at Pac-Girl coordinates (he will eat SGDK)
	   VDP_setTileMapXY(BG_A, 1, (pacGirlX/8), (pacGirlY/8));
	   VDP_setTileMapXY(BG_A, 1, (pacGirlX/8), (pacGirlY/8) + 1);
   }

}


/**
 * Switch game to pause mode
 */
void pause() {
	gameState = STATE_PAUSE;
	playersTime = 30;
	SPR_setPosition(pacGirlSprite, -100, -100);
	SPR_setPosition(pacmanSprite, -100, -100);
	SPR_setPosition(redSprite, -100, -100);
	dxSonic = 8;
	sonicX = -10;
	pauseX = -60;
	pauseY = sonicY;
}


/**
 * Exit from pause, i.e. change state to continue game
 */
void resumeGame() {
	gameState = STATE_GAME;
	playersTime = 30;
	SPR_setPosition(pauseSprite, -100, -100);
	SPR_setPosition(sonicSprite, -100, -100);
}


/**
 * Playing on two consoles and our master console
 * send to slave console what was pressed on our controller
 * in case of character switching when selecting player count, send who plays which character
 * and also send all packet objects that haven't been sent yet
 * then parse what was received from slave console, specifically what was pressed on the controller
 * of the other console
 */
void masterControls() {
	// assume nothing is pressed on 2nd controller
	pad2 = 0;

	if (pad1) {
		// put in transferObject an object containing information about pressed buttons on 1st controller of our console
		// i.e. there will be OBJECT_TYPE_JOY object as byte array (if something was pressed)
		padToTransferObject(pad1);


		// add OBJECT_TYPE_JOY object to packet that will be sent to other console when calling LCP_masterCycle()
		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_JOY, LINK_TYPES_LENGTH);
	}

	if (STATE_SELECT == gameState) {
		// send information about which player plays which character
		// if switchPlayers == 0 - master is Pac-Man,  slave is Pac-Girl
		// if switchPlayers == 1 - master is Pac-Girl, slave is Pac-Man
		transferObject[0] = switchPlayers;

		// add OBJECT_TYPE_SWITCH_PLAYERS object to packet, will be sent to other console when calling LCP_masterCycle()
		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_SWITCH_PLAYERS, LINK_TYPES_LENGTH);
	}

	// send packet with data from our master console to slave console
	// via Link Cable (slave console will receive external interrupt EX-INT - External interrupt)
	// also in the same method receive from slave console packet with data for our console
	LCP_masterCycle();

	do {
		// try to get next object from received packet from slave console into transferObject
		objectType = LCP_getNextObjectFromReceivePacket(transferObject, LINK_TYPES_LENGTH);
		if (OBJECT_TYPE_JOY == objectType) {
			// the one playing on the other console, slave - second player by default plays as Pac-Girl
			// therefore what was pressed on the first controller of the other console we save in pad2
			pad2 = getPadFromTransferObject();

			// BUT if in '2 PLAYERS' menu RIGHT was pressed before game (switchPlayers == 1), then second
			// player will play as Pac-Man because at the end of this function pad1 and pad2 will be swapped in this case
		}
	} while (objectType != 0);

}


/**
 * Playing on two consoles and our slave console
 * save to packet for sending to master console what was pressed on our controller
 * packet will be sent asynchronously when external interrupt occurs initiated by master console
 * at random moment in time for our console (external interrupt handler - function LCP_slaveCycle() from link_cable.c library)
 * parse what was received from master console during previous external interrupt call (LCP_slaveCycle())
 */
void slaveControls() {
	// we play as second player, therefore what is pressed on first controller
	// we save to variable of 2nd controller
	pad2 = pad1;

	if (pad1) {
		// put in transferObject an object containing information about pressed buttons on 1st controller of our console
		// i.e. there will be OBJECT_TYPE_JOY object as byte array (if something was pressed)
		padToTransferObject(pad1);

		// add OBJECT_TYPE_JOY object to packet that will be sent to other console when calling LCP_slaveCycle()
		// at the moment of receiving external interrupt called by master console at random moment in time for us
		LCP_objectToPacketForSend(transferObject, OBJECT_TYPE_JOY, LINK_TYPES_LENGTH);
	}

	// assume that nothing is pressed on 1st controller at the moment
	pad1 = 0;

	do {
		// try to get next object from received packet from master console into transferObject
		objectType = LCP_getNextObjectFromReceivePacket(transferObject, LINK_TYPES_LENGTH);
		switch (objectType) {
			case OBJECT_TYPE_JOY:
				// the one playing on the other console, master - first player by default plays as Pac-Man
				// therefore what was pressed on the first controller of the other console we save in pad1
				pad1 = getPadFromTransferObject();

				// BUT if in '2 PLAYERS' menu RIGHT was pressed before game (switchPlayers == 1), then first
				// player will play as Pac-Girl because at the end of this function pad1 and pad2 will be swapped in this case
			break;
			case OBJECT_TYPE_SWITCH_PLAYERS:
				if (STATE_SELECT == gameState) {
					// received information about which player plays which character
					switchPlayers = transferObject[0];
				}
			break;
			case OBJECT_TYPE_PAC_MAN_STATE:
				// received dx, dy, pacmanX, pacmanY, oldX, oldY from master console
				pacManStateFromTransferObject();
				drawSprite(pacmanY, pacmanX, PACMAN);
				drawBlackBox(oldY, oldX);
			break;
			case OBJECT_TYPE_PAC_GIRL_STATE:
				// received dxPacGirl, dyPacGirl, pacGirlX, pacGirlY, oldPacGirlX, oldPacGirlY from master console
				pacGirlStateFromTransferObject();
				drawSprite(pacGirlY, pacGirlX, PACGIRL);
				drawBlackBox(oldPacGirlY, oldPacGirlX);
			break;
			case OBJECT_TYPE_RED_STATE:
				// received dxRed, dyRed, redX, redY, redFlag from master console
				redStateFromTransferObject();
				if (redFlag) {
					drawSprite(redY, redX, RED);
				} else {
					drawSprite(redY, redX, SHADOW);
				}
			break;
			case OBJECT_TYPE_END_GAME:
				// received game end event from master console
				// if our console is still in game state, need to play win or lose sound

				// immobilize everyone
				dxRed = 0;
				dyRed = 0;
				dx = 0;
				dy = 0;
				dxPacGirl = 0;
				dyPacGirl =0;

				if (winner()) {
					// game end sound - won
					XGM_startPlay(victory_vgm);
				} else {
					// Pac-Man was eaten
					map[pacmanY][pacmanX] = RED;
					// remove Pac-Man sprite from screen (we were eaten)
					SPR_setPosition(pacmanSprite, -90, 90);
					// game end sound - lost
					XGM_startPlay(fatality_vgm);
				}

				// calculate earned points
				calcScore();

				// change game state to show results (game over)
				gameState = STATE_RESULT;
			break;
			case OBJECT_TYPE_PAUSE:
				// master console notified that need to enter pause mode
				pause();
			break;
			case OBJECT_TYPE_RESUME_GAME:
				// master console notified that need to continue game
				resumeGame();
			break;
			case OBJECT_TYPE_EAT_POINT:
				// master console notified that food was eaten
				incFood();
			break;
			case OBJECT_TYPE_EAT_POWERUP:
				// master console notified that powerup was eaten
				// powerup eating sound
				XGM_startPlayPCM(SFX_SOUND_POWERUP, 15, SOUND_PCM_CH2);

				// RED became edible
				redTime = RED_TIME;

				// and give additional bonus
				++powerBonus;
			break;
			case OBJECT_TYPE_EAT_SHADOW:
				// master console notified that ghost was eaten
				// ghost eating sound
				XGM_startPlayPCM(SFX_SOUND_EAT_SHADOW, 15, SOUND_PCM_CH2);

				// close door to ghost house
				closeDoors();

		    	// hide cherry
		    	SPR_setPosition(cherrySprite, -90, 100);

		       	// let it stay in house additional time
		        redTime = RED_TIME;

				// give bonus for eating RED
				++redBonus;
			break;
			case OBJECT_TYPE_EAT_CHERRY:
				// master console notified that cherry was eaten
				// cherry eating sound
				XGM_startPlayPCM(SFX_SOUND_CHERRY, 15, SOUND_PCM_CH2);

				// hide cherry
				SPR_setPosition(cherrySprite, -90, 100);

				// give bonus for cherry
				++cherryBonus;
			break;
			case OBJECT_TYPE_OPEN_DOOR:
				// master console notified that need to open door to ghost house
				openDoors();
			break;
		}
	} while (objectType != 0);
}


/**
 * Determine what was pressed by players on controllers. In case of playing via Link Cable we get what
 * was pressed on the first controller connected to the other console.
 * Analysis of objects in incoming packets via Link Cable also happens here for both master and
 * slave console.
 *
 * pad1 - player controlling Pac-Man. What is pressed on 1st controller when playing on one console.
 *        When playing via Link Cable by default in pad1 will be what is pressed on master console
 *        on 1st controller, also playing as Pac-Man.
 *
 * pad2 - player controlling Pac-Girl. What is pressed on 2nd controller when playing on one console.
 *        When playing via Link Cable by default in pad2 will be what is pressed on slave console
 *        on 1st controller, also playing as Pac-Girl
 *
 * BUT if in '2 PLAYERS' menu RIGHT is pressed then pad1 and pad2 will be swapped! I.e. when
 * switchPlayers == 1 and players == 2 players will play as opposite characters!
 *
 */
void controls() {
	// buffer variable used in case if players changed who plays which character in '2 PLAYERS' menu
	// needed to swap pad1 and pad2
	u16 switchPad;

	// data transmission error
	u16 lcpError = 0;

	// what is pressed on 1st joystick
	pad1 = JOY_readJoypad(JOY_1);

	// determine the mode in which the console operates
	switch (controllerPort2Mode) {
		case MODE_MULTI_PLAYER:
			// multiplayer no connection between consoles via Link Cable but in second port 3 or 6
			// button controller. Second player by default plays as PAC-GIRL with this controller
			// BUT if in '2 PLAYERS' menu RIGHT was pressed before game (switchPlayers == 1), then as Pac-Man
			// because at the end of this function pad1 and pad2 will be swapped in this case

			// what is pressed on 2nd controller in pad2
			pad2 = JOY_readJoypad(JOY_2);
		break;
		case MODE_PORT2_MASTER:
			// master - our console is master, playing together via Link Cable (network game on two SEGA consoles)
			// we play as first player PAC-MAN but if in '2 PLAYERS' menu characters were changed then as PAC-Girl

			masterControls();
		break;
		case MODE_PORT2_SLAVE:
			// slave - our console is slave, playing together via Link Cable (network game on two SEGA consoles)
			// we play as second player PAC-GIRL but if in '2 PLAYERS' menu characters were changed then as PAC-MAN

			slaveControls();
		break;
		case MODE_SINGLE_PLAYER:
			// singleplayer - no connection via Link Cable and no controller in port 2
		  	// main and only player is the one pressing buttons on first controller, therefore
			// we do nothing we already put everything in pad1
		case MODE_PORT2_UNKNOWN:
			// don't know what with second controller,
			// do nothing
		default:
			// no need to process events from 2nd controller port
			// do nothing
		break;
	}

	// display on screen only real errors that occurred during transmission
	// via Link Cable
	lcpError = LCP_getError();
	if (lcpError != 0 && lcpError != 0x1000) {
		linkCableErrors = lcpError;
		linkCableErrorsCount++;
	}

	if (P1_PACGIRL__P2_PACMAN == switchPlayers && players == 2) {
		// switchPlayers == P1_PACGIRL__P2_PACMAN - first player plays Pac-Girl not Pac-Man, when playing together on one console
	    // if playing via Link Cable - master plays as Pac-Girl not as Pac-Man as initially
		// and second player plays as Pac-Man not as Pac-Girl, when playing together on one console
		// and if playing via Link Cable - slave plays as Pac-Man not as Pac-Girl
		// for this simply swap values of pad1 and pad2
		switchPad = pad1;
		pad1 = pad2;
		pad2 = switchPad;
	}
}


/**
 * Determine if you won the game
 *
 * return true - won the game
 */
u8 winner() {
	return food100 == 2 && food010 == 7 && food001 == 1 && powerBonus == 4;
}


/**
 * Sound effects initialization
 */
void initSound() {
	// voice during screensaver pronouncing word SEGA
	XGM_setPCM(SFX_SOUND_SEGA, sega_sfx, sizeof(sega_sfx));
	// sound of eating white dot - food
	XGM_setPCM(SFX_SOUND_EAT, eat_sfx, sizeof(eat_sfx));
	// sound of eating cherry
	XGM_setPCM(SFX_SOUND_CHERRY, cherry_sfx, sizeof(cherry_sfx));
	// sound of eating green dot - powerup
	XGM_setPCM(SFX_SOUND_POWERUP, powerup_sfx, sizeof(powerup_sfx));
	// sound when ghost is edible
	XGM_setPCM(SFX_SOUND_SHADOW, shadow_sfx, sizeof(shadow_sfx));
	// sound when ghost was eaten
	XGM_setPCM(SFX_SOUND_EAT_SHADOW, eatred_sfx, sizeof(eatred_sfx));
	// sound of creating connection via Link Cable
	XGM_setPCM(SFX_SOUND_CONNECT_LINK_CABLE, connect_sfx, sizeof(connect_sfx));
	// sound of disconnecting connection via Link Cable
	XGM_setPCM(SFX_SOUND_DISCONNECT_LINK_CABLE, disconnect_sfx, sizeof(disconnect_sfx));
}


/**
 * Palettes setup
 */
void  initPaletts() {
    // set colors in 4th palette (counting starts from zero), with colors taken from sonic sprite,
    // and selected DMA as transfer method.
    // Sega supports 4 palettes of 16 colors each (PAL0-PAL3), and stores them in CRAM.
    PAL_setPalette(PAL3, sonic_sprite.palette->data, DMA);
    PAL_setPalette(PAL2, red_sprite.palette->data, DMA);
    PAL_setPalette(PAL1, pacgirl_sprite.palette->data, DMA);
}


/**
 * Sprites initialization
 *
 */
void initSprites() {
    // add sonic sprite to screen
    sonicSprite = SPR_addSprite(&sonic_sprite, sonicX, sonicY, 
                                    TILE_ATTR(PAL3       // palette
                                                , 0      // sprite priority (sprite with smaller number will overlap sprite with larger number)
                                                , FALSE  // flip vertically
                                                , FALSE  // flip horizontally
                                              )
                                );

    // add Pac-Man sprite to screen
    pacmanSprite = SPR_addSprite(&pacman_sprite, pacmanX, pacmanY, 
                                    TILE_ATTR(PAL1       // palette
                                                , 1      // sprite priority (sprite with smaller number will overlap sprite with larger number)
                                                , FALSE  // flip vertically
                                                , FALSE  // flip horizontally
                                              )
                                );

    // add Red sprite to screen
    redSprite = SPR_addSprite(&red_sprite, redX, redY, 
                                    TILE_ATTR(PAL2       // palette
                                                , 0      // sprite priority (sprite with smaller number will overlap sprite with larger number)
                                                , FALSE  // flip vertically
                                                , FALSE  // flip horizontally
                                              )
                                );


    // add Pac-Girl sprite to screen
    pacGirlSprite = SPR_addSprite(&pacgirl_sprite, pacGirlX, pacGirlY, 
                                    TILE_ATTR(PAL1       // palette
                                                , 1      // sprite priority (sprite with smaller number will overlap sprite with larger number)
                                                , FALSE  // flip vertically
                                                , FALSE  // flip horizontally
                                              )
                                );


    // add cherry sprite to screen
    cherrySprite = SPR_addSprite(&cherry_sprite, cherryX, cherryY,
                                    TILE_ATTR(PAL1       // palette
                                                , 1      // sprite priority (sprite with smaller number will overlap sprite with larger number)
                                                , FALSE  // flip vertically
                                                , FALSE  // flip horizontally
                                              )
                                );
    // add door sprite to screen
    doorSprite = SPR_addSprite(&door_sprite, doorX, doorY,
									TILE_ATTR(PAL1       // palette
												, 1      // sprite priority (sprite with smaller number will overlap sprite with larger number)
												, FALSE  // flip vertically
												, FALSE  // flip horizontally
											  )
								);
    // add pause sprite to screen
    pauseSprite = SPR_addSprite(&pause_sprite, -100, -100,
									TILE_ATTR(PAL1       // palette
												, 1      // sprite priority (sprite with smaller number will overlap sprite with larger number)
												, FALSE  // flip vertically
												, FALSE  // flip horizontally
											  )
								);

}


/**
 * Initialization and reset variables to default values
 * here we also need to reset variables after soft reset (pressing RESET button)
 */
void initGame() {
    // initialize sprite engine (allocate space in VRAM for sprites)
    SPR_init();

    // palettes setup
    initPaletts();

    // sprites initialization
    initSprites();

	// sound effects initialization
	initSound();


    if (MODE_PORT2_MASTER == controllerPort2Mode || MODE_PORT2_SLAVE == controllerPort2Mode) {
    	// if there was soft reset (pressed RESET), variables are not reset, need to also
    	// reset SEGA Link Cable Protocol variables state
    	LCP_close();
    }

    // reset console operation mode with second port for controller
    controllerPort2Mode = MODE_PORT2_UNKNOWN;

    // don't display anything on screen about game mode
    memset(gameModeText, 0, GAME_MODE_TEXT_SIZE);

    // by default first player controls Pac-Man, second player Pac-Girl
    switchPlayers = P1_PACMAN__P2_PACGIRL;

    // by default single player game is selected
    players = 1;

    // screensaver (must be set in main because SEGA has soft reset)
    gameState = STATE_SCREENSAVER;

    // initialize character positions for screensaver
    initScreensaver();
}


// program entry point
int main() {

	// Initialization and reset variables to default values
	initGame();

	// Draw SEGA as background
	drawBackground();

	// SEGA sound at game start
	XGM_startPlayPCM(SFX_SOUND_SEGA, 15, SOUND_PCM_CH2);

	// game animation loop
	while (1) {

		// determine pressed buttons on controllers
		// data transfer via Link Cable from master console
		// to slave console and reaction to received objects via Link Cable
		controls();

		// draw bonuses, scores or game result
		drawText();

		// process player actions (controller button presses)
		// move characters depending on what is pressed on the map
		actions();

		// draw sprites according to positions on the map
		drawSprites();

		// Updates and displays sprites on screen
		SPR_update();

		// does all background processing, needed when there are sprites, music, joystick
		SYS_doVBlankProcess();

		// number of rendered frames
		++linkCableFrameCount;
	}

	return 0;
}

