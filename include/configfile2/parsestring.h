#ifndef PARSESTRING_H_INCLUDED
#define PARSESTRING_H_INCLUDED
#include <stdint.h>
#include <stdbool.h>

//a new version of the library is currently in development. Therefore there is not a lot of comments in the source code

extern char* SkipBlankSpace(char* s);

extern char* SkipBlankSpaceReverse(char* s);

extern char* RstripString(char* s,char d);

extern char* SkipLine(char* s);

extern char* DelimitString(char* s,char d);

extern char* ReadNumber(char* read_ptr,void* ptr,bool* isdouble);

extern char* ReadInteger(char* readptr,int* u);
#endif