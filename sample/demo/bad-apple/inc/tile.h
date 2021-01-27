#define TILESPACE_FRAME     512


void ts_init();
u16 ts_unpack(u16 frame);
bool ts_transfer(u16 numTile);

