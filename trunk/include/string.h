#ifndef _STRING_H_
#define _STRING_H_


u32 strlen(const char *str);
char* strcpy(char *to, const char *from);

void intToStr(s32 value, char *str, s16 minsize);
void uintToStr(u32 value, char *str, s16 minsize);

void fix32ToStr(fix32 value, char *str, s16 numdec);
void fix16ToStr(fix16 value, char *str, s16 numdec);


#endif // _STRING_H_

