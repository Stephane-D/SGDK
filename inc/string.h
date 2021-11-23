/**
 *  \file string.h
 *  \brief String manipulations
 *  \author Stephane Dallongeville
 *  \author Paspallas Dev
 *  \author Jack Nolddor
 *  \author Jesus Alonso (doragasu)
 *  \date 08/2011
 *
 * This unit provides basic null terminated string operations and type conversions.
 */

#if (ENABLE_NEWLIB != 0) && !defined(_NEWLIB_STRING_H_)
#define _NEWLIB_STRING_H_
#include_next <string.h> // Include string.h from newlib
#undef _STRING_H_        // Will be defined again just below
#endif

#ifndef _STRING_H_
#define _STRING_H_

#if (ENABLE_NEWLIB == 0) || !defined(ENABLE_NEWLIB)

/**
 *  \brief
 *      Test if specified character is a digit or not
 */
#define isdigit(c)      ((c) >= '0' && (c) <= '9')


typedef void *__gnuc_va_list;
typedef __gnuc_va_list va_list;

#define va_start(v,l) __builtin_va_start(v,l)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v,l) __builtin_va_arg(v,l)


/**
 *  \brief
 *      Calculate the length of a string (limited to 65535 characters maximum).
 *
 *  \param str
 *      The string we want to calculate the length.
 *  \return length of string
 *
 * This function calculates and returns the length of the specified string (limited to 65535 characters maximum).
 */
u16 strlen(const char *str);
/**
 *  \brief
 *      Compute the length of a string, to a maximum number of bytes.
 *
 *  \param str
 *      The string whose length you want to calculate.
 *  \param maxlen
 *      The maximum length to check.
 *
 *  \return The minimum of 'maxlen' and the number of characters that precede the terminating null character.
 *
 *  The strnlen() function computes the length of the string pointed to by 'str', not including the terminating null character ('\0'), <br>
 *  up to a maximum of 'maxlen' bytes. The function doesn't check any more than the first 'maxlen' bytes.
 */
u16 strnlen(const char *str, u16 maxlen);
/**
 *  \brief
 *      Compare the 2 strings.
 *
 *  \param str1
 *      The string we want to compare.
 *  \param str2
 *      The string we want to compare.
 *  \return an integral value indicating the relationship between the strings:<br>
 *      A zero value indicates that both strings are equal.<br>
 *      A value greater than zero indicates that the first character that does not match has a greater value in str1 than in str2<br>
 *      A value less than zero indicates the opposite.
 *
 * This function starts comparing the first character of each string.<br>
 * If they are equal to each other, it continues with the following pairs until
 * the characters differ or until a terminating null-character is reached.
 */
s16 strcmp(const char *str1, const char *str2);

/**
 *  \brief
 *      Copy a string.
 *
 *  \param dest
 *      Destination string (it must be large enough to receive the copy).
 *  \param src
 *      Source string.
 *  \return pointer on destination string.
 *
 * Copies the source string to destination.
 */
char* strcpy(char *dest, const char *src);
/**
 *  \brief
 *      Copy the first 'len' character of string.
 *
 *  \param dest
 *      Destination string (its size must be >= (len + 1)).
 *  \param src
 *      Source string.
 *  \param len
 *      Maximum number of character to copy.
 *  \return pointer on destination string.
 *
 * Copies the source string to destination.
 */
char* strncpy(char *dest, const char *src, u16 len);
/**
 *  \brief
 *      Concatenate two strings.
 *
 *  \param dest
 *      Destination string (it must be large enough to receive appending).
 *  \param src
 *      Source string.
 *  \return pointer on destination string.
 *
 * Appends the source string to the destination string.
 */
char* strcat(char *dest, const char *src);
/**
 *  \brief
 *      Composes a string with the same text that would be printed if format was used on printf,
 *      but instead of being printed, the content is stored as a C string in the buffer pointed by str.
 *
 *  \param buffer
 *      Destination string (it must be large enough to receive result).
 *  \param fmt
 *      C string that contains the text to be written to destination string.<br />
 *      It can optionally contain embedded format specifiers.
 *
 *  \param ... (additional arguments)
 *      Depending on the format string, the function may expect a sequence of additional arguments, <br>
 *      each containing a value to be used to replace a format specifier in the format string.
 *
 *      There should be at least as many of these arguments as the number of values specified in the format specifiers. <br>
 *      Additional arguments are ignored by the function.
 *
 *  \return On success, the total number of characters written is returned..
 *
 *  Copy the string pointed by 'fmt' param to the 'buffer' param.<br>
 *  If 'fmt' includes format specifiers (subsequences beginning with %), the additional arguments following format are
 *  formatted and inserted in the resulting string replacing their respective specifiers
 *
 */
u16 sprintf(char *buffer,const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));

#endif  // ENABLE_NEWLIB

/**
 *  \brief
 *      Clear a string.
 *
 *  \param str
 *      string to clear.
 *  \return pointer on the given string.
 *
 * Clear the specified string.
 */
char* strclr(char *str);
/**
 *  \brief
 *      Replace the given character in a string.
 *
 *  \param str
 *      The string to operate on.
 *  \param oldc
 *      The character being replaced.
 *  \param newc
 *      The character 'oldc' is replaced with.
 *  \return pointer to the null byte at the end of 'str'.
 *
 * Replace all occurrences of character in a null-terminated string.
 */
char *strreplacechar(char *str, char oldc, char newc);
/**
 *  \brief
 *      Convert a s32 value to string (input value should be in [-500000000..500000000] range).
 *
 *  \param value
 *      The s32 integer value to convert to string (input value should be in [-500000000..500000000] range).
 *  \param str
 *      Destination string (it must be large enough to receive result).
 *  \param minsize
 *      Minimum size of resulting string.
 *  \return string length
 *
 * Converts the specified s32 value to string.<br>
 * If resulting value is shorter than requested minsize the method prepends result with '0' character.
 */
u16 intToStr(s32 value, char *str, u16 minsize);
/**
 *  \brief
 *      Convert a u32 value to string (input value should be in [0..500000000] range).
 *
 *  \param value
 *      The u32 integer value to convert to string (input value should be in [0..500000000] range).
 *  \param str
 *      Destination string (it must be large enough to receive result).
 *  \param minsize
 *      Minimum size of resulting string.
 *  \return string length
 *
 * Converts the specified u32 value to string.<br>
 * If resulting value is shorter than requested minsize the method prepends result with '0' character.
 */
u16 uintToStr(u32 value, char *str, u16 minsize);
/**
 *  \brief
 *      Convert a u32 value to hexadecimal string.
 *
 *  \param value
 *      The u32 integer value to convert to hexadecimal string.
 *  \param str
 *      Destination string (it must be large enough to receive result).
 *  \param minsize
 *      Minimum size of resulting string.
 *  \return string length
 *
 * Converts the specified u32 value to hexadecimal string.<br>
 * If resulting value is shorter than requested minsize the method prepends result with '0' character.
 */
u16 intToHex(u32 value, char *str, u16 minsize);

/**
 *  \brief
 *      Convert a fix32 value to string.
 *
 *  \param value
 *      The fix32 value to convert to string.
 *  \param str
 *      Destination string (it must be large enough to receive result).
 *  \param numdec
 *      Number of wanted decimal.
 *
 * Converts the specified fix32 value to string.<br>
 */
void fix32ToStr(fix32 value, char *str, u16 numdec);
/**
 *  \brief
 *      Convert a fix16 value to string.
 *
 *  \param value
 *      The fix16 value to convert to string.
 *  \param str
 *      Destination string (it must be large enough to receive result).
 *  \param numdec
 *      Number of wanted decimal.
 *
 * Converts the specified fix16 value to string.<br>
 */
void fix16ToStr(fix16 value, char *str, u16 numdec);

#endif // _STRING_H_

