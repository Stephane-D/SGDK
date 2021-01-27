#define TILEMAP_WIDTH       40
#define TILEMAP_HEIGHT      28
#define TILEMAP_SIZE        (TILEMAP_WIDTH * TILEMAP_HEIGHT)


void tm_init();
void tm_unpack(u16 frame);
bool tm_transfer();
