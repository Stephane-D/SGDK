#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <memory.h>

#include "../inc/spr_tools.h"
#include "../inc/tools.h"


frameSprite_* getFrameSprite(unsigned char *image8bpp, int wi, int x, int y, int w, int h)
{
    int i, j;
    int p, pal;
    int index;
    unsigned int tile[8];
    frameSprite_* result;
    tileset_* tileset;

    // get palette for this sprite
    pal = getTile(image8bpp, tile, 0, 0, wi * 8);
    // error retrieving tile --> return NULL
    if (pal == -1) return NULL;

    // allocate tileset
    tileset = createTileSet(malloc(w * h * 32), 0);

    for(i = 0; i < w; i++)
    {
        for(j = 0; j < h; j++)
        {
            p = getTile(image8bpp, tile, i + x, j + y, wi * 8);

            // error retrieving tile --> return NULL
            if (p == -1)
            {
                freeTileset(tileset);
                return NULL;
            }
            // different palette in VDP same sprite --> error
            if (p != pal)
            {
                printf("Error: Sprite at position (%d,%d) of size [%d,%d] reference different palette.", x, y, w, h);
                freeTileset(tileset);
                return NULL;
            }

            index = addTile(tile, tileset, FALSE);
            // error adding new tile --> return NULL
            if (index == -1)
            {
                freeTileset(tileset);
                return NULL;
            }
        }
    }

    // allocate result
    result = malloc(sizeof(frameSprite_));
    // always first index as we
    result->ind = 0;
    result->attr = TILE_ATTR(pal, FALSE, FALSE, FALSE);
    // initialized afterward
    result->x = 0;
	result->y = 0;
    result->w = w;
    result->h = h;
	result->tileset = tileset;

    return result;
}

animFrame_* getAnimFrame(unsigned char *image8bpp, int wi, int fx, int fy, int wf, int hf, int time, int collision)
{
    int i, j;
    int nbSprW, nbSprH;
    int lastSprW, lastSprH;
    int ws, hs;
    animFrame_* result;
    frameSprite_** frameSprites;
    frameSprite_* frameSprite;
    box_* box;
    circle_* circle;

    nbSprW = (wf + 3) / 4;
    nbSprH = (hf + 3) / 4;
    lastSprW = wf & 3;
    if (lastSprW == 0) lastSprW = 4;
    lastSprH = hf & 3;
    if (lastSprH == 0) lastSprH = 4;

    // allocate result
    result = malloc(sizeof(animFrame_));
    result->numSprite = nbSprW * nbSprH;
    // allocate frameSprite array
    frameSprites = malloc(result->numSprite * sizeof(frameSprite_*));
    result->frameSprites = frameSprites;
	result->w = wf;
	result->h = hf;
	result->tc = collision;
    result->timer = time;

    // handle collision structure
    switch(collision)
    {
        case COLLISION_NONE:
            result->numCollision = 0;
            result->collisions = NULL;
        break;

        case COLLISION_BOX:
            result->numCollision = 1;
            // allocate collision array
            result->collisions = malloc(1 * sizeof(void*));
            // allocate collision structure
            box = malloc(sizeof(box_));
            // use 75% the size of the frame for the collision
            box->x = (wf * 8) / 2;
            box->y = (hf * 8) / 2;
            box->w = ((wf * 8) * 3) / 4;
            box->h = ((hf * 8) * 3) / 4;
            result->collisions[0] = box;
        break;

        case COLLISION_CIRCLE:
            result->numCollision = 1;
            // allocate collision array
            result->collisions = malloc(1 * sizeof(void*));
            // allocate collision structure
            circle = malloc(sizeof(circle_));
            // use 75% the size of the frame for the collision
            circle->x = (wf * 8) / 2;
            circle->y = (hf * 8) / 2;
            circle->ray = ((wf * 8) * 3) / 4;
            result->collisions[0] = circle;
        break;
    }

    for(j = 0; j < nbSprH; j++)
    {
        if (j == (nbSprH - 1)) hs = lastSprH;
        else hs = 4;

        for(i = 0; i < nbSprW; i++)
        {
            if (i == (nbSprW - 1)) ws = lastSprW;
            else ws = 4;

            frameSprite = getFrameSprite(image8bpp, wi, (fx * wf) + (i * 4), (fy * hf) + (j * 4), ws, hs);
            if (frameSprite == NULL) return NULL;

            // set x and y offset
            frameSprite->x = i * 32;
            frameSprite->y = j * 32;
            *frameSprites++ = frameSprite;
        }
    }

    return result;
}

animation_* getAnimation(unsigned char *image8bpp, int wi, int anim, int wf, int hf, int time, int collision)
{
    int i;
    animation_* result;
    animFrame_** frames;
    unsigned char* sequence;
    int numFrame;

    // get max number of frame
    numFrame = wi / wf;

    // allocate result
    result = malloc(sizeof(animation_));
    result->numFrame = numFrame;
    // allocate animFrame array
    frames = malloc(numFrame * sizeof(animFrame_*));
    result->frames = frames;
    // default sequence
	result->length = numFrame;
    // allocate sequence
    sequence = malloc(numFrame * sizeof(unsigned char));
    result->sequence = sequence;
    // default: loop to frame 0
    result->loop = 0;

    for(i = 0; i < numFrame; i++)
    {
        *frames = getAnimFrame(image8bpp, wi, i, anim, wf, hf, time, collision);
        if (*frames == NULL) return NULL;
        frames++;
        *sequence++ = i;
    }

    return result;
}

spriteDefinition_* getSpriteDefinition(unsigned char *image8bpp, int w, int h, int wf, int hf, int time, int collision)
{
    int i;
    int numAnim;
    spriteDefinition_* result;
    animation_** animations;

    // get number of animation
    numAnim = h / hf;

    // allocate result
    result = malloc(sizeof(spriteDefinition_));
    result->numAnimation = numAnim;
    // allocate animation array
    animations = malloc(numAnim * sizeof(animation_*));
    result->animations = animations;

    for(i = 0; i < numAnim; i++)
    {
        *animations = getAnimation(image8bpp, w, i, wf, hf, time, collision);
        if (*animations == NULL) return NULL;
        animations++;
    }

    return result;
}


int packSpriteDef(spriteDefinition_ *spriteDef, int *method)
{
    int i, j, k;
    int result;
    frameSprite_ **frameSprites;
    animFrame_ **animFrames;
    animation_ **animations;
    frameSprite_ *frameSprite;
    animFrame_ *animFrame;
    animation_ *animation;

    result = TRUE;
    animations = spriteDef->animations;

    for(i = 0; i < spriteDef->numAnimation; i++)
    {
        animation = *animations++;
        animFrames = animation->frames;

        for(j = 0; j < animation->numFrame; j++)
        {
            animFrame = *animFrames++;
            frameSprites = animFrame->frameSprites;

            for(k = 0; k < animFrame->numSprite; k++)
            {
                frameSprite = *frameSprites++;
                if (!packTileSet(frameSprite->tileset, method)) result = FALSE;
            }
        }
    }

    return result;
}

void removeEmptyFrame(spriteDefinition_ *spriteDef)
{
    int i, j;
    animFrame_ **animFrames;
    animation_ **animations;
    animation_ *animation;

    animations = spriteDef->animations;

    for(i = 0; i < spriteDef->numAnimation; i++)
    {
        animation = *animations++;
        animFrames = animation->frames;

        j = animation->numFrame - 1;
        while((j >= 0) && isEmptyFrame(animFrames[j])) j--;

        // adjust number of frame
        animation->numFrame = j + 1;
        animation->length = j + 1;
    }
}

int isEmptyFrame(animFrame_ *animFrame)
{
    int i;
    frameSprite_ **frameSprites;

    frameSprites = animFrame->frameSprites;

    for(i = 0; i < animFrame->numSprite; i++)
        if (!isEmptyFrameSprite(*frameSprites++)) return FALSE;

    return TRUE;
}

int isEmptyFrameSprite(frameSprite_ *frameSprite)
{
    return isEmptyTileSet(frameSprite->tileset);
}

int isEmptyTileSet(tileset_ *tileset)
{
    int i;
    unsigned int *tile;

    i = tileset->num * 8;
    tile = tileset->tiles;
    while(i--)
    {
        if (*tile++) return FALSE;
    }

    return TRUE;
}
