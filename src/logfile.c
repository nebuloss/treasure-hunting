#include "logfile.h"
#include <time.h>
#include <stdarg.h>

static const char* log_status[]={
    "INFO",
    "ERROR",
    "WARNING"
};

int LogWrite(FILE* f,int status,char* format,...){ //permet d'écrire dans le fichier log
    time_t t;
    struct tm *now;
    va_list v;

    if (f!=NULL){
        t=time(&t); //on récupère l'heure
        now=localtime(&t);
        fprintf(f,"%4d-%02d-%02d %02d:%02d:%02d %s: ",now->tm_year+1900,now->tm_mon+1,now->tm_mday,now->tm_hour,now->tm_min,now->tm_sec,log_status[status]);
        va_start(v,format); //utilisation des va_args pour gérer les arguments à nombres variables
        vfprintf(f,format,v);
        va_end(v); //libération des ressources des va_args
        putc('\n',f); //retour à la ligne
        return 0;
    }
    return -1;
}