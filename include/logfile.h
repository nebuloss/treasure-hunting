#ifndef LOGFILE_H_INCLUDED
#define LOGFILE_H_INCLUDED
#include <stdio.h>

#define LOG_INFO 0 //les différents types de messages
#define LOG_ERROR 1
#define LOG_WARNING 2

//écrit dans le fichier log un message (possibilité de formatage du texte comme printf)
extern int LogWrite(FILE* f,int status,char* format,...); 
#endif