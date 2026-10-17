#include <utils/virtual_keyboard.h>

typedef struct {
    uint8_t x;
    uint8_t y;
    char lower;
    char upper;
} VirtualKeyboardKey;

static const VirtualKeyboardKey keys[] = {
    {0, 4, 'q', 'Q'}, {1, 4, 'w', 'W'}, {2, 4, 'e', 'E'}, {3, 4, 'r', 'R'},
    {4, 4, 't', 'T'}, {5, 4, 'y', 'Y'}, {6, 4, 'u', 'U'}, {7, 4, 'i', 'I'},
    {8, 4, 'o', 'O'}, {9, 4, 'p', 'P'},
    {0, 5, 'a', 'A'}, {1, 5, 's', 'S'}, {2, 5, 'd', 'D'}, {3, 5, 'f', 'F'},
    {4, 5, 'g', 'G'}, {5, 5, 'h', 'H'}, {6, 5, 'j', 'J'}, {7, 5, 'k', 'K'},
    {8, 5, 'l', 'L'},
    {0, 6, 'z', 'Z'}, {1, 6, 'x', 'X'}, {2, 6, 'c', 'C'}, {3, 6, 'v', 'V'},
    {4, 6, 'b', 'B'}, {5, 6, 'n', 'N'}, {6, 6, 'm', 'M'},
    {0, 8, '0', '0'}, {1, 8, '1', '1'}, {2, 8, '2', '2'}, {3, 8, '3', '3'},
    {4, 8, '4', '4'}, {5, 8, '5', '5'}, {6, 8, '6', '6'}, {7, 8, '7', '7'},
    {8, 8, '8', '8'}, {9, 8, '9', '9'},
    {0, 9, '@', '@'}, {1, 9, '_', '_'}, {2, 9, '-', '-'}, {3, 9, '.', '.'},
    {4, 9, ' ', ' '}, {5, 9, '!', '!'}, {6, 9, '#', '#'}, {7, 9, '$', '$'},
    {8, 9, '%', '%'}, {9, 9, '&', '&'}
};

#define KEY_COUNT ((uint16_t)(sizeof(keys) / sizeof(keys[0])))

static char keyChar(const VirtualKeyboard *keyboard, uint16_t index)
{
    return keyboard->uppercase ? keys[index].upper : keys[index].lower;
}

static uint16_t moveSelection(uint16_t selected, uint16_t direction)
{
    const VirtualKeyboardKey *current = &keys[selected];
    uint16_t best = selected;
    uint16_t bestScore = 0xFFFF;

    for (uint16_t index = 0; index < KEY_COUNT; index++) {
        const VirtualKeyboardKey *candidate = &keys[index];
        uint16_t score = 0xFFFF;
        uint16_t deltaX;
        uint16_t deltaY;

        if (index == selected) continue;
        if (direction == BUTTON_LEFT && candidate->y == current->y && candidate->x < current->x) score = current->x - candidate->x;
        else if (direction == BUTTON_RIGHT && candidate->y == current->y && candidate->x > current->x) score = candidate->x - current->x;
        else if (direction == BUTTON_UP && candidate->y < current->y) {
            deltaX = current->x > candidate->x ? current->x - candidate->x : candidate->x - current->x;
            deltaY = current->y - candidate->y;
            score = deltaY * 32u + deltaX;
        } else if (direction == BUTTON_DOWN && candidate->y > current->y) {
            deltaX = current->x > candidate->x ? current->x - candidate->x : candidate->x - current->x;
            deltaY = candidate->y - current->y;
            score = deltaY * 32u + deltaX;
        }
        if (score < bestScore) {
            bestScore = score;
            best = index;
        }
    }
    return best;
}

void VIRTUAL_KEYBOARD_begin(VirtualKeyboard *keyboard, char *buffer,
        uint16_t capacity, const char *title)
{
    keyboard->buffer = buffer;
    keyboard->capacity = capacity;
    keyboard->length = strlen(buffer);
    if (keyboard->length >= capacity) {
        keyboard->length = capacity - 1;
        buffer[keyboard->length] = '\0';
    }
    keyboard->selectedKey = 10;
    keyboard->uppercase = FALSE;
    keyboard->title = title;
    keyboard->result = VIRTUAL_KEYBOARD_EDITING;
}

void VIRTUAL_KEYBOARD_handle(VirtualKeyboard *keyboard, uint16_t actions)
{
    uint16_t direction = actions & (BUTTON_UP | BUTTON_DOWN | BUTTON_LEFT | BUTTON_RIGHT);

    if (keyboard->result != VIRTUAL_KEYBOARD_EDITING) return;
    if (direction & BUTTON_UP) keyboard->selectedKey = moveSelection(keyboard->selectedKey, BUTTON_UP);
    else if (direction & BUTTON_DOWN) keyboard->selectedKey = moveSelection(keyboard->selectedKey, BUTTON_DOWN);
    else if (direction & BUTTON_LEFT) keyboard->selectedKey = moveSelection(keyboard->selectedKey, BUTTON_LEFT);
    else if (direction & BUTTON_RIGHT) keyboard->selectedKey = moveSelection(keyboard->selectedKey, BUTTON_RIGHT);
    if (actions & BUTTON_A && keyboard->length + 1 < keyboard->capacity) {
        keyboard->buffer[keyboard->length++] = keyChar(keyboard, keyboard->selectedKey);
        keyboard->buffer[keyboard->length] = '\0';
    }
    if (actions & BUTTON_B && keyboard->length) keyboard->buffer[--keyboard->length] = '\0';
    if (actions & BUTTON_C) keyboard->uppercase = !keyboard->uppercase;
    if (actions & BUTTON_START) keyboard->result = VIRTUAL_KEYBOARD_ACCEPTED;
    if (actions & BUTTON_MODE) keyboard->result = VIRTUAL_KEYBOARD_CANCELLED;
}

void VIRTUAL_KEYBOARD_draw(const VirtualKeyboard *keyboard)
{
    char character[2] = {'\0', '\0'};

    VDP_clearTextArea(0, 0, 40, 28);
    VDP_drawText(keyboard->title, 0, 0);
    VDP_drawText(keyboard->buffer, 0, 1);
    VDP_drawText("D-PAD mover A anadir B borrar", 0, 12);
    VDP_drawText("C mayusculas START aceptar MODE cancelar", 0, 13);
    for (uint16_t index = 0; index < KEY_COUNT; index++) {
        character[0] = keyChar(keyboard, index);
        VDP_drawText(character, keys[index].x * 2u + 1u, keys[index].y);
    }
    VDP_drawText(">", keys[keyboard->selectedKey].x * 2u, keys[keyboard->selectedKey].y);
}