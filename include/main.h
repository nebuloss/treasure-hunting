#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED
#include <stdio.h>
#include "logfile.h"

#define TEXTURE_WIDTH 32 //do not change these values
#define TEXTURE_HEIGHT 32
#define TEXTURE_MASK 0xFFFFFFE0 //modulus 32

extern FILE* logfile; //share logfile stream to other module

#endif

