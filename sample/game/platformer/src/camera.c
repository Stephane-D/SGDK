#include "camera.h"

#include "global.h"

#include "resources.h"

Vect2D_s16 cameraPosition;

AABB cameraDeadzone;

void setupCamera(Vect2D_u16 deadZoneCenter, u16 deadZoneWidth, u16 deadZoneHeight) {
	//Calculates the bounds of the deadZone
	cameraDeadzone.min.x = deadZoneCenter.x - (deadZoneWidth >> 1);
	cameraDeadzone.max.x = deadZoneCenter.x + (deadZoneWidth >> 1);
	cameraDeadzone.min.y = deadZoneCenter.y - (deadZoneHeight >> 1);
	cameraDeadzone.max.y = deadZoneCenter.y + (deadZoneHeight >> 1);

	updateCamera();

	//We force to update the whole layer in order to prevent wrong tile being loaded depending on the position
	MAP_scrollToEx(bga, cameraPosition.x, cameraPosition.y, TRUE);
}

void updateCamera() {
	//Update camera only if the center of the player is outside the deadZone of the camera
	if (playerBody.globalPosition.x + playerBody.centerOffset.x > cameraPosition.x + cameraDeadzone.max.x) {
		cameraPosition.x = playerBody.globalPosition.x + playerBody.centerOffset.x - cameraDeadzone.max.x;

	}else if (playerBody.globalPosition.x + playerBody.centerOffset.x < cameraPosition.x + cameraDeadzone.min.x) {
		cameraPosition.x = playerBody.globalPosition.x + playerBody.centerOffset.x - cameraDeadzone.min.x;
	}

	if (playerBody.globalPosition.y + playerBody.centerOffset.y > cameraPosition.y + cameraDeadzone.max.y) {
		cameraPosition.y = playerBody.globalPosition.y + playerBody.centerOffset.y - cameraDeadzone.max.y;

	}else if (playerBody.globalPosition.y + playerBody.centerOffset.y < cameraPosition.y + cameraDeadzone.min.y) {
		cameraPosition.y = playerBody.globalPosition.y + playerBody.centerOffset.y - cameraDeadzone.min.y;
	}

	//Clamp camera to the limits of the level
	cameraPosition.x = clamp(cameraPosition.x, 0, 448); // 768 - 320 = 448 (level width - screen width)
	cameraPosition.y = clamp(cameraPosition.y, 0, 544); // 768 - 224 = 544 (level height - screen height)

	//Finally we update the position of the camera
	MAP_scrollTo(bga, cameraPosition.x, cameraPosition.y);
}