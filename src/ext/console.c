/**
 *  \file   console.c
 *  \brief  TTY text console
 *  \author Andreas Dietrich
 *  \date   09/2022
 *
 *  This unit provides a simple TTY text console. Characters are written as a
 *  stream, where lines are automatically wrapped if the horizontal border of
 *  the screen is reached. When at the bottom of the screen window, the console
 *  content is moved up by one text line and a blank row is inserted.
 *
 *  Per default, the console occupies a standad screen of 40x28 tiles. All text
 *  attributes, such as font, palette, plane etc., are taken from SGDK text
 *  settings. Screen updates are done using DMA transfer mode (which can be
 *  changed with CON_setTransferMethod()).
 *
 *  One of the use cases are assert messages. To this end, the Genesis state can
 *  be automatically reset before text is displayed (see assert macro in
 *  console.h).
 */

// *****************************************************************************
//
//  Includes
//
// *****************************************************************************

#include "config.h"

#if (MODULE_CONSOLE != 0)

#include "types.h"
#include "vdp.h"
#include "vdp_bg.h"
#include "vdp_tile.h"
#include "sys.h"
#include "ym2612.h"
#include "psg.h"
#include "z80_ctrl.h"
#include "memory.h"

#include "ext/console.h"

// *****************************************************************************
//
//  External functions
//
// *****************************************************************************

// SGDK vsprintf() used as default
extern u16 vsprintf(char *buf, const char *fmt, va_list args);

// *****************************************************************************
//
//  Defines
//
// *****************************************************************************

#define CONSOLE_TAB_SIZE 8 // horizontal tab width

// *****************************************************************************
//
//  Variables
//
// *****************************************************************************

static bool m_consoleDoBufferReset = TRUE;
static bool m_consoleDoSystemReset = FALSE;

static u16 m_consoleX      =  0;
static u16 m_consoleY      =  0;
static u16 m_consoleLeft   =  0;
static u16 m_consoleTop    =  0;
static u16 m_consoleWidth  = 40;
static u16 m_consoleHeight = 28;

static TransferMethod m_consoleTransferMethod = DMA;

static u16*  m_consoleFrameBuffer    = NULL;
static char* m_consoleLineBuffer     = NULL;
static u16   m_consoleLineBufferSize = 160;

static vsprintf_t  m_vsprintf_func  = (vsprintf_t)vsprintf;
static vsnprintf_t m_vsnprintf_func = NULL;

// *****************************************************************************
//
//  Functions
//
// *****************************************************************************

// -----------------------------------------------------------------------------
//  Private console functions
// -----------------------------------------------------------------------------

static u16 consoleGetBasetile()
{
    // Build a basetile with current text attributes
    return TILE_ATTR_FULL(
        VDP_getTextPalette(),
        VDP_getTextPriority(),
        FALSE,
        FALSE,
        TILE_FONT_INDEX
    );
}

// -----------------------------------------------------------------------------

static u16* consoleGetFrameBuffer()
{
    // Reset SGDK state
    if (m_consoleDoSystemReset)
    {
        SYS_disableInts();
        YM2612_reset();
        PSG_reset();
        Z80_init();
        VDP_init();

        m_consoleDoSystemReset = FALSE;
    }

    // Reset console buffer
    if (m_consoleDoBufferReset)
    {
        const u16 basetile = consoleGetBasetile();
        const u16 tiles    = m_consoleWidth * m_consoleHeight;
        const u16 bytes    = tiles * 2;

        // (Re)allocate frame buffer memory
        MEM_free(m_consoleFrameBuffer);
        m_consoleFrameBuffer = (u16*)MEM_alloc(bytes);

        // Clear frame buffer memory
        if (m_consoleFrameBuffer)
        {
            memsetU16(m_consoleFrameBuffer, basetile, tiles);
            m_consoleDoBufferReset = FALSE;
        }
    }

    return m_consoleFrameBuffer;
}

// -----------------------------------------------------------------------------

static void consoleClearFrameBuffer()
{
    // Clear frame buffer memory
    if (m_consoleFrameBuffer)
    {
        const u16 basetile = consoleGetBasetile();
        const u16 tiles    = m_consoleWidth * m_consoleHeight;

        memsetU16(m_consoleFrameBuffer, basetile, tiles);
    }
}

// -----------------------------------------------------------------------------

static char* consoleGetLineBuffer()
{
    // Allocate line buffer memory
    if (!m_consoleLineBuffer)
        m_consoleLineBuffer = MEM_alloc(m_consoleLineBufferSize);

    return m_consoleLineBuffer;
}

// -----------------------------------------------------------------------------

static void consoleScroll()
{
    // Scroll frame buffer
    if (m_consoleFrameBuffer)
    {
        u16* const dst = m_consoleFrameBuffer;
        u16* const src = dst + m_consoleWidth;

        const u16 tiles    = m_consoleWidth * (m_consoleHeight-1);
        const u16 basetile = consoleGetBasetile();

        // Move upper part of buffer and clear last line
        memcpy(dst, src, tiles * 2);
        memsetU16(dst+tiles, basetile, m_consoleWidth);
    }
}

// -----------------------------------------------------------------------------

static void consoleUploadFrameBuffer()
{
    // Upload the console tile map to VDP RAM
    if (m_consoleFrameBuffer)
    {
        VDP_setTileMapDataRect(
            VDP_getTextPlane(),
            m_consoleFrameBuffer,
            m_consoleLeft,
            m_consoleTop,
            m_consoleWidth,
            m_consoleHeight,
            m_consoleWidth,
            m_consoleTransferMethod
        );
    }
}

// -----------------------------------------------------------------------------

static void consoleBackspace()
{
    // Mover cursor position one tile to the left
    if (m_consoleX > 0)
        m_consoleX--;
}

// -----------------------------------------------------------------------------

static void consoleCarriageReturn()
{
    // Mover cursor position to start of current line
    m_consoleX = 0;
}

// -----------------------------------------------------------------------------

static void consoleHorizontalTab()
{
    // Move cursor to next tab positon.
    // This is the next position divisible by CONSOLE_TAB_SIZE.
    m_consoleX = min((m_consoleX/CONSOLE_TAB_SIZE + 1) * CONSOLE_TAB_SIZE,
                      m_consoleWidth-1);
}

// -----------------------------------------------------------------------------

static void consoleVerticalTab()
{
    // Move cursor down and scroll if we are past the bottom line
    if (++m_consoleY >= m_consoleHeight)
    {
        m_consoleY = m_consoleHeight - 1;
        consoleScroll();
    }
}

// -----------------------------------------------------------------------------

static void consoleNewLine()
{
    // Move cursor to the start of the next line
    consoleCarriageReturn();
    consoleVerticalTab();
}

// -----------------------------------------------------------------------------

static void consoleCharacter(char c, u16* buffer, u16 basetile)
{
    // Test if we are past the right border and create a newline if so
    if (m_consoleX >= m_consoleWidth)
        consoleNewLine();

    // Clamp cursor position to window
    m_consoleX = min(m_consoleX, m_consoleWidth-1);
    m_consoleY = min(m_consoleY, m_consoleHeight-1);

    // Insert tile code (basetile index = ASCII code - 32)
    buffer[m_consoleY * m_consoleWidth + m_consoleX] = basetile + (c - 32);

    // Move cursor to the right. Note that we do not automatically create a new
    // line if we are past the right border. This is because the next character
    // might be '\n'. This avoids creating an unwanted blank row.
    m_consoleX++;
}

// -----------------------------------------------------------------------------

static void consolePrint(const char *str)
{
    // Get frame buffer pointer
    u16* const buffer = consoleGetFrameBuffer();
    if (!buffer)
        return;

    // Get base tile
    const u16 basetile = consoleGetBasetile();

    // Execute control code or insert new character
    char c;
    while ((c = *str++))
    {
        switch (c)
        {
            case '\b': consoleBackspace();      break;
            case '\n': consoleNewLine();        break;
            case '\r': consoleCarriageReturn(); break;
            case '\t': consoleHorizontalTab();  break;
            case '\v': consoleVerticalTab();    break;

            default:
                consoleCharacter(c, buffer, basetile);
        }
    }
}

// -----------------------------------------------------------------------------
//  Public printf functions
// -----------------------------------------------------------------------------

int CON_sprintf(char* buf, const char *fmt, ...)
{
    int len = 0;

    // Collect arguments and call vsprintf
    va_list args;
    va_start(args, fmt);

    if (m_vsprintf_func)
        len = m_vsprintf_func(buf, fmt, args);

    va_end(args);

    return len;
}

// -----------------------------------------------------------------------------

int CON_snprintf(char* buf, int count, const char *fmt, ...)
{
    int len = 0;

    // Collect arguments and call vsnprintf
    va_list args;
    va_start(args, fmt);

    if (m_vsnprintf_func)
        len = m_vsnprintf_func(buf, count, fmt, args);

    va_end(args);

    return len;
}

// -----------------------------------------------------------------------------

void CON_setVsprintf(vsprintf_t vsprintf_func)
{
    m_vsprintf_func = vsprintf_func;
}

// -----------------------------------------------------------------------------

void CON_setVsnprintf(vsnprintf_t vsnprintf_func)
{
    m_vsnprintf_func = vsnprintf_func;
}

// -----------------------------------------------------------------------------
//  Public console functions
// -----------------------------------------------------------------------------

void CON_setConsoleSize(u16 left, u16 top, u16 width, u16 height)
{
    // Set console window size (use system defaults if width/height is 0)
    if (width && height)
    {
        m_consoleLeft   = left;
        m_consoleTop    = top;
        m_consoleWidth  = width;
        m_consoleHeight = height;
    }
    else
    {
        m_consoleLeft   = 0;
        m_consoleTop    = 0;
        m_consoleWidth  = VDP_getScreenWidth() / 8;
        m_consoleHeight = VDP_getScreenHeight() / 8;
    }

    // Reset cursor position
    m_consoleX = m_consoleY = 0;

    m_consoleDoBufferReset = TRUE;
}

// -----------------------------------------------------------------------------

void CON_setLineBufferSize(u16 size)
{
    // Free current line buffer
    MEM_free(m_consoleLineBuffer);
    m_consoleLineBuffer = NULL;

    // Only set new size, allocation will be done on next write
    m_consoleLineBufferSize = size;
}

// -----------------------------------------------------------------------------

void CON_setTransferMethod(TransferMethod tm)
{
    // SGDK transfer method used when uploading the buffer tile map
    m_consoleTransferMethod = tm;
}

// -----------------------------------------------------------------------------

void CON_reset()
{
    // Reset window size, line buffer size, and transfer method to defaults
    CON_setConsoleSize(0, 0, 40, 28);
    CON_setLineBufferSize(160);
    CON_setTransferMethod(DMA);
}

// -----------------------------------------------------------------------------

void CON_systemResetOnNextWrite()
{
    // Reset Genesis to SGDK default state on next write
    m_consoleDoSystemReset = TRUE;
}

// -----------------------------------------------------------------------------

void CON_clear()
{
    // Reset cursor position
    m_consoleX = m_consoleY = 0;

    // Clear and upload tile map
    consoleClearFrameBuffer();
    consoleUploadFrameBuffer();
}

// -----------------------------------------------------------------------------

void CON_setCursorPosition(u16 x, u16 y)
{
    // Set new cursor position and clamp to window
    m_consoleX = min(x, m_consoleWidth-1);
    m_consoleY = min(y, m_consoleHeight-1);
}

// -----------------------------------------------------------------------------

V2u16 CON_getCursorPosition()
{
    // Create vector containing cursor position
    V2u16 result = { m_consoleX, m_consoleY };

    return result;
}

// -----------------------------------------------------------------------------

int CON_write(const char *fmt, ...)
{
    int   len = 0;
    char* buf = consoleGetLineBuffer();

    if (buf)
    {
        // Collect arguments
        va_list args;
        va_start(args, fmt);

        // Format output string, prefer a function with buffer size check
        if (m_vsnprintf_func)
            len = m_vsnprintf_func(buf, m_consoleLineBufferSize, fmt, args);
        else if (m_vsprintf_func)
            len = m_vsprintf_func(buf, fmt, args);

        va_end(args);

        // Process characters and upload tile map
        if (len)
        {
            consolePrint(buf);
            consoleUploadFrameBuffer();
        }
    }

    return len;
}

#endif // MODULE_CONSOLE
