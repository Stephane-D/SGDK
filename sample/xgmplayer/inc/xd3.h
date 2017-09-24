#ifndef XD3_H
#define XD3_H

typedef struct
{
    char *trackName;
    char *gameName;
    char *authorName;
    char *date;
    char *conversionAuthor;
    char *notes;
    u32 duration;
    u32 loopDuration;
} XD3;


s32 XD3_getDuration(XD3* xd3, s16 loopCnt);


#endif // XD3_H
