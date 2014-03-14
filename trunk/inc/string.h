/**
 *  \file string.h
 *  \brief String manipulations
 *  \author Stephane Dallongeville
 *  \date 08/2011
 *
 * This unit provides basic null terminated string operations and type conversions.
 */

#ifndef _STRING_H_
#define _STRING_H_


/**
 *  \brief
 *      Calculate the length of a string.
 *
 *  \param str
 *      The string we want to calculate the length.
 *  \return length of string.
 *
 * This function calculates and returns the length of the specified string.
 */
u32 strlen(const char *str);
/**
 *  \brief
 *      Compare the 2 strings.
 *
 *  \param str1
 *      The string we want to compare.
 *  \param str2
 *      The string we want to compare.
 *  \return an integral value indicating the relationship between the strings:</br>
 *      A zero value indicates that both strings are equal.</br>
 *      A value greater than zero indicates that the first character that does not match has a greater value in str1 than in str2</br>
 *      A value less than zero indicates the opposite.
 *
 * This function starts comparing the first character of each string.</br>
 * If they are equal to each other, it continues with the following pairs until
 * the characters differ or until a terminating null-character is reached.
 */
s16 strcmp(const char *str1, const char *str2);

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
 *      Convert a s32 value to string.
 *
 *  \param value
 *      The s32 integer value to convert to string.
 *  \param str
 *      Destination string (it must be large enough to receive result).
 *  \param minsize
 *      Minimum size of resulting string.
 *
 * Converts the specified s32 value to string.<br>
 * If resulting value is shorter than requested minsize the method prepends result with '0' character.
 */
void intToStr(s32 value, char *str, s16 minsize);
/**
 *  \brief
 *      Convert a u32 value to string.
 *
 *  \param value
 *      The u32 integer value to convert to string.
 *  \param str
 *      Destination string (it must be large enough to receive result).
 *  \param minsize
 *      Minimum size of resulting string.
 *
 * Converts the specified u32 value to string.<br>
 * If resulting value is shorter than requested minsize the method prepends result with '0' character.
 */
void uintToStr(u32 value, char *str, s16 minsize);
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
 *
 * Converts the specified u32 value to hexadecimal string.<br>
 * If resulting value is shorter than requested minsize the method prepends result with '0' character.
 */
void intToHex(u32 value, char *str, s16 minsize);

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
void fix32ToStr(fix32 value, char *str, s16 numdec);
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
void fix16ToStr(fix16 value, char *str, s16 numdec);


#endif // _STRING_H_

