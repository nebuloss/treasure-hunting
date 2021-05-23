#ifndef LOADER_H_INCLUDED
#define LOADER_H_INCLUDED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include "main.h"
#include "mapgrid.h"
#include "configfile2/configfile.h"

//pour le chargement de main.conf
typedef struct{
    TTF_Font *title_font,*text_font,*default_font;
    char *map_folder,*map_conf,*map_binary,*base_directory,*chest,*character,*welcome_background;
    SDL_Surface *icon;
}mainconf;

//chargements de la map
typedef struct{
    map_grid mg;
    SDL_Surface *map,*chest,*character,*tresure,*void_chest,*error;
}map_objects;

//structure pour l'éditeur de map
typedef struct{
    uint32_t* arr;
    dictionary* d;
}editor_objects;

//chargement du fichier main.conf
extern mainconf* LoadConf(key* k);

//chargement des objets
extern map_objects* LoadObjects(mainconf* cf);

//libère le fichier conf
extern void FreeMainConf(mainconf* cf);

//libère les objets
extern void FreeMapObjects(map_objects* mo);

//libère les ressources de l'éditeur
extern void FreeEditorObject(editor_objects o);

//charge les ressources de l'éditeur de map
extern editor_objects LoadEditor(mainconf * cf);

#endif