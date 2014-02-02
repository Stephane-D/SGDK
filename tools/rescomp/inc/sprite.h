#ifndef _SPRITE_H_
#define _SPRITE_H_

#include "../inc/spr_tools.h"


extern Plugin sprite;

void outCollisionBox(box_* box, FILE* fs, FILE* fh, char* id, int global);
void outCircleBox(box_* box, FILE* fs, FILE* fh, char* id, int global);
void outFrameSprite(frameSprite_* frameSprite, FILE* fs, FILE* fh, char* id, int global);
void outAnimFrame(animFrame_* animFrame, FILE* fs, FILE* fh, char* id, int global);
void outAnimation(animation_* animation, FILE* fs, FILE* fh, char* id, int global);
void outSpriteDef(spriteDefinition_* spriteDef, FILE* fs, FILE* fh, char* id, int global);


#endif // _SPRITE_H_

