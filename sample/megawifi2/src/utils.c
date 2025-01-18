#include "genesis.h"

#include "utils.h"

u16 readButton(u16 joy){
    static u16 lastValue = 0;
    u16 val = JOY_readJoypad(JOY_1);
    if(val != lastValue){
        lastValue = val;
    }else{
        return 0;
    }
    return lastValue;
}
void clearScreen(){
    strclr(buffer);
    //VDP_clearTextArea(0u,3u, 1u, 3u);
    for(u16 i = 0; i< SCREEN_ROWS - 1; i++){
        VDP_clearTextLine(i);
    }
}

// Obtener el carácter en la posición (x, y)
static char getCharFromPosition(u16 x, u16 y) {
    // Calcula el carácter basado en su posición en la cuadrícula
    // Offset por empezar en el carácter 32 (espacio)
    u16 index = y * SCREEN_COLUMNS + x; // Filas comienzan en 1
    char c = 32 + index;

    if (c >= 32 && c <= 126) {
        return c;
    }

    return '\0'; // Fuera del rango imprimible
}

// Actualizar la posición del cursor según la entrada del pad
static void updateCursor(u16 *cursorX, u16 *cursorY, u16 button) {
    if (button == BUTTON_RIGHT && *cursorX < SCREEN_COLUMNS - 1) (*cursorX)++;
    if (button == BUTTON_LEFT && *cursorX > 0) (*cursorX)--;
    if (button == BUTTON_DOWN && *cursorY < 5) (*cursorY)++;
    if (button == BUTTON_UP && *cursorY > 0) (*cursorY)--; // Comienza en fila 1
}

// Dibujar el alfabeto en la pantalla
static void drawAlphabet() {
    char c[2] = " "; // Primer carácter imprimible (espacio)

    for (u16 y = 1; y < SCREEN_ROWS; y++) { // Comienza en la fila 1 (para no tapar el buffer de texto)
        for (u16 x = 0; x < SCREEN_COLUMNS; x++) {
            if (c[0] > 126) return; // Detener cuando se pinten todos los caracteres
            VDP_drawText(c, (x) * 2u + 1u, y);
            c[0]++; // Siguiente carácter
        }
    }
}

void delay_ms(u16 milliseconds) {
    u16 frames = MS_TO_FRAMES(milliseconds);
    for (u16 i = 0; i < frames; i++) {
        SYS_doVBlankProcess(); // Espera al siguiente VBlank
    }
}

void println(const char *str)
{
	if (str) {
		VDP_drawText(str, 1, 0);
	}
}

int readText(char* buffer, size_t lengthMax){
    u16 button;
    bool next = TRUE;
    strclr(buffer);
    u16 textIndex = 0; 
    u16 cursorX = 0, cursorY = 0; // Posición inicial del cursor
    u16 prevCursorX = cursorX, prevCursorY = cursorY; // Para evitar redibujos innecesarios
    clearScreen();
    drawAlphabet();  
    VDP_drawText(">", cursorX * 2, cursorY + 1 ); // Dibuja el nuevo cursor
    do {
        button = readButton(JOY_1);
        // Actualizar la posición del cursor
        updateCursor(&cursorX, &cursorY, button);

        // Dibujar el cursor en la nueva posición
        if (cursorX != prevCursorX || cursorY != prevCursorY) {
            VDP_drawText(" ", prevCursorX * 2, prevCursorY + 1); // Borra el cursor anterior
            VDP_drawText(">", cursorX * 2, cursorY + 1 ); // Dibuja el nuevo cursor
            prevCursorX = cursorX;
            prevCursorY = cursorY;
        }
        // Leer la entrada del botón A para seleccionar el carácter
        if (button == BUTTON_A) {
            char selectedChar = getCharFromPosition(cursorX, cursorY);
            if (selectedChar && textIndex < lengthMax - 1) {
                buffer[textIndex++] = selectedChar; // Agregar al buffer
                buffer[textIndex] = '\0';          // Finalizar la cadena

                // Mostrar el texto construido hasta ahora en la esquina superior izquierda
                VDP_clearTextLine(0); // Borra la línea superior
                VDP_drawText(buffer, 0, 0);
            }
        }
        if (button == BUTTON_B) {
            char selectedChar = getCharFromPosition(cursorX, cursorY);
            if (selectedChar && textIndex < lengthMax - 1) {
                textIndex--;
                buffer[textIndex] = '\0';          // Finalizar la cadena
                // Mostrar el texto construido hasta ahora en la esquina superior izquierda
                VDP_clearTextLine(0); // Borra la línea superior
                VDP_drawText(buffer, 0, 0);
            }
        }
        if (button == BUTTON_START) {
            next = FALSE;
        }
        SYS_doVBlankProcess();
    }while(next);
    return textIndex;
}

void print(){
    ciclo++;
    sprintf(buffer, "%2u", option);
    VDP_drawText(buffer, 1u, 27u);
    sprintf(buffer, "%6lu", ciclo);
    VDP_drawText(buffer, 25u, 27u);
    SYS_doVBlankProcess();
}

void printStatus(union mw_msg_sys_stat * status){    
    if(status!=NULL){
        if(status->sys_stat == MW_ST_READY){            
            VDP_drawText("READY", 5u, 27u);
        }else{
            u16 oldColors[16]; 
            PAL_getColors(VDP_getTextPalette(), oldColors, 16);
            PAL_setColors(VDP_getTextPalette(), palette_red, 16, CPU);
            //PAL_setColor(15, RGB24_TO_VDPCOLOR(0xFF0000));
            VDP_drawText("NO CON", 5u, 27u);
            PAL_setColors(VDP_getTextPalette(), oldColors, 16, CPU);
        }
    }
    SYS_doVBlankProcess();
}

