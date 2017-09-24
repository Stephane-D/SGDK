#include "genesis.h"

#include "xd3.h"
#include "xgm.h"
#include "ym_state.h"
#include "psg_state.h"


// we want to modify it from here
extern vs16 frameToParse;


u32 XGM_getMusicDataOffset(const u8* xgm)
{
    u32 result;

    // get sample table size
    result = (xgm[0xFC] << 8) + (xgm[0xFD] << 16);

    return result + 0x104;
}

/*
u32 XGM_getDuration(const u8* xgm, s16 loopCnt)
{
    u8* xgmData;
    u8 *src;
    u32 result;
    u16 loop;

    xgmData = xgm + XGM_getMusicDataOffset(xgm);

    result = 0;
    loop = loopCnt + 1;
    src = xgmData;

    while(loop)
    {
        u16 frameSize = *src++ - 1;

        while(frameSize--)
        {
            u16 command = *src++ >> 1;
            u16 size = 0;

            switch(command & 0xF0)
            {
                case 0x10:  // PSG
                    if (command & 0x08)
                        size = (command & 0x03) + 1;
                    else
                        size = (command & 0x07) + 1;
                    break;

                case 0x20:  // YM port 0
                case 0x30:  // YM port 1
                case 0x60:  // YM state
                    size = (command & 0x0F) + 1;
                    size *= 2;
                    break;

                case 0x40:  // YM key on/off
                    size = (command & 0x0F) + 1;
                    break;

                case 0x50:  // PCM
                    size = 1;
                    break;

                case 0x70:  // special
                    switch(command)
                    {
                        u32 offset;

                        case 0x7E:  // loop

                            offset = *src++;
                            offset |= *src++ << 8;
                            offset |= *src++ << 16;

                            // restart from loop point
                            src = xgmData + offset;
                            // done
                            loop--;
                            frameSize = 0;
                            break;

                        case 0x7F:  // end
                            // done
                            loop = 0;
                            frameSize = 0;
                            break;
                    }
                    break;
            }

            src += size;
            frameSize -= size;
        }

        result++;
    }

    return result;
}
*/
void XGM_getXD3(const u8* xgm, XD3* xd3)
{
    // XD3 tag ?
    if (xgm[0xFF] & 2)
    {
        u32 offset;

        // get music size field offset
        offset = XGM_getMusicDataOffset(xgm) - 4;
        // add music data size
        offset += (xgm[offset + 0] << 0) + (xgm[offset + 1] << 8) + (xgm[offset + 2] << 16);
        // add both size field len (music and xd3)
        offset += 8;

        xd3->trackName = (char*) (xgm + offset);
        offset += strlen(xd3->trackName) + 1;
        xd3->gameName = (char*) (xgm + offset);
        offset += strlen(xd3->gameName) + 1;
        xd3->authorName = (char*) (xgm + offset);
        offset += strlen(xd3->authorName) + 1;
        xd3->date = (char*) (xgm + offset);
        offset += strlen(xd3->date) + 1;
        xd3->conversionAuthor = (char*) (xgm + offset);
        offset += strlen(xd3->conversionAuthor) + 1;
        xd3->notes = (char*) (xgm + offset);
        offset += strlen(xd3->notes) + 1;
        xd3->duration = (xgm[offset + 0] << 0) + (xgm[offset + 1] << 8) + (xgm[offset + 2] << 16) + (xgm[offset + 3] << 24);
        offset += 4;
        xd3->loopDuration = (xgm[offset + 0] << 0) + (xgm[offset + 1] << 8) + (xgm[offset + 2] << 16) + (xgm[offset + 3] << 24);
        offset += 4;
    }
}

s32 XD3_getDuration(XD3* xd3, s16 loopCnt)
{
    s32 result = xd3->duration;
    u32 loopDuration = xd3->loopDuration;

    if (loopDuration)
    {
        // infinite loop --> infinite time
        if (loopCnt == -1) return -1;

        s16 i = loopCnt;
        while(i--) result += loopDuration;
    }

    return result;
}


s32 XGM_parseFrame(u8* frameData, YM* ym, PSG *psg)
{
    u8 *src = frameData;
    s32 totalSize = 0;
    u32 loop = 0;
    u16 end = 0;
    u16 frameSize = *src++;
    totalSize += frameSize;
    frameSize--;

    while(frameSize--)
    {
        u16 command = *src++ >> 1;
        u16 size = 0;

        switch(command & 0xF0)
        {
            case 0x10:  // PSG
                if (command & 0x08)
                    size = (command & 0x03) + 1;
                else
                    size = (command & 0x07) + 1;
                PSGState_XGMWrites(psg, src, size);
                break;

            case 0x20:  // YM port 0
                size = (command & 0x0F) + 1;
                YM_XGMWrites(ym, 0, src, size);
                size *= 2;
                break;

            case 0x30:  // YM port 1
                size = (command & 0x0F) + 1;
                YM_XGMWrites(ym, 1, src, size);
                size *= 2;
                break;

            case 0x40:  // YM key on/off
                size = (command & 0x0F) + 1;
                YM_XGMKeyWrites(ym, src, size);
                break;

            case 0x50:  // PCM
                size = 1;
                break;

            case 0x60:  // YM state
                size = (command & 0x0F) + 1;
                size *= 2;
                break;

            case 0x70:  // special
                switch(command)
                {
                    case 0x7D:  // extra frame
                        frameToParse++;
                        break;

                    case 0x7E:  // loop
                        loop = src[0];
                        loop |= src[1] << 8;
                        loop |= src[2] << 16;
                        size = 3;
                        break;

                    case 0x7F:  // end
                        end = 1;
                        break;
                }
                break;
        }

        src += size;
        frameSize -= size;
    }

    // loop has priority over end in same frame
    if (loop) return -loop;
    if (end) return 0;

    return totalSize;
}
