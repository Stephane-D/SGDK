/**
 *  \file   console.h
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
 *  be automatically reset before text is displayed (see assert macro below).
 */

// *****************************************************************************
//
//  Includes
//
// *****************************************************************************

#include "config.h"

#if (MODULE_CONSOLE != 0)

#include "types.h"
#include "string.h"
#include "maths.h"
#include "dma.h"

#pragma once

// *****************************************************************************
//
//  Defines
//
// *****************************************************************************

/// Comment out to disable asserts
#define ENABLE_ASSERT_CHECKS

// *****************************************************************************
//
//  Types
//
// *****************************************************************************

typedef u32 size_t;
typedef u32 ptrdiff_t;

/// Callback prototype for a vsprintf() function using va_list
typedef int (*vsprintf_t)(char *buf, const char *fmt, va_list args);
/// Callback prototype for a vsnprintf() function using va_list
typedef int (*vsnprintf_t)(char *buf, int count, const char *fmt, va_list args);

// *****************************************************************************
//
//  Macros
//
// *****************************************************************************

/// Helper macro to stringify numbers
#define str(s) xstr(s)
/// Helper macro
#define xstr(s) #s

/**
 *  \brief
 *      Assert a condition.
 *
 *  \param condition
 *      Expression that must evaluate to TRUE, otherwise an error error message
 *      in printed to the console. This will also reset the system to the SGDK
 *      default state.
 *
 *  Can be disabled by undefining ENABLE_ASSERT_CHECKS or by defining NDEBUG.
 */

#if defined(ENABLE_ASSERT_CHECKS) && !defined(NDEBUG)
#define assert(condition)                                                              \
    if ( !(condition) )                                                                \
    {                                                                                  \
        CON_reset();                                                                   \
        CON_systemResetOnNextWrite();                                                  \
        CON_write(__FILE__":"str(__LINE__)": Assertion \'"str(condition)"\' failed."); \
        while (TRUE);                                                                  \
    }
#else
#define assert(condition)
#endif

/**
 *  \brief
 *      Assert a condition.
 *
 * Uppercase version. Same as assert().
 */

#define ASSERT(condition) assert(condition)

// *****************************************************************************
//
//  Function Declarations
//
// *****************************************************************************

// -----------------------------------------------------------------------------
// printf functions
// -----------------------------------------------------------------------------

/**
 *  \brief
 *      Standard C library sprintf function.
 *
 *  \param buf
 *      Pointer to the buffer where the resulting C-string is stored.
 *  \param fmt
 *      C-string that contains the text to be written. It can optionally
 *      contain embedded format specifiers that are replaced by the values
 *      specified in subsequent additional arguments and formatted as requested.
 *      (See https://en.wikipedia.org/wiki/Printf_format_string)
 *  \param ... (additional arguments)
 *      The function may expect a sequence of additional arguments, each
 *      containing a value to be used to replace a format specifier in the
 *      format string.
 *  \return
 *      The total number of characters written is returned. This count does not
 *      include the additional null-character automatically appended at the end
 *      of the string.
 *
 *  Composes a string that would result from using a standard C library printf
 *  function. In case fmt includes format specifiers (beginning with %), the
 *  additional arguments are formatted and inserted in the resulting string
 *  replacing their respective specifiers.
 *
 *  This function is a wrapper for a vsprintf type function. Per default SGDK's
 *  built-in vsprintf() is used. CON_setVsprintf() can be used to register
 *  another implementation, e.g., stbsp_vsprintf().
 */

int CON_sprintf(char* buf, const char *fmt, ...)  __attribute__ ((format (printf, 2, 3)));

/**
 *  \brief
 *      Standard C library snprintf function.
 *
 *  \param buf
 *      Pointer to the buffer where the resulting C-string is stored.
 *  \param count
 *      Maximum number of bytes to be used in the buffer. The generated string
 *      has a length of at most count-1, leaving space for the additional
 *      terminating null-character.
 *  \param fmt
 *      C-string that contains the text to be written. It can optionally
 *      contain embedded format specifiers that are replaced by the values
 *      specified in subsequent additional arguments and formatted as requested.
 *      (See https://en.wikipedia.org/wiki/Printf_format_string)
 *  \param ... (additional arguments)
 *      The function may expect a sequence of additional arguments, each
 *      containing a value to be used to replace a format specifier in the
 *      format string.
 *  \return
 *      The total number of characters written is returned. This count does not
 *      include the additional null-character automatically appended at the end
 *      of the string.
 *
 *  Composes a string that would result from using a standard C library printf
 *  function. In case fmt includes format specifiers (beginning with %), the
 *  additional arguments are formatted and inserted in the resulting string
 *  replacing their respective specifiers.
 *
 *  This function is a wrapper for a vsnprintf type function. Per default no
 *  function is registered. CON_setVsnprintf() can be used to register an
 *  implementation, e.g., stbsp_vsnprintf().
 */

int CON_snprintf(char* buf, int count, const char *fmt, ...)  __attribute__ ((format (printf, 3, 4)));

/**
 *  \brief
 *      Register a standard vsprintf function as callback for CON_sprintf()
 *
 *  \param vsprintf_func
 *      Function pointer to a standard C library vsprintf function
 *
 *  Registers a callback function that will be wrapped by CON_sprintf(). The
 *  funcion supplied needs to be a vsprintf function that takes va_list as
 *  arguments, defined as follows:
 *
 *  typedef int (*vsprintf_t)(char *buf, const char *fmt, va_list args);
 *
 *  Possible implementations are SGDK's vsprintf() or alternatively
 *  stbsp_vsprintf() (http://github.com/nothings/stb).
 */

void CON_setVsprintf(vsprintf_t vsprintf_func);

/**
 *  \brief
 *      Register a standard vsnprintf function as callback for CON_snprintf()
 *
 *  \param vsnprintf_func
 *      Function pointer to a standard C library vsnprintf function
 *
 *  Registers a callback function that will be wrapped by CON_snprintf(). The
 *  funcion supplied needs to be a vsnprintf function that takes va_list as
 *  arguments, defined as follows:
 *
 *  typedef int (*vsnprintf_t)(char *buf, int count, const char *fmt, va_list args);
 *
 *  A possible implementation is stbsp_vsnprintf()
 *  (http://github.com/nothings/stb).
 */

void CON_setVsnprintf(vsnprintf_t vsnprintf_func);

// -----------------------------------------------------------------------------
// Console setup
// -----------------------------------------------------------------------------

/**
 *  \brief
 *      Set the size of the console window.
 *
 *  \param left
 *      Position of the leftmost window column in tiles. Default is 0.
 *  \param top
 *      Position of the topmost window row in tiles. Default is 0.
 *  \param width
 *      Width of the console window in tiles. Default is 40.
 *  \param height
 *      Height of the console window in tiles. Default is 28.
 *
 *  If either width or height are 0 then VDP_getScreenWidth() and
 *  VDP_getScreenHeight() will be used to determine suitable default values.
 */

void CON_setConsoleSize(u16 left, u16 top, u16 width, u16 height);

/**
 *  \brief
 *      Set the size of the character line buffer.
 *
 *  \param size
 *      Buffer size in bytes. This is the number of characters the buffer can
 *      hold including a terminating null-character.
 *
 *  Sets the size of the character buffer that is used internally to compose
 *  the formatted output string which is printed on the console.
 *
 *  Upon calling CON_setLineBufferSize(), the previous line buffer memory is
 *  immedieately freed. The new buffer will be allocated once the next call to
 *  CON_write() occurs.
 *
 *  Per default, the buffer size is set to 160 characters. Note that if only a
 *  vsprintf function is registered (see CON_setVsprintf()), the user is
 *  responsible for making sure a call to CON_write() does not exceed the line
 *  buffer. In case a vsnprintf function is registered (see CON_setVsnprintf()),
 *  it will automtically check the buffer size and prevent overflows.
 */

void CON_setLineBufferSize(u16 size);

/**
 *  \brief
 *      Set the transfer method used to upload the console tile buffer to VDP
 *      RAM.
 *
 *  \param tm
 *      Transfer method. <br>
 *      Accepted values are: <br>
 *      - CPU <br>
 *      - DMA <br>
 *      - DMA_QUEUE <br>
 *      - DMA_QUEUE_COPY
 *
 *  This sets the transfer method used with VDP_setTileMapDataRect(). The
 *  default value is DMA, which will cause an immediate upload. Note that when
 *  DMA_QUEUE or DMA_QUEUE_COPY is used, the user is responsible for triggering
 *  DMA upload, e.g., by calling SYS_doVBlankProcess().
 */

void CON_setTransferMethod(TransferMethod tm);

/**
 *  \brief
 *      Reset the console.
 *
 *  This function is a shortcut for:
 *
 *  CON_setConsoleSize(0, 0, 40, 28); <br>
 *  CON_setLineBufferSize(160); <br>
 *  CON_setTransferMethod(DMA);
 */

void CON_reset();

/**
 *  \brief
 *      Reset the system to the SGDK default state when CON_write() is called
 *      next time.
 *
 *  To reset the system the following SGDK functions will be called:
 *
 *  - SYS_disableInts() <br>
 *  - YM2612_reset() <br>
 *  - PSG_init() <br>
 *  - Z80_init() <br>
 *  - VDP_init()
 */

void CON_systemResetOnNextWrite();

// -----------------------------------------------------------------------------
// Console write functions
// -----------------------------------------------------------------------------

/**
 *  \brief
 *      Clear the console window.
 *
 *  This clears the console window by filling it with space characters. It is
 *  assumed that the first tile of the font is a space character. DMA settings
 *  apply here as well.
 */

void CON_clear();

/**
 *  \brief
 *      Set a new cursor position.
 *
 *  \param x
 *      New cursor x position.
 *  \param y
 *      New cursor y position.
 *
 *  This function specifies a new column and row for the cursor. This is the
 *  position where the next character will appear when CON_write() is processed.
 *  A position of (0,0) relates to the tile in the top/left corner.
 */

void CON_setCursorPosition(u16 x, u16 y);

/**
 *  \brief
 *      Return the current cursor position.
 *
 *  \return
 *      A 2D vector containing the current (x,y) position of the cursor, where
 *      (0,0) means the top/left corner of the console window.
 */

V2u16 CON_getCursorPosition();

/**
 *  \brief
 *      Write a C-string to the console window.
 *
 *  \param fmt
 *      C-string that contains the text to be written. It can optionally
 *      contain embedded format specifiers that are replaced by the values
 *      specified in subsequent additional arguments and formatted as requested.
 *      (See https://en.wikipedia.org/wiki/Printf_format_string)
 *  \param ... (additional arguments)
 *      The function may expect a sequence of additional arguments, each
 *      containing a value to be used to replace a format specifier in the
 *      format string.
 *  \return
 *      The total number of characters written is returned. This count does not
 *      include the additional null-character automatically appended at the end
 *      of the string.
 *
 *  This function writes a C-string into the console window where lines are
 *  automatically wrapped if the horizontal border of the screen is reached.
 *  When at the bottom of the screen window, the console content is moved up by
 *  one text line and a blank row is inserted.
 *
 *  Internally it uses either vsprintf or vsnprintf functions that have been
 *  registered with CON_setVsprintf() or CON_setVsnprintf(), respectively.
 *
 *  While processing the string, this function evaluates and executes escape
 *  control sequences. The following control characters are supported:
 *
 *  - \\b : backspace <br>
 *  - \\n : new line (line feed) <br>
 *  - \\r : carriage return <br>
 *  - \\t : horizontal tab <br>
 *  - \\v : vertical tab
 */

int CON_write(const char *fmt, ...)  __attribute__ ((format (printf, 1, 2)));

#endif // MODULE_CONSOLE
