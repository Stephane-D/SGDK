#ifndef _BASE_H_
#define _BASE_H_


#define PROCESS_PALETTE_FADING      (1 << 0)
#define PROCESS_BITMAP_TASK         (1 << 1)


// soft reset
void reset();

void setVBlankCallback(_voidCallback *CB);
void setHBlankCallback(_voidCallback *CB);


#endif // _BASE_H_
