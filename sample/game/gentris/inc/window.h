#ifndef HEADER_WINDOW
#define HEADER_WINDOW

#include <types.h>

// Structure for window state storage
typedef struct
{
    u8 columnLeft;
    u8 columnRight;
    u8 rowTop;
    u8 rowBottom;
    u8 firstScanline;
    u8 lastScanline;
    bool enabled; // Is window enabled
} Window;


void Window_SetCentered(const u8 x, const u8 y, const u8 width, const u8 height);
void Window_SetHidden();
#endif // HEADER_WINDOW
