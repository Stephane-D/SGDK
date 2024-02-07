#ifndef _PLAYER_H_
#define _PLAYER_H_


#define MAX_SPEED_MAX       FIX32(20L)
#define MAX_SPEED_MIN       FIX32(1L)
#define MAX_SPEED_DEFAULT   FIX32(8L)

#define JUMP_SPEED_MIN      FIX32(4L)
#define JUMP_SPEED_MAX      FIX32(22L)
#define JUMP_SPEED_DEFAULT  FIX32(7.8L)

#define GRAVITY_MIN         FIX32(0.15)
#define GRAVITY_MAX         FIX32(0.8)
#define GRAVITY_DEFAULT     FIX32(0.32)


// physic settings
extern fix32 maxSpeed;
extern fix32 jumpSpeed;
extern fix32 gravity;

// player position
extern fix32 posX;
extern fix32 posY;


u16 PLAYER_init(u16 vramIndex);

void PLAYER_update(void);
void PLAYER_updateScreenPosition(void);

void PLAYER_handleInput(u16 value);
void PLAYER_doJoyAction(u16 joy, u16 changed, u16 state);


#endif // _PLAYER_H_
