#ifndef _SPR_TOOLS_H_
#define _SPR_TOOLS_H_

#include "tile_tools.h"


#define SPRITE_SIZE(w, h)   ((((w) - 1) << 2) | ((h) - 1))

#define COLLISION_NONE      0
#define COLLISION_BOX       1
#define COLLISION_CIRCLE    2


typedef struct
{
	tileset_ *tileset;
    int ind;
    int attr;
    int x;
	int y;
    int w;
    int h;
}  frameSprite_;

typedef struct
{
	int numSprite;
	frameSprite_ **frameSprites;
	int numCollision;
	void **collisions;
	int w;
	int h;
	int tc;
	int timer;
} animFrame_;

typedef struct
{
	int numFrame;
	animFrame_ **frames;
	int length;
	unsigned char *sequence;
	int loop;
} animation_;

typedef struct
{
	int numAnimation;
	animation_ **animations;
} spriteDefinition_;


frameSprite_* getFrameSprite(unsigned char *image8bpp, int wi, int x, int y, int w, int h);
animFrame_* getAnimFrame(unsigned char *image8bpp, int wi, int fx, int fy, int wf, int hf, int time, int collision);
animation_* getAnimation(unsigned char *image8bpp, int wi, int anim, int wf, int hf, int time, int collision);
spriteDefinition_* getSpriteDefinition(unsigned char *image8bpp, int w, int h, int wf, int hf, int time, int collision);

int packSpriteDef(spriteDefinition_* spriteDef, int *method);
void removeEmptyFrame(spriteDefinition_ *spriteDef);

int isEmptyFrame(animFrame_ *animFrame);
int isEmptyFrameSprite(frameSprite_ *frameSprite);
int isEmptyTileSet(tileset_ *tileset);

//int getSpriteFrameFlip(frameSprite_* s1, frameSprite_* s2);
//int isSameFrameSprite(frameSprite_* s1, frameSprite_* s2);
//frameSprite_* findFrameSprite(frameSprite_* spr, spriteDefinition_* sprDef);


#endif // _SPR_TOOLS_H_
