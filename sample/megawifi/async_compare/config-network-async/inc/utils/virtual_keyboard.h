#ifndef VIRTUAL_KEYBOARD_H
#define VIRTUAL_KEYBOARD_H

#include "genesis.h"

typedef enum {
    VIRTUAL_KEYBOARD_EDITING,
    VIRTUAL_KEYBOARD_ACCEPTED,
    VIRTUAL_KEYBOARD_CANCELLED
} VirtualKeyboardResult;

typedef struct {
    char *buffer;
    uint16_t capacity;
    uint16_t length;
    uint16_t selectedKey;
    bool uppercase;
    const char *title;
    VirtualKeyboardResult result;
} VirtualKeyboard;

void VIRTUAL_KEYBOARD_begin(VirtualKeyboard *keyboard, char *buffer,
        uint16_t capacity, const char *title);
void VIRTUAL_KEYBOARD_handle(VirtualKeyboard *keyboard, uint16_t actions);
void VIRTUAL_KEYBOARD_draw(const VirtualKeyboard *keyboard);

#endif