/**
 * This version is specifically created for SGDK as an example of a game using SEGA Link Cable!
 * You MUST set MODULE_LINK_CABLE to 1 in config.h to build this project!
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

/**
 * Constants
 */

// player count selection - screen after splash screen, i.e. after STATE_SCREENSAVER
#define STATE_SELECT  	  			0

// game - level map screen, after player count selection, i.e. after STATE_SELECT
#define STATE_GAME    	  			1

// game result - screen after game completion, i.e. after STATE_GAME
#define STATE_RESULT  	  			2

// splash screen - very first screen with SGDK eating animation
#define STATE_SCREENSAVER 			3

// pause - level map screen with Sonic running and PAUSE word flying around, when start is pressed on STATE_GAME
#define STATE_PAUSE 	  			4

// Food
#define FOOD 					    '.'

// Power-up allowing to eat ghosts
#define POWER_FOOD 					'*'

// Door to ghost room
#define DOOR						'-'

// Unoccupied cell
#define EMPTY 						' '

// Pac-Man
#define PACMAN 						'O'

// Pac Girl
#define PACGIRL 					'Q'

// Red ghost (BLINKY)
#define RED 						'^'

// Red ghost when it can be eaten (SHADOW or BLINKY)
#define SHADOW 						'@'

// Cherry
#define CHERRY 						'%'

// Pac-Man transition speed from one cell to another in rendering cycles
#define PACMAN_SPEED 				9

// Pac-Girl transition speed from one cell to another in rendering cycles
#define PACGIRL_SPEED 				9

// Red ghost transition speed from one cell to another in rendering cycles
#define RED_SPEED 					9

// Time after which cherry appears in rendering cycles
#define CHERRY_TIME 				255

// Time after which RED stops being edible in rendering cycles
#define RED_TIME 					255

// Map size by x
#define MAP_SIZE_Y 					23

// Map size by y
#define MAP_SIZE_X 					32

// Unknown if there is connection between consoles via SEGA Link Cable and if controller is plugged into port 2
#define MODE_PORT2_UNKNOWN			0

// Our console is master (first transmits data then reads data via SEGA Link cable)
#define MODE_PORT2_MASTER			1

// Our console is slave (first reads data then transmits data via SEGA Link cable)
#define MODE_PORT2_SLAVE			2

// 3 or 6-button controller in second port, no connection via SEGA Link cable
#define MODE_MULTI_PLAYER			3

// No connection between consoles and nothing connected in second port
#define MODE_SINGLE_PLAYER          4

// Voice during splash screen pronouncing word SEGA
#define SFX_SOUND_SEGA				64

// Sound of eating white dot - food
#define SFX_SOUND_EAT				66

// Sound of eating cherry
#define SFX_SOUND_CHERRY			67

// Sound of eating green dot - powerup
#define SFX_SOUND_POWERUP			68

// Sound when ghost is edible
#define SFX_SOUND_SHADOW			69

// Sound when ghost is eaten
#define SFX_SOUND_EAT_SHADOW		70

// Sound of establishing connection via Link cable
#define SFX_SOUND_CONNECT_LINK_CABLE	72

// Sound of disconnecting via Link cable
#define SFX_SOUND_DISCONNECT_LINK_CABLE	74

// Don't show any debug information
#define SHOW_LINK_CABLE_NO			0

// Display errors during transmission via Link Cable
#define SHOW_LINK_CABLE_LAST_ERROR	1

// Display error count during transmission via Link Cable
#define SHOW_LINK_CABLE_ERRORS_COUNT	2

// Number of rendered frames since connection establishment via Link Cable
#define SHOW_LINK_CABLE_FRAME_COUNT 3

// When playing on one console: first player - Pac-Man, second player - Pac-Girl.
// When playing on two consoles via Link cable, master - Pac-Man, slave - Pac-Girl.
#define P1_PACMAN__P2_PACGIRL 		0

// When playing on one console: first player - Pac-Girl, second player - Pac-Man.
// When playing on two consoles via Link cable, master - Pac-Girl, slave - Pac-Man.
#define P1_PACGIRL__P2_PACMAN		1

// Length of text displayed on screen about game type. Text can be:
// '1P NO LINK!'     - single console game, nothing detected in port 2; controllerPort2Mode = MODE_SINGLE_PLAYER
// '2P NO Link!'     - single console game on two controllers; controllerPort2Mode = MODE_MULTI_PLAYER
// 'TRY MASTER!'     - trying to become master console, assuming Link Cable in port 2
// 'TRY  SLAVE!'     - trying to become slave console, assuming Link Cable in port 2
// 'LINK MASTER'     - multiplayer game via Link Cable, we are master console; controllerPort2Mode = MODE_PORT2_MASTER
// 'LINK  SLAVE'     - multiplayer game via Link Cable, we are slave console; controllerPort2Mode = MODE_PORT2_SLAVE
// '           '     - unknown if consoles are connected and what's plugged into port 2; controllerPort2Mode = MODE_PORT2_UNKNOWN
#define GAME_MODE_TEXT_SIZE	 		21

// Constants below are for functions LCP_objectToPacketForSend() and LCP_getNextObjectFromReceivePacket()
// so protocol can identify which object was transmitted in transferObject array or received from another console and its size
// Protocol has no information about objects it transmits - it doesn't care what to transfer!

// object data type: master - keyword that master console sends to slave during connection initialization
#define OBJECT_TYPE_MASTER 			1

// object data type: slave - keyword that slave console sends to master during connection initialization
#define OBJECT_TYPE_SLAVE  			2

// object data type: joy - pressed buttons on controller in port 1 SEGA
#define OBJECT_TYPE_JOY    			3

// object data type: switch players - who plays which character
#define OBJECT_TYPE_SWITCH_PLAYERS 	4

// object data type: coordinates and movement direction of PAC-MAN
#define OBJECT_TYPE_PAC_MAN_STATE	5

// object data type: coordinates and movement direction of PAC-GIRL
#define OBJECT_TYPE_PAC_GIRL_STATE	6

// object data type: coordinates and movement direction of RED
#define OBJECT_TYPE_RED_STATE		7

// object data type: end of game
#define OBJECT_TYPE_END_GAME		8

// object data type: pause
#define OBJECT_TYPE_PAUSE 			9

// object data type: resume game
#define OBJECT_TYPE_RESUME_GAME		10

// object data type: ate a dot
#define OBJECT_TYPE_EAT_POINT		11

// object data type: ate a power-up
#define OBJECT_TYPE_EAT_POWERUP		12

// object data type: ate a ghost
#define OBJECT_TYPE_EAT_SHADOW		13

// object data type: ate a cherry
#define OBJECT_TYPE_EAT_CHERRY		14

// object data type: open door
#define OBJECT_TYPE_OPEN_DOOR		15


// length of 'master' data type - 8 bytes, text: 'Pac-Girl'
#define MASTER_OBJECT_LENGTH 		8

// length of 'slave' data type - 8 bytes, text: 'Pac-Man!'
#define SLAVE_OBJECT_LENGTH  		8

// length of 'joy' data type - 2 bytes (state of all pressed buttons on controller)
#define JOY_OBJECT_LENGTH 	 		2

// length of 'switch players' data type - 2 bytes
#define SWITCH_PLAYERS_LENGTH 		2

// length of 'coordinates and movement direction of PAC-MAN' data type - 4 bytes
#define PAC_MAN_STATE_LENGTH 		4

// length of 'coordinates and movement direction of PAC-GIRL' data type - 4 bytes
#define PAC_GIRL_STATE_LENGTH 		4

// length of 'coordinates and movement direction of RED' data type - 2 bytes
#define RED_STATE_LENGTH 			2

// length of 'end of game' data type - 2 bytes
#define END_GAME_LENGTH				2

// length of 'pause' data type - 2 bytes
#define PAUSE_LENGTH				2

// length of 'resume game' data type - 2 bytes
#define RESUME_GAME_LENGTH			2

// length of 'ate a dot' data type - 2 bytes
#define EAT_POINT_LENGTH			2

// length of 'ate a power-up' data type - 2 bytes
#define EAT_POWERUP_LENGTH			2

// length of 'ate a ghost' data type - 2 bytes
#define EAT_SHADOW_LENGTH			2

// length of 'ate a cherry' data type - 2 bytes
#define EAT_CHERRY_LENGTH			2

// length of 'open door' data type - 2 bytes
#define OPEN_DOOR_LENGTH			2


/**
 * Global variables
 */

/**
 *  0 - not an object
 *  1 - OBJECT_TYPE_MASTER         - MASTER_OBJECT_LENGTH  = 8 bytes
 *  2 - OBJECT_TYPE_SLAVE          - SLAVE_OBJECT_LENGTH   = 8 bytes
 *  3 - OBJECT_TYPE_JOY            - JOY_OBJECT_LENGTH     = 2 bytes
 *  4 - OBJECT_TYPE_SWITCH_PLAYERS - SWITCH_PLAYERS_LENGTH = 2 bytes
 *  5 - OBJECT_TYPE_PAC_MAN_STATE  - PAC_MAN_STATE_LENGTH  = 4 bytes
 *  6 - OBJECT_TYPE_PAC_GIRL_STATE - PAC_GIRL_STATE_LENGTH = 4 bytes
 *  7 - OBJECT_TYPE_RED_STATE      - RED_STATE_LENGTH      = 2 bytes
 *  8 - OBJECT_TYPE_END_GAME       - END_GAME_LENGTH       = 2 bytes
 *  9 - OBJECT_TYPE_PAUSE          - PAUSE_LENGTH          = 2 bytes
 * 10 - OBJECT_TYPE_RESUME_GAME    - RESUME_GAME_LENGTH    = 2 bytes
 * 11 - OBJECT_TYPE_EAT_POINT      - EAT_POINT_LENGTH      = 2 bytes
 * 12 - OBJECT_TYPE_EAT_POWERUP    - EAT_POWERUP_LENGTH    = 2 bytes
 * 13 - OBJECT_TYPE_EAT_SHADOW     - EAT_SHADOW_LENGTH     = 2 bytes
 * 14 - OBJECT_TYPE_EAT_CHERRY     - EAT_CHERRY_LENGTH     = 2 bytes
 * 15 - OBJECT_TYPE_OPEN_DOOR      - OPEN_DOOR_LENGTH      = 2 bytes
 *
 * Array containing sizes of objects transmitted via SEGA_LINK_CABLE.
 * Since the protocol transmits 2 bytes (16 bits) at a time, it makes no sense to create objects with sizes not multiple of 2 bytes.
 * Minimum 2 bytes will be transmitted anyway, but such objects can also be defined!
 * Used as the last parameter of LCP_objectToPacketForSend() and LCP_getNextObjectFromReceivePacket() functions.
 * The protocol doesn't know the sizes of your objects, so they need to be passed to it!
 */
u16 LINK_TYPES_LENGTH[] = {
							0, MASTER_OBJECT_LENGTH, SLAVE_OBJECT_LENGTH, JOY_OBJECT_LENGTH, SWITCH_PLAYERS_LENGTH, PAC_MAN_STATE_LENGTH,
							   PAC_GIRL_STATE_LENGTH, RED_STATE_LENGTH, END_GAME_LENGTH, PAUSE_LENGTH, RESUME_GAME_LENGTH,
							   EAT_POINT_LENGTH, EAT_POWERUP_LENGTH, EAT_SHADOW_LENGTH, EAT_CHERRY_LENGTH, OPEN_DOOR_LENGTH
						  };

// game level map
u8 map[MAP_SIZE_Y][MAP_SIZE_X] = {
 "7888888888888895788888888888889",
 "4.............654.............6",
 "4*i220.i22220.l8d.i22220.i220*6",
 "4..............Q..............6",
 "4.i220.fxj.i22mxn220.fxj.i220.6",
 "4......654....654....654......6",
 "1xxxxj.65s220.l8d.222e54.fxxxx3",
 "555554.654...........654.655555",
 "555554.654.fxxj-fxxj.654.655555",
 "88888d.l8d.678d l894.l8d.l88888",
 "...........64  %  64..^........",
 "xxxxxj.fxj.61xxxxx34.fxj.fxxxxx",
 "555554.654.l8888888d.654.655555",
 "555554.654...........654.655555",
 "78888d.l8d.i22mxn220.l8d.l88889",
 "4.............654.............6",
 "4.i2mj.i22220.l8d.i22220.fn20.6",
 "4*..64.........O.........64..*6",
 "s20.ld.fxj.i22mxn220.fxj.ld.i2e",
 "4......654....654....654......6",
 "4.i2222y8z220.l8d.i22y8z22220.6",
 "4.............................6",
 "1xxxxxxxxxxxxxxxxxxxxxxxxxxxxx3"
 };

// console operation mode with second controller port
// MODE_PORT2_UNKNOWN       0 - unknown if there is connection between consoles via SEGA Link Cable and if controller is plugged into second port
// MODE_PORT2_MASTER        1 - master: our console is master (first sends data, then reads data via SEGA Link cable)
// MODE_PORT2_SLAVE         2 - slave: our console is slave (first reads data, then sends data via SEGA Link cable)
// MODE_MULTI_PLAYER        3 - multiplayer: 3 or 6 button controller in second port, no connection via SEGA Link cable
// MODE_SINGLE_PLAYER       4 - singleplayer: no connection between consoles and nothing connected to second port
u8 controllerPort2Mode = MODE_PORT2_UNKNOWN;

// for processing button presses by first player on controller
u16 pad1 = 0;

// for processing button presses by second player on controller
u16 pad2 = 0;

// SHOW_LINK_CABLE_NO - show nothing
// SHOW_LINK_CABLE_LAST_ERROR - show errors occurring during data transmission via Link Cable when pressing A + C
// SHOW_LINK_CABLE_ERRORS_COUNT - show number of errors occurring during data transmission via Link Cable when pressing B + C
// SHOW_LINK_CABLE_FRAME_COUNT - show number of rendered frames since connection establishment when pressing A + X
u8 showLinkCableErrors = SHOW_LINK_CABLE_NO;

// error that occurred during data transmission via Link cable, displayed on screen when pressing A + C
u16 linkCableErrors = 0;

// number of errors during transmission via Link cable, displayed on screen when pressing B + C
u16 linkCableErrorsCount = 0;

// number of rendered frames since connection establishment, displayed on screen when pressing A + X
u16 linkCableFrameCount = 0;

// which player plays which character
// if switchPlayers == P1_PACMAN__P2_PACGIRL when playing via Link cable, master - Pac-Man, slave - Pac-Girl. When playing on single
// console first player - Pac-Man, second player - Pac-Girl.
// if switchPlayers == 1 when playing via Link cable, master - Pac-Girl, slave - Pac-Man. When playing on single
// console first player - Pac-Girl, second player - Pac-Man.
// To change who plays which character, need to select '2 PLAYRS' in player selection menu and press
// on any controller RIGHT (switchPlayers = 1) and LEFT (to set switchPlayers = 0)
u8 switchPlayers = P1_PACMAN__P2_PACGIRL;

// contains object as byte array that we plan to transmit via SEGA Link Cable
// to another console, and must be size >= largest size among object data types
u8 transferObject[MASTER_OBJECT_LENGTH + 1];

// text 'LINK MASTER' - game on two consoles via Link Cable, we are the master console (controllerPort2Mode = MODE_PORT2_MASTER)
const u8 TEXT_LINK_MASTER[GAME_MODE_TEXT_SIZE] = "LINK MASTER";

// text 'LINK  SLAVE' - game on two consoles via Link Cable, we are the slave console (controllerPort2Mode = MODE_PORT2_SLAVE)
const u8 TEXT_LINK_SLAVE[GAME_MODE_TEXT_SIZE] =  "LINK  SLAVE";

// text '1P NO LINK!' - game on single console, nothing detected in port 2 (controllerPort2Mode = MODE_SINGLE_PLAYER)
const u8 TEXT_1P_NO_LINK[GAME_MODE_TEXT_SIZE] =  "1P NO LINK!";

// text '2P NO Link!' - game on single console, on two controllers (controllerPort2Mode = MODE_MULTI_PLAYER)
const u8 TEXT_2P_NO_LINK[GAME_MODE_TEXT_SIZE] =  "2P NO Link!";

// text 'TRY MASTER!' - trying to become master console, assume that Link Cable is in port 2
const u8 TEXT_TRY_MASTER[GAME_MODE_TEXT_SIZE] =  "TRY MASTER!";

// text 'TRY  SLAVE!' - trying to become slave console, assume that Link Cable is in port 2
const u8 TEXT_TRY_SLAVE[GAME_MODE_TEXT_SIZE] =   "TRY  SLAVE!";

// for displaying console operation mode as text considering what is connected to port 2 of the console
// '1P NO LINK!' (TEXT_1P_NO_LINK)  game on single console, nothing detected in port 2 (controllerPort2Mode = MODE_SINGLE_PLAYER)
// '2P NO Link!' (TEXT_2P_NO_LINK)  game on single console on two controllers (controllerPort2Mode = MODE_MULTI_PLAYER)
// 'TRY MASTER!' (TEXT_TRY_MASTER)  trying to become master console, assume that Link Cable is in port 2
// 'TRY  SLAVE!' (TEXT_TRY_SLAVE)   trying to become slave console, assume that Link Cable is in port 2
// 'LINK MASTER' (TEXT_LINK_MASTER) game on two consoles via Link Cable, we are the master console (controllerPort2Mode = MODE_PORT2_MASTER)
// 'LINK  SLAVE' (TEXT_LINK_SLAVE)  game on two consoles via Link Cable, we are the slave console (controllerPort2Mode = MODE_PORT2_SLAVE)
// '           ' unknown if there is connection between consoles and what is connected to port 2 (controllerPort2Mode = MODE_PORT2_UNKNOWN)
char gameModeText[GAME_MODE_TEXT_SIZE + 1];

// type of object transmitted via Link Cable Protocol
u16 objectType = 0;

// variable for rendering text information
// score, bonuses, GAME OVER, YOU WINNER
char text[4];

// game state (which screen we are on)
// 0 - STATE_SELECT - player count selection
// 1 - STATE_GAME - game, second screen after player count selection
// 2 - STATE_GAME_RESULT - game result, after game ends
// 3 - STATE_SCREENSAVER - screensaver, very first screen with SGDK eating
// 4 - STATE_PAUSE - pause during game
u8 gameState = STATE_SCREENSAVER;


// current PACMAN coordinates
s16 pacmanX = 15;
s16 pacmanY = 17;

// current PACGIRL coordinates
s16 pacGirlX = 15;
s16 pacGirlY = 3;

// previous PACMAN coordinates
s16 oldX = 15;
s16 oldY = 17;

// PACMAN movement direction
s8 dx = 0;
s8 dy = 0;

// PACGIRL movement direction
s8 dxPacGirl = 0;
s8 dyPacGirl = 0;

// previous PACGIRL coordinates
s16 oldPacGirlX = 15;
s16 oldPacGirlY = 3;

// RED (SHADOW or BLINKY) movement direction
s8 dxRed = 1;
s8 dyRed = 0;

// RED (SHADOW or BLINKY) coordinates
s16 redX = 22;
s16 redY = 10;

// previous RED (SHADOW or BLINKY) coordinates
s16 oldXRed = 22;
s16 oldYRed = 10;

// 1 - RED in hunting mode
// 0 - PACMAN ate POWER_FOOD, and RED is currently edible
u8 redFlag = 1;

// time when RED last became edible
u8 redTime = 0;

// 1 - Cherry exists
// 0 - No cherry
u8 cherryFlag = 0;

// need to redraw cherry
u8 refreshCherry = 0;

// cherry x coordinate
s16 cherryX = 15;
// cherry y coordinate
s16 cherryY = 10;

// door x coordinate
s16 doorX = 15;
// door y coordinate
s16 doorY = 8;

// PAUSE text sprite coordinates
s16 pauseX = 120;
s16 pauseY = 120;

// PAUSE text sprite movement direction
s16 pauseDX = 2;
s16 pauseDY = 1;

// need to redraw door
u8 refreshDoor = 1;

// what is on RED's (BLINKY) cell
u8 oldRedVal = '.';

// what is on PACGIRL's cell
u8 oldPacGirlVal = '.';

// bonus for eating RED (BLINKY)
u8 redBonus = 0;

// bonus for eating POWER_FOOD
u8 powerBonus = 0;

// bonus for eaten cherries
u8 cherryBonus = 0;

// cycle counters start with different values,
// so each character's rendering is in different global cycle
// time of last pacman position update
u8 pacmanLastUpdateTime = PACMAN_SPEED;

// time of last RED update
u8 redLastUpdateTime = 4; //RED_SPEED;

// time of last pacGirl position update
u8 pacGirlLastUpdateTime = 6; //PACGIRL_SPEED;

// time after which cherry will appear
u8 cherryTime = CHERRY_TIME;

// whether ghost music is playing
u8 shadowLastSoundTime = 0;

// variable through which we write and read values from map
// via setValToMap getValFromMap functions
u8 val;

// x, y coordinates tied to top-left corner (in pixels)
// used to determine where to draw objects
s16 y = 0;
s16 x = 0;

// number of selected players for game
u8 players = 1;

// cycle counter for protection against 2nd press on start screen
// Select button
u8 playersTime = 0;

// for displaying how many white dots were eaten
// units for eaten food
u8 food001 = 1;

// tens for eaten food
u8 food010 = 0;

// hundreds for eaten food
u8 food100 = 0;

// for displaying game result value from 000 to 999
// units for final score
u8 score001 = 0;

// tens for final score
u8 score010 = 0;

// hundreds for final score
u8 score100 = 0;

// value how many times ghost was eaten (used by 2nd player client)
// (used by 2nd player client in network interaction)
u8 redBonusVal = 0;

// door value (used by 2nd player client)
// (used by 2nd player client in network interaction)
u8 doorVal = 0;

// value for cherry
// (used by 2nd player client in network interaction)
u8 cherryVal = 0;

// main console game mode
// (used by 2nd player client in network interaction)
u8 gameStateMaster = 1;

// Sonic sprite
Sprite* sonicSprite;

// Pac-Man sprite
Sprite* pacmanSprite;

// RED sprite
Sprite* redSprite;

// Pac-Girl sprite
Sprite* pacGirlSprite;

// cherry sprite
Sprite* cherrySprite;

// door sprite
Sprite* doorSprite;

// sprite with 'PAUSE' text
Sprite* pauseSprite;

// coordinates where we draw Sonic sprite
s16 sonicX = -95;
s16 sonicY = 83;

// Sonic movement speed
s16 dxSonic = 1;


/**
 * Functions
 */

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
void masterToTransferObject();

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
void slaveToTransferObject();

/**
 * Save dx, dy, pacmanX, pacmanY, oldX, oldY in byte array transferObject
 * for transmission to another console as OBJECT_TYPE_PAC_MAN_STATE object
 */
void pacManStateToTransferObject();

/**
 * Get dx, dy, pacmanX, pacmanY, oldX, oldY from byte array transferObject
 * in case we received OBJECT_TYPE_PAC_MAN_STATE object from another console
 */
void pacManStateFromTransferObject();

/**
 * Save dxPacGirl, dyPacGirl, pacGirlX, pacGirlY, oldPacGirlX, oldPacGirlY in byte array transferObject
 * for transmission to another console as OBJECT_TYPE_PAC_GIRL_STATE object
 */
void pacGirlStateToTransferObject();

/**
 * Get dxPacGirl, dyPacGirl, pacGirlX, pacGirlY from byte array transferObject
 * in case we received OBJECT_TYPE_PAC_GIRL_STATE object from another console
 */
void pacGirlStateFromTransferObject();

/**
 * Save dxRed, dyRed, redX, redY, redFlag in byte array transferObject
 * for transmission to another console as OBJECT_TYPE_RED_STATE object
 */
void redStateToTransferObject();

/**
 * Get dxRed, dyRed, redX, redY, redFlag from byte array transferObject
 * in case we received OBJECT_TYPE_RED_STATE object from another console
 */
void redStateFromTransferObject();

/**
 * In transferObject save object containing information about what was pressed on the first controller
 * of our console for transmission to another console as OBJECT_TYPE_JOY object
 *
 * pad - information about what was pressed on the controller
 */
void padToTransferObject(u16 pad);

/**
 * From object stored in transferObject transmitted via Link Cable get information about what was pressed
 * on the first controller of another console in case we received OBJECT_TYPE_JOY object from another console
 *
 * return information about what was pressed on the controller
 */
u16 getPadFromTransferObject();

/**
 * Determine console operation mode when trying to play together
 * check what is connected to controller port 2.
 * If playing via SEGA Link Cable determine which
 * console is master and which is slave
 */
void initControllerPort2();

/**
 * Score calculation taking into account all bonuses
 */
void calcScore();

/**
 * Cell at given coordinates is not a wall (WALL)
 * i - row in map array
 * j - column in map array
 * return val = 1 - not a wall, 0 - wall
 */
u8 isNotWall(s16 y, s16 x);

/**
 * Cell at given coordinates is not a wall and not a door (WALL, DOOR)
 * y - Y coordinate on map (map[][])
 * x - X coordinate on map (map[][])
 * return val = 1 - not a wall and not a door, 0 - wall or door
 */
u8 isNotWallOrDoor(s16 y, s16 x);

/**
 * Coordinate correction for PAC-MAN, PAC-GIRL or Ghost
 * if went beyond the field (appearing from the other side of the field)
 * x - X coordinate on map (map[][])
 * y - Y coordinate on map (map[][])
 * values are passed by reference, so they change
 */
void moveBound(s16 *x, s16 *y);

/**
 * Open doors to cherry and ghost house
 */
void openDoors();

/**
 * Close doors to ghost house
 */
void closeDoors();

/**
 * Reset everything to initial map settings:
 * initial cycle counter values,
 * initial character positions,
 * where food and power-ups will be
 */
void resetGame();

/**
 * Food was eaten
 * recalculate counter values
 * food001, food010, food100
 */
void incFood();

/**
 * Whether PACMAN lost or could eat the ghost
 * and what was eaten at the ghost's location: PACMAN or PACGIRL
 */
u8 pacmanLooser();

/**
 * PAC-MAN movement processing algorithm on the map
 * return 0 - End of game
 *        1 - PACMAN is still alive
 */
u8 pacManState();

/**
 * PAC-GIRL movement processing algorithm on the map
 * return 0 - End of game
 *        1 - PACMAN is still alive
 */
u8 pacGirlState();

/**
 * Ghost algorithm chasing PAC-MAN
 * return 0 - End of game
 *        1 - PACMAN is still alive
 */
u8 redState();

/**
 * Redraw background tile when a dot was eaten
 * draw black square 8x8
 */
void drawBlackBox(s16 y, s16 x);

/**
 * For debugging, put number value as characters in text buffer
 * 4 char type characters, number is displayed in hexadecimal format
 * for example, decimal 65535 will be displayed as FFFF
 *
 * val - number that we want to display on screen
 */
void printU16(u16 val);

/**
 * Draw bonuses, score,
 * game result (GAME OVER or YOU WINNER)
 */
void drawText();

/**
 * Draw sprites
 */
void drawSprites();

/**
 * Draw only 1 object from the map
 * i - row in map array
 * j - column in map array
 */
void draw(s16 i, s16 j);

/**
 * Draw the passed object at coordinates
 * i - row in map array
 * j - column in map array
 * val - what to draw at coordinates
 */
void drawSprite(s16 i, s16 j, u8 val);

/**
 * Update map, characters, doors, cherry on screen
 */
void refreshGame();

/**
 * Draw background
 */
void drawBackground();

/**
 * Processing of buttons pressed by players during screensaver when eating SGDK text
 * for gameState == STATE_SCREENSAVER
 */
void actionsStateScreensaver();

/**
 * Processing of buttons pressed by players and game logic during display of player count selection menu screen
 * for gameState == STATE_SELECT
 */
void actionsStateSelectPlayers();

/**
 * Processing of buttons pressed by players and game logic during pause
 * for gameState == STATE_PAUSE
 */
void actionsStatePause();

/**
 * Processing of buttons pressed by players and game logic during actual gameplay
 * for gameState == STATE_GAME
 */
void actionsStateGame();

/**
 * Processing of buttons pressed by players and game logic when showing result screen after game ends
 * gameState == STATE_RESULT
 */
void actionsStateResult();

/**
 * Processing of buttons pressed by player and main game logic based on player actions
 */
void actions();

/**
 * Reset variable values for screensaver display
 * and draw them in new positions
 */
void initScreensaver();

/**
 * Display screensaver:
 * Sonic running away from Pac-Man and SGDK text that will be eaten by Pac-Man and Pac-Girl,
 * then Pac-Man running away from ghost
 */
void screensaver();


/**
 * Switch game to pause mode
 */
void pause();

/**
 * Exit pause, i.e. change state to resume game
 */
void resumeGame();

/**
 * Playing on two consoles and our master console
 * sends to slave console what was pressed on our controller.
 * In case of character switching when selecting number of players, send which player plays which character,
 * and also send all packet objects that haven't been sent yet.
 * Then parse what was received from slave console, specifically what was pressed on the controller
 * of the other console.
 */
void masterControls();

/**
 * Playing on two consoles and our slave console
 * save in packet for sending to master console what was pressed on our controller.
 * Packet will be sent asynchronously when external interrupt occurs initiated by master console
 * at random time for our console (external interrupt handler - LCP_slaveCycle() function from link_cable.c library)
 * Parse what was received from master console during previous external interrupt call (LCP_slaveCycle())
 */
void slaveControls();

/**
 * Determine what was pressed by players on controllers. When playing via Link Cable, get what
 * was pressed on the first controller connected to the other console.
 * Analysis of objects in packets received via Link Cable also happens here for both master
 * and slave console.
 *
 * pad1 - player controlling Pac-Man. What is pressed on 1st controller when playing on single console.
 *        When playing via Link Cable, by default pad1 will contain what was pressed on master console
 *        on 1st controller, also playing as Pac-Man.
 *
 * pad2 - player controlling Pac-Girl. What is pressed on 2nd controller when playing on single console.
 *        When playing via Link Cable, by default pad2 will contain what was pressed on slave console
 *        on 1st controller, also playing as Pac-Girl.
 *
 * BUT if in '2 PLAYERS' menu you press RIGHT, then pad1 and pad2 will swap! I.e. when
 * switchPlayers == 1 and players == 2, players will play as opposite characters!
 *
 */
void controls();

/**
 * Determine if you won the game
 * return true - won the game
 */
u8 winner();

/**
 * Sound effects initialization
 */
void initSound();

/**
 * Palette setup
 */
void initPaletts();

/**
 * Sprites initialization
 */
void initSprites();

/**
 * Initialization and reset of variables to default values.
 * Here we also need to reset variables after soft reset (pressing RESET button).
 */
void initGame();
