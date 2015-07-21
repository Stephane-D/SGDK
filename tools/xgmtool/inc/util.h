#ifndef UTIL_H_
#define UTIL_H_

#include <stdio.h>
#include <stdbool.h>


#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))


//typedef struct
//{
//    void** elements;
//    int allocated;
//    int size;
//} List;

typedef struct LList_
{
    void* element;
    struct LList_ *prev;
    struct LList_ *next;
} LList;


//void initList(List* list);
//List* createList();
//void deleteList(List* list);
//void clearList(List* list);
//void* getFromList(List* list, int index);
//void addToList(List* list, void* element);
//void addAllToList(List* list, List* elements);
//void addToListEx(List* list, int index, void* element);
//void addAllToListEx(List* list, int index, List* elements);
//void setToList(List* list, int index, void* element);
//void* removeFromList(List* list, int index);

LList* createEmptyElement();
LList* createElement(void* element);
void deleteLList(LList* list);
LList* getHeadLList(LList* list);
LList* getTailLList(LList* list);
int getSizeLList(LList* list);
LList* getElementAtLList(LList* list, int index);
LList* insertAfterLList(LList* linkedElement, void* element);
LList* insertBeforeLList(LList* linkedElement, void* element);
LList* insertAllAfterLList(LList* linkedElement, LList* elements);
LList* insertAllBeforeLList(LList* linkedElement, LList* elements);
LList* removeFromLList(LList* linkedElement);

//void* getFromLList(LList* list, int index);
//void addToLList(LList* list, void* element);
//void addAllToLList(LList* list, LList* elements);
//void setToLList(LList* list, int index, void* element);
//void* removeFromLList(LList* list, int index);

void** llistToArray(LList* list);
//void** listToArray(List* list);
//LList* listToLList(List* list);
//List* linkedListToList(LList* list);

bool arrayEquals(unsigned char* array1, unsigned char* array2, int size);
unsigned int swapNibble32(unsigned int value);
unsigned short swapNibble16(unsigned short value);
unsigned char swapNibble8(unsigned char value);
unsigned short getShort(unsigned char* data, int offset);
unsigned int getInt16(unsigned char* data, int offset);
unsigned int getInt24(unsigned char* data, int offset);
unsigned int getInt(unsigned char* data, int offset);
void setInt(unsigned char* array, int offset, unsigned int value);
void setInt24(unsigned char* array, int offset, unsigned int value);
void setInt16(unsigned char* array, int offset, unsigned int value);

char* getFileExtension(char* path);
unsigned int getFileSizeEx(FILE* f);
unsigned int getFileSize(char* file);
unsigned char* readBinaryFile(char* fileName, int* size);
bool writeBinaryFile(unsigned char* data, int size, char* fileName);
unsigned char* inEx(FILE* fin, int inOffset, int size, int* outSize);
int inEx2(FILE* fin, int inOffset, int size, unsigned char* dest, int outOffset);
bool out(unsigned char* data, int inOffset, int size, int intSize, bool swap, char* out);
bool outEx(unsigned char* data, int inOffset, int size, int intSize, bool swap, FILE* fout, int outOffset);

unsigned char* resample(unsigned char* data, int offset, int len, int inputRate, int outputRate, int align, int* outSize);


#endif // UTIL_H_
