#ifndef _SPR_TOOLS_H_
#define _SPR_TOOLS_H_

#include "tools.h"
#include "tile_tools.h"


#define SPRITE_SIZE(w, h)   ((((w) - 1) << 2) | ((h) - 1))

#define COLLISION_NONE      0
#define COLLISION_BOX       1
#define COLLISION_CIRCLE    2


typedef struct
{
    int type;
    union
    {
        box_ box;
        circle_ circle;
    } norm;
    union
    {
        box_ box;
        circle_ circle;
    } hflip;
    union
    {
        box_ box;
        circle_ circle;
    } vflip;
    union
    {
        box_ box;
        circle_ circle;
    } hvflip;
    void* inner;
    void* next;
} collision_;

typedef struct
{
    int x;
    int y;
    int w;
    int h;
    int numTile;
} frameSprite_ ;

typedef struct
{
	int numSprite;
	frameSprite_  **frameSprites;
	collision_ *collision;
	tileset_ *tileset;
    int w;
    int h;
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
    int maxNumTile;
    int maxNumSprite;
} spriteDefinition_;


//typedef struct
//{
//	tileset_ *tileset;
//    int ind;
//    int attr;
//    int x;
//	int y;
//    int w;
//    int h;
//}  frameSprite_;

//typedef struct
//{
//	int numSprite;
//	frameSprite_ **frameSprites;
//	int numCollision;
//	void **collisions;
//	int w;
//	int h;
//	int tc;
//	int timer;
//} animFrame_;

//typedef struct
//{
//	int numFrame;
//	animFrame_ **frames;
//	int length;
//	unsigned char *sequence;
//	int loop;
//} animation_;

//typedef struct
//{
//	int numAnimation;
//	animation_ **animations;
//  int maxNumTile;
//  int maxNumSprite;
//	int numTileset;
//	tileset_ **globalTilesets;
//} spriteDefinition_;


frameSprite_* getFlippedFrameSprite(frameSprite_* frameSprite, int wf, int hf, int hflip, int vflip);
frameSprite_* getFrameSprite(unsigned char *image8bpp, tileset_* tileset, int wi, int x, int y, int w, int h);
animFrame_* getAnimFrame(unsigned char *image8bpp, int wi, int fx, int fy, int wf, int hf, int time, int collisionType);
animation_* getAnimation(unsigned char *image8bpp, int wi, int anim, int wf, int hf, int time, int collisionType);
spriteDefinition_* getSpriteDefinition(unsigned char *image8bpp, int w, int h, int wf, int hf, int time, int collisionType);

int packSpriteDef(spriteDefinition_* spriteDef, int method);
void removeEmptyFrame(spriteDefinition_ *spriteDef);

int isEmptyTileData(unsigned int *tiles, int numTiles);
int isEmptyTileSet(tileset_ *tileset);
int isEmptyFrame(animFrame_ *animFrame);

//int getSpriteFrameFlip(frameSprite_* s1, frameSprite_* s2);
//int isSameFrameSprite(frameSprite_* s1, frameSprite_* s2);
//frameSprite_* findFrameSprite(frameSprite_* spr, spriteDefinition_* sprDef);


#endif // _SPR_TOOLS_H_
