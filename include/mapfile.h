#ifndef MAPFILE_H_INCLUDED
#define MAPFILE_H_INCLUDED

#include <stdint.h>

//lit la map
extern uint32_t* ReadMap(char *filename);

//écrit la map
extern uint32_t* WriteMap(char* filename, uint32_t* bytes);

//affiche sous la forme d'un tableau la map
extern void PrintMap(uint32_t* bytes);

//génère une map vide
extern uint32_t* GenerateMap(uint32_t width,uint32_t height);

#endif