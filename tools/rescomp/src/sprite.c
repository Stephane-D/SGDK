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
    if (!strcasecmp(type, "SPRITE")) return 1;

    return 0;
}

static int execute(char *info, FILE *fs, FILE *fh)
{
    char temp[MAX_PATH_LEN];
    char id[50];
    char fileIn[MAX_PATH_LEN];
    char packedStr[256];
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

    nbElem = sscanf(info, "%s %s \"%[^\"]\" %d %d %s %d %s", temp, id, temp, &wf, &hf, packedStr, &time, collision);

    if (nbElem < 5)
    {
        printf("Wrong SPRITE definition\n");
        printf("SPRITE name \"file\" width heigth [packed [time [collid]]]\n");
        printf("  name      Sprite variable name\n");
        printf("  file      the image file to convert to SpriteDefinition structure (should be a 8bpp .bmp or .png)\n");
        printf("  width     width of a single sprite frame in tile\n");
        printf("  height    heigth of a single sprite frame in tile\n");
        printf("  packed    compression type, accepted values:\n");
        printf("              -1 / BEST / AUTO = use best compression\n");
        printf("               0 / NONE        = no compression\n");
        printf("               1 / APLIB       = aplib library (good compression ratio but slow)\n");
        printf("               2 / FAST / LZ4W = custom lz4 compression (average compression ratio but fast)\n");
        printf("  time      display frame time in 1/60 of second (time between each animation frame).\n");
        printf("  collid    collision type: CIRCLE, BOX or NONE (BOX by default).\n");

        return FALSE;
    }

    if (!strcasecmp(collision, "CIRCLE")) collid = COLLISION_CIRCLE;
    else if (!strcasecmp(collision, "BOX")) collid = COLLISION_BOX;
    else collid = COLLISION_NONE;

    // adjust input file path
    adjustPath(resDir, temp, fileIn);
    // get packed value
    packed = getCompression(packedStr);

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
    if (!sprDef) return FALSE;

    //TODO: optimize
    removeEmptyFrame(sprDef);

    // pack data
    if (packed != PACK_NONE)
    {
        if (!packSpriteDef(sprDef, packed)) return FALSE;
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


void outCollision(collision_* collision, FILE* fs, FILE* fh, char* id, int global)
{
    // Collision structure
    decl(fs, fh, "Collision", id, 2, global);
    // type position
    fprintf(fs, "    dc.w    %d\n", collision->type);

    switch(collision->type)
    {
        case COLLISION_BOX:
            // normal version
            fprintf(fs, "    dc.w    %d\n", collision->norm.box.x);
            fprintf(fs, "    dc.w    %d\n", collision->norm.box.y);
            fprintf(fs, "    dc.w    %d\n", collision->norm.box.w);
            fprintf(fs, "    dc.w    %d\n", collision->norm.box.h);
            // H flip version
            fprintf(fs, "    dc.w    %d\n", collision->hflip.box.x);
            fprintf(fs, "    dc.w    %d\n", collision->hflip.box.y);
            fprintf(fs, "    dc.w    %d\n", collision->hflip.box.w);
            fprintf(fs, "    dc.w    %d\n", collision->hflip.box.h);
            // V flip version
            fprintf(fs, "    dc.w    %d\n", collision->vflip.box.x);
            fprintf(fs, "    dc.w    %d\n", collision->vflip.box.y);
            fprintf(fs, "    dc.w    %d\n", collision->vflip.box.w);
            fprintf(fs, "    dc.w    %d\n", collision->vflip.box.h);
            // HV flip version
            fprintf(fs, "    dc.w    %d\n", collision->hvflip.box.x);
            fprintf(fs, "    dc.w    %d\n", collision->hvflip.box.y);
            fprintf(fs, "    dc.w    %d\n", collision->hvflip.box.w);
            fprintf(fs, "    dc.w    %d\n", collision->hvflip.box.h);
            break;

        case COLLISION_CIRCLE:
            // normal version
            fprintf(fs, "    dc.w    %d\n", collision->norm.circle.x);
            fprintf(fs, "    dc.w    %d\n", collision->norm.circle.y);
            fprintf(fs, "    dc.w    %d\n", collision->norm.circle.ray);
            fprintf(fs, "    dc.w    %d\n", 0);
            // H flip version
            fprintf(fs, "    dc.w    %d\n", collision->hflip.circle.x);
            fprintf(fs, "    dc.w    %d\n", collision->hflip.circle.y);
            fprintf(fs, "    dc.w    %d\n", collision->hflip.circle.ray);
            fprintf(fs, "    dc.w    %d\n", 0);
            // V flip version
            fprintf(fs, "    dc.w    %d\n", collision->vflip.circle.x);
            fprintf(fs, "    dc.w    %d\n", collision->vflip.circle.y);
            fprintf(fs, "    dc.w    %d\n", collision->vflip.circle.ray);
            fprintf(fs, "    dc.w    %d\n", 0);
            // HV flip version
            fprintf(fs, "    dc.w    %d\n", collision->hvflip.circle.x);
            fprintf(fs, "    dc.w    %d\n", collision->hvflip.circle.y);
            fprintf(fs, "    dc.w    %d\n", collision->hvflip.circle.ray);
            fprintf(fs, "    dc.w    %d\n", 0);
            break;
    }
    // inner pointer
    fprintf(fs, "    dc.l    %d\n", (int) collision->inner);
    // next pointer
    fprintf(fs, "    dc.l    %d\n", (int) collision->next);

    fprintf(fs, "\n");
}

//void outFrameSprite(frameSprite_* frameSprite, FILE* fs, FILE* fh, char* baseid, char* id, int global)
//{
//    char temp[MAX_PATH_LEN];
//    char refid[MAX_PATH_LEN];
//
//    // EXPORT TILESET
//    sprintf(refid, "%s_gtileset", baseid);
//    sprintf(temp, "%s_tileset", id);
//    outTileset(frameSprite->tileset, fs, fh, refid, temp, FALSE);
//
//    // FrameSprite structure
//    decl(fs, fh, "FrameSprite", id, 2, global);
//    // Y position
//    fprintf(fs, "    dc.w    %d\n", frameSprite->y);
//    // size infos
//    fprintf(fs, "    dc.w    %d\n", SPRITE_SIZE(frameSprite->w, frameSprite->h) << 8);
//    // index and attributs
//    fprintf(fs, "    dc.w    %d\n", frameSprite->ind | (frameSprite->attr << 11));
//    // X position
//    fprintf(fs, "    dc.w    %d\n", frameSprite->x);
//    // set tileset pointer
//    fprintf(fs, "    dc.l    %s\n", temp);
//    fprintf(fs, "\n");
//}

void outFrameSprite(frameSprite_* frameSprite, FILE* fs, FILE* fh, char* id, int global)
{
    // FrameSprite = VDPSpriteInf structure in SGDK
    decl(fs, fh, "VDPSpriteInf", id, 2, global);
    // Y position
    fprintf(fs, "    dc.w    %d\n", frameSprite->y);
    // size infos
    fprintf(fs, "    dc.w    %d\n", SPRITE_SIZE(frameSprite->w, frameSprite->h) << 0);
    // X position
    fprintf(fs, "    dc.w    %d\n", frameSprite->x);
    // Num tile
    fprintf(fs, "    dc.w    %d\n", frameSprite->numTile);
    fprintf(fs, "\n");
}

void outAnimFrame(animFrame_* animFrame, FILE* fs, FILE* fh, char* id, int global)
{
    int i;
    int numSprite;
    frameSprite_ **frameSprites;
    char temp[MAX_PATH_LEN];

    // EXPORT FRAME SPRITE
    numSprite = animFrame->numSprite;
    frameSprites = animFrame->frameSprites;
    // norm
    for(i = 0; i < numSprite; i++)
    {
        sprintf(temp, "%s_sprite%d", id, i);
        outFrameSprite(frameSprites[numSprite * 0], fs, fh, temp, FALSE);
        sprintf(temp, "%s_sprite%d_h", id, i);
        outFrameSprite(frameSprites[numSprite * 1], fs, fh, temp, FALSE);
        sprintf(temp, "%s_sprite%d_v", id, i);
        outFrameSprite(frameSprites[numSprite * 2], fs, fh, temp, FALSE);
        sprintf(temp, "%s_sprite%d_hv", id, i);
        outFrameSprite(frameSprites[numSprite * 3], fs, fh, temp, FALSE);
        // next
        frameSprites++;
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
    for(i = 0; i < animFrame->numSprite; i++)
        fprintf(fs, "    dc.l    %s_sprite%d_h\n", id, i);
    for(i = 0; i < animFrame->numSprite; i++)
        fprintf(fs, "    dc.l    %s_sprite%d_v\n", id, i);
    for(i = 0; i < animFrame->numSprite; i++)
        fprintf(fs, "    dc.l    %s_sprite%d_hv\n", id, i);

    fprintf(fs, "\n");

    // EXPORT COLLISION DATA
    if (animFrame->collision)
    {
        collision_* collision;

        i = 0;
        collision = animFrame->collision;
        while(collision)
        {
            sprintf(temp, "%s_collision%d", id, i);
            outCollision(collision, fs, fh, temp, FALSE);

            // do next collision (we also need to do inner collision in future)
            i++;
            collision = collision->next;
        }

        fprintf(fs, "\n");

//        // collisions data
//        strcpy(temp, id);
//        strcat(temp, "_collisions");
//        // declare
//        decl(fs, fh, NULL, temp, 2, FALSE);
//        // output collision pointers
//        for(i = 0; i < animFrame->numCollision; i++)
//            fprintf(fs, "    dc.l    %s_collision%d\n", id, i);
//
//        fprintf(fs, "\n");
    }

    // EXPORT TILESET
    sprintf(temp, "%s_tileset", id);
    outTileset(animFrame->tileset, fs, fh, temp, FALSE);

    // AnimationFrame structure
    decl(fs, fh, "AnimationFrame", id, 2, global);
    // set number of sprite
    fprintf(fs, "    dc.w    %d\n", animFrame->numSprite);
    // set frames pointer
    fprintf(fs, "    dc.l    %s_sprites\n", id);
//    // set number of collision
//    fprintf(fs, "    dc.w    %d\n", animFrame->numCollision);
//    // set collisions pointer
//    if (animFrame->numCollision > 0)
//        fprintf(fs, "    dc.l    %s_collisions\n", id);
    if (animFrame->collision)
        fprintf(fs, "    dc.l    %s_collision0\n", id);
    else
        fprintf(fs, "    dc.l    0\n");
    // set tileset pointer
    fprintf(fs, "    dc.l    %s_tileset\n", id);
    // frame width
    fprintf(fs, "    dc.w    %d\n", animFrame->w * 8);
    // frame height
    fprintf(fs, "    dc.w    %d\n", animFrame->h * 8);
    // timer info
    fprintf(fs, "    dc.w    %d\n", animFrame->timer);
    fprintf(fs, "\n");
}

void outAnimation(animation_* animation, FILE* fs, FILE* fh, char* id, int global)
{
    int i;
    animFrame_ **frames;
    char temp[MAX_PATH_LEN];

    // EXPORT FRAME
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

    // EXPORT SEQUENCE DATA
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

    // EXPORT ANIMATION
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
    // set maximum number of tile used by a single animation frame (used for VRAM tile space allocation)
    fprintf(fs, "    dc.w    %d\n", spriteDef->maxNumTile);
    // set maximum number of VDP sprite used by a single animation frame (used for VDP sprite allocation)
    fprintf(fs, "    dc.w    %d\n", spriteDef->maxNumSprite);

    fprintf(fs, "\n");
}
