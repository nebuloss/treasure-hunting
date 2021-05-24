#ifndef DISPLAY_H_INCLUDED
#define DISPLAY_H_INCLUDED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "mapgrid.h"
#include "loader.h"

typedef struct{ //structures qui stocke les couleurs usuelles
    SDL_Color red,green,blue,yellow,magenta,cyan,black,white,pink,brown,grey;
}SDL_STD_COLOR;


typedef enum{
    NONE_EVENT,INVALID_EVENT,VALID_EVENT //énumération des évènements
}event_type;

//cette structure est utilisé pour l'affichage des déplacements du personnage
typedef struct{
    map_position pos; 
    SDL_Texture *map,*character;
    SDL_Surface *tresure,*void_chest;
    SDL_Rect text_rect,map_rect;
    TTF_Font* text_font;
}map_package; 

extern SDL_STD_COLOR colors; //partage les couleurs

//filtre les évènements
extern event_type FilterEvents(SDL_Event* e);

//écrit du texte dans le renderer
extern char* WriteText(TTF_Font* font,SDL_Color foreground,coords c,SDL_Renderer* render,char* format,...);

//menu permettant la sélection d'un emplacement
extern coords SelectLocation(SDL_Renderer* renderer,SDL_Texture* background_texture,coords background_location,SDL_Texture* valid_texture,SDL_Texture* invalid_texture,map_grid* mg);

//permet de sélectionner tous les emplacements necessaires pour calculer le chemin
extern map_package* MenuAskForPositions(SDL_Renderer* renderer,map_objects* mo,mainconf* cf);

//affiche une image sur le renderer
extern char* DisplayImage(char* filename,SDL_Renderer* renderer,SDL_Rect* r);

//libère les textures utilisées
extern void FreeMapPackage(map_package* mp);

//affiche les déplacements du personnage
extern bool CharacterSearchTresure(SDL_Renderer* renderer,map_package* mp,character_path* cp);

//attend un évènement
extern bool WaitKeyEvent();

//menu d'acceuil
extern void WelcomeMenu(SDL_Renderer* renderer,mainconf* cf);

#endif