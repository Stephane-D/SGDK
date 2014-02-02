#include <stdio.h>
#include <string.h>

#include "../inc/rescomp.h"
#include "../inc/plugin.h"
#include "../inc/tools.h"
#include "../inc/img_tools.h"
#include "../inc/spr_tools.h"

#include "../inc/sprite.h"
#include "../inc/palette.h"
#include "../inc/tileset.h"


// forward
static int isSupported(char *type);
static int execute(char *info, FILE *fs, FILE *fh);

// SPRITE resource support
Plugin sprite = { isSupported, execute };


static int isSupported(char *type)
{
    if (!stricmp(type, "SPRITE")) return 1;

    return 0;
}

static int execute(char *info, FILE *fs, FILE *fh)
{
    char temp[MAX_PATH_LEN];
    char id[50];
    char fileIn[MAX_PATH_LEN];
    int w, h, bpp;
    int wf, hf;
    int wt, ht;
    int size, psize;
    int packed, time;
    int maxIndex;
    char collision[32];
    int collid;
    int nbElem;
    unsigned char *data;
    unsigned short *palette;
    spriteDefinition_ *sprDef;

    packed = 0;
    time = 0;
    strcpy(collision, "NONE");

    nbElem = sscanf(info, "%s %s \"%[^\"]\" %d %d %d %d %s", temp, id, temp, &wf, &hf, &packed, &time, collision);

    if (nbElem < 5)
    {
        printf("Wrong SPRITE definition\n");
        printf("SPRITE name file width heigth [packed [time [collision]]]\n");
        printf("  name\t\tSprite variable name\n");
        printf("  file\tthe image file to convert to SpriteDefinition structure (should be a 8bpp .bmp or .png)\n");
        printf("  width\twidth of a single sprite frame in tile\n");
        printf("  height\theigth of a single sprite frame in tile\n");
        printf("  packed\tset to 1 to pack the Sprite data (0 by default).\n");
        printf("  time\tdisplay frame time in 1/60 of second (time between each animation frame).\n");
        printf("  collision\tcollision type: CIRCLE, BOX or NONE (BOX by default).\n");

        return FALSE;
    }

    if (!stricmp(collision, "CIRCLE")) collid = COLLISION_CIRCLE;
    else if (!stricmp(collision, "BOX")) collid = COLLISION_BOX;
    else collid = COLLISION_NONE;

    // adjust input file path
    adjustPath(resDir, temp, fileIn);

    // retrieve basic infos about the image
    if (!Img_getInfos(fileIn, &w, &h, &bpp)) return FALSE;

    // get size in tile
    wt = (w + 7) / 8;
    ht = (h + 7) / 8;

    // inform about incorrect size
    if ((w & 7) != 0)
    {
        printf("Warning: Image %s width is not a multiple of 8 (%d)\n", fileIn, w);
        printf("Width changed to %d\n", wt * 8);
    }
    if ((h & 7) != 0)
    {
        printf("Warning: Image %s height is not a multiple of 8 (%d)\n", fileIn, h);
        printf("Height changed to %d\n", ht * 8);
    }

    // get image data (always 8bpp)
    data = Img_getData(fileIn, &size, 8, 8);
    if (!data) return FALSE;

    // find max color index
    maxIndex = getMaxIndex(data, size);
    // not allowed here
    if (maxIndex >= 64)
    {
        printf("Error: Image %s use color index >= 64\n", fileIn);
        printf("IMAGE resource require image with a maximum of 64 colors.\n");
        return FALSE;
    }

    sprDef = getSpriteDefinition(data, wt, ht, wf, hf, time, collid);

    //TODO: optimize
    removeEmptyFrame(sprDef);

    // pack data
    if (packed != PACK_NONE)
    {
        if (!packSpriteDef(sprDef, &packed)) return FALSE;
    }

    // get palette
    palette = Img_getPalette(fileIn, &psize);
    if (!palette) return FALSE;

    // optimize palette size
    if (maxIndex < 16) psize = 16;
    else if (maxIndex < 32) psize = 32;
    else if (maxIndex < 48) psize = 48;
    else psize = 64;

    // EXPORT PALETTE
    strcpy(temp, id);
    strcat(temp, "_palette");
    outPalette(palette, 0, psize, fs, fh, temp, FALSE);

    // EXPORT SPRITE
    outSpriteDef(sprDef, fs, fh, id, TRUE);

    return TRUE;
}


void outBox(box_* box, FILE* fs, FILE* fh, char* id, int global)
{
    // Box structure
    decl(fs, fh, "Box", id, 2, global);
    fprintf(fs, "    dc.x    %d\n", box->x);
    fprintf(fs, "    dc.y    %d\n", box->y);
    fprintf(fs, "    dc.w    %d\n", box->w);
    fprintf(fs, "    dc.h    %d\n", box->h);
    fprintf(fs, "\n");
}

void outCircle(circle_* circle, FILE* fs, FILE* fh, char* id, int global)
{
    // Box structure
    decl(fs, fh, "Circle", id, 2, global);
    fprintf(fs, "    dc.x    %d\n", circle->x);
    fprintf(fs, "    dc.y    %d\n", circle->y);
    fprintf(fs, "    dc.ray  %d\n", circle->ray);
    fprintf(fs, "\n");
}

void outFrameSprite(frameSprite_* frameSprite, FILE* fs, FILE* fh, char* id, int global)
{
    char temp[MAX_PATH_LEN];

    // EXPORT TILESET
    strcpy(temp, id);
    strcat(temp, "_tileset");
    outTileset(frameSprite->tileset, fs, fh, temp, FALSE);

    // FrameSprite structure
    decl(fs, fh, "FrameSprite", id, 2, global);
    // Y position
    fprintf(fs, "    dc.w    %d\n", frameSprite->y);
    // size infos
    fprintf(fs, "    dc.w    %d\n", SPRITE_SIZE(frameSprite->w, frameSprite->h) << 8);
    // index and attributs
    fprintf(fs, "    dc.w    %d\n", frameSprite->ind | (frameSprite->attr << 11));
    // X position
    fprintf(fs, "    dc.w    %d\n", frameSprite->x);
    // set tileset pointer
    fprintf(fs, "    dc.l    %s\n", temp);
    fprintf(fs, "\n");
}

void outAnimFrame(animFrame_* animFrame, FILE* fs, FILE* fh, char* id, int global)
{
    int i;
    frameSprite_ **frameSprites;
    box_** boxes;
    circle_** circles;
    char temp[MAX_PATH_LEN];

    frameSprites = animFrame->frameSprites;
    for(i = 0; i < animFrame->numSprite; i++)
    {
        sprintf(temp, "%s_sprite%d", id, i);
        outFrameSprite(*frameSprites++, fs, fh, temp, FALSE);
    }

    fprintf(fs, "\n");

    // sprites data
    strcpy(temp, id);
    strcat(temp, "_sprites");
    // declare
    decl(fs, fh, NULL, temp, 2, FALSE);
    // output sprite pointers
    for(i = 0; i < animFrame->numSprite; i++)
        fprintf(fs, "    dc.l    %s_sprite%d\n", id, i);

    fprintf(fs, "\n");

    // collision data
    if (animFrame->tc != COLLISION_NONE)
    {
        switch (animFrame->tc)
        {
            case COLLISION_BOX:
                boxes = (box_**) animFrame->collisions;
                for(i = 0; i < animFrame->numCollision; i++)
                {
                    sprintf(temp, "%s_collision%d", id, i);
                    outBox(*boxes++, fs, fh, id, FALSE);
                }
                break;

            case COLLISION_CIRCLE:
                circles = (circle_**) animFrame->collisions;
                for(i = 0; i < animFrame->numCollision; i++)
                {
                    sprintf(temp, "%s_collision%d", id, i);
                    outCircle(*circles++, fs, fh, id, FALSE);
                }
                break;
        }

        fprintf(fs, "\n");

        // collisions data
        strcpy(temp, id);
        strcat(temp, "_collisions");
        // declare
        decl(fs, fh, NULL, temp, 2, FALSE);
        // output collision pointers
        for(i = 0; i < animFrame->numCollision; i++)
            fprintf(fs, "    dc.l    %s_collision%d\n", id, i);

        fprintf(fs, "\n");
    }

    // AnimationFrame structure
    decl(fs, fh, "AnimationFrame", id, 2, global);
    // set number of sprite
    fprintf(fs, "    dc.w    %d\n", animFrame->numSprite);
    // set frames pointer
    fprintf(fs, "    dc.l    %s_sprites\n", id);
    // set number of collision
    fprintf(fs, "    dc.w    %d\n", animFrame->numCollision);
    // set collisions pointer
    if (animFrame->tc != COLLISION_NONE)
        fprintf(fs, "    dc.l    %s_collisions\n", id);
    else
        fprintf(fs, "    dc.l    0\n");
    // frame width
    fprintf(fs, "    dc.w    %d\n", animFrame->w * 8);
    // frame height
    fprintf(fs, "    dc.w    %d\n", animFrame->h * 8);
    // collision type info
    fprintf(fs, "    dc.b    %d\n", animFrame->tc);
    // timer info
    fprintf(fs, "    dc.b    %d\n", animFrame->timer);
    fprintf(fs, "\n");
}

void outAnimation(animation_* animation, FILE* fs, FILE* fh, char* id, int global)
{
    int i;
    animFrame_ **frames;
    char temp[MAX_PATH_LEN];

    frames = animation->frames;
    for(i = 0; i < animation->numFrame; i++)
    {
        sprintf(temp, "%s_frame%d", id, i);
        outAnimFrame(*frames++, fs, fh, temp, FALSE);
    }

    fprintf(fs, "\n");

    // frames data
    strcpy(temp, id);
    strcat(temp, "_frames");
    // declare
    decl(fs, fh, NULL, temp, 2, FALSE);
    // output frame pointers
    for(i = 0; i < animation->numFrame; i++)
        fprintf(fs, "    dc.l    %s_frame%d\n", id, i);

    fprintf(fs, "\n");

    // sequence data
    strcpy(temp, id);
    strcat(temp, "_sequence");
    // declare
    decl(fs, fh, NULL, temp, 2, FALSE);
    // output sequence data
    outS(animation->sequence, 0, animation->length, fs, 1);

    fprintf(fs, "\n");

    // Animation structure
    decl(fs, fh, "Animation", id, 2, global);
    // set number of frame
    fprintf(fs, "    dc.w    %d\n", animation->numFrame);
    // set frames pointer
    fprintf(fs, "    dc.l    %s_frames\n", id);
    // set size of sequence
    fprintf(fs, "    dc.w    %d\n", animation->length);
    // set sequence pointer
    fprintf(fs, "    dc.l    %s_sequence\n", id);
    // loop info
    fprintf(fs, "    dc.w    %d\n", animation->loop);
    fprintf(fs, "\n");
}

void outSpriteDef(spriteDefinition_* spriteDef, FILE* fs, FILE* fh, char* id, int global)
{
    int i;
    animation_ **animations;
    char temp[MAX_PATH_LEN];

    animations = spriteDef->animations;
    for(i = 0; i < spriteDef->numAnimation; i++)
    {
        sprintf(temp, "%s_animation%d", id, i);
        outAnimation(*animations++, fs, fh, temp, FALSE);
    }

    fprintf(fs, "\n");

    // animations data
    strcpy(temp, id);
    strcat(temp, "_animations");
    // declare
    decl(fs, fh, NULL, temp, 2, FALSE);
    // output animation pointers
    for(i = 0; i < spriteDef->numAnimation; i++)
        fprintf(fs, "    dc.l    %s_animation%d\n", id, i);

    fprintf(fs, "\n");

    // SpriteDefinition structure
    decl(fs, fh, "SpriteDefinition", id, 2, global);
    // set palette pointer
    fprintf(fs, "    dc.l    %s_palette\n", id);
    // set number of animation
    fprintf(fs, "    dc.w    %d\n", spriteDef->numAnimation);
    // set animations pointer
    fprintf(fs, "    dc.l    %s_animations\n", id);
    fprintf(fs, "\n");
}

