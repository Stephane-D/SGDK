#include <genesis.h>

#include "../inc/levels.h"
#include "../inc/camera.h"
#include "../inc/player.h"

void inGameJoyEvent(u16 joy, u16 changed, u16 state);

int main(bool resetType) {
	//Soft resets don't clear RAM, this can bring some bugs so we hard reset every time we detect a soft reset
	if (!resetType)
		SYS_hardReset();

	//Initialize joypad and sprite engine in order to use them
	JOY_init();
	SPR_init();

	//Setup the basic things we need for this demo
	loadLevel();
	playerInit();
	setupCamera(newVector2D_u16(160, 112), 20, 20); //We have to setup always after the player, it directly depends on player's data

	//Setup a callback when a button is pressed, we could call it a "pseudo parallel" joypad handler
	JOY_setEventHandler(inGameJoyEvent);

	//Infinite loop where the game is run
	while (TRUE) {
		//First we update all the things that have to be updated each frame
		updatePlayer();
		updateCamera();

		//Then we update sprites and after that we tell the Mega Drive that we have finished processing all the frame data
		SPR_update();
		SYS_doVBlankProcess();
	}

	return 0;
}

//In order to make this data more accessible from all scripts we write them into a struct from global.h
void inGameJoyEvent(u16 joy, u16 changed, u16 state) {
	input.joy = joy;
	input.changed = changed;
	input.state = state;

	playerInputChanged();
}