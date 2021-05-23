#include "editor.h"
#include "main.h"
#include "configfile2/dictionary.h"
#include <unistd.h>
#include "mapfile.h"
#include "display.h"

static SDL_Renderer* renderer=NULL; //varibales internes à l'éditeur de map
static SDL_Window* window=NULL;
static uint32_t window_width,window_height,map_height,map_width,separation,*arr;
static uint32_t x_pallet,y_pallet,y_pallet_cpy;
static editor_objects current_eo;

void EditorSurfaceToTexture(key* k){ //transforme les surfaces en textures dans le dictionnaire
    SDL_Texture* t;
    if ((t=SDL_CreateTextureFromSurface(renderer,k->value.p))==NULL){
        SDL_FreeSurface(k->value.p);
        DictionaryRemoveElement(current_eo.d,k,false);
        return;
    }
    SDL_FreeSurface(k->value.p);
    k->value.p=t;
}

static void FreeEditorDictionary2(key* k){ //nouvelle fonction de libération de mémoire
    void* ptr;
    if ((ptr=k->value.p)) SDL_DestroyTexture(ptr);
}

bool InitEditor(editor_objects o){
    map_width=o.arr[0];
    map_height=o.arr[1];
    separation=map_height<<5; //on définit les positions importantes dans la fenêtre
    window_height=separation+96;
    window_width=map_width<<5;
    if (SDL_CreateWindowAndRenderer(window_width,window_height,SDL_WINDOW_SHOWN,&window,&renderer)){ //création de la fenêtre et du renderer
        LogWrite(logfile,LOG_ERROR,"Unable to init window or renderer (Editor)");
        return false;
    }
    
    current_eo=o; //initialisation de l'éditeur
    arr=current_eo.arr+2;
    y_pallet=separation+TEXTURE_HEIGHT;
    ForEachKey(current_eo.d,EditorSurfaceToTexture);
    current_eo.d->func=FreeEditorDictionary2;
    SDL_SetWindowTitle(window,"Map Editor");
    
    return true;
}

void CloseEditor(){
    if (renderer) SDL_DestroyRenderer(renderer); //libère toutes les ressources
    if (window) SDL_DestroyWindow(window);
    FreeEditorObject(current_eo);
}

static void Dictionary_PrintTexture(key *k){ //affiche la pallette de texture
    SDL_Rect r = {x_pallet,y_pallet_cpy,32,32};
    x_pallet +=32;
    if (x_pallet==window_width){
        y_pallet_cpy+=32;
        x_pallet=0;
    }

    SDL_RenderCopy(renderer,k->value.p,NULL,&r);
}

void EditorEditMap(){
    key* k;
    SDL_Rect r={0,0,window_width,4};
    SDL_Texture* t=NULL;
    uint32_t ax,ay;
    int x,y;
    int64_t id;

    SDL_SetRenderDrawColor(renderer,colors.grey.r,colors.grey.g,colors.grey.b,colors.grey.a);

    for (;r.y<separation;r.y+=TEXTURE_HEIGHT){ //affichage du quadrillage
        SDL_RenderFillRect(renderer,&r);
    }
    r=(SDL_Rect){0,0,4,separation};
    for (;r.x<window_width;r.x+=TEXTURE_WIDTH){
        SDL_RenderFillRect(renderer,&r);
    }
    r=(SDL_Rect){0,separation,window_width,TEXTURE_HEIGHT};
    SDL_RenderFillRect(renderer,&r);
    x_pallet=0;
    y_pallet_cpy=y_pallet;
    ForEachKey(current_eo.d,Dictionary_PrintTexture); //palette

    for(uint32_t i=0,*map=arr,value;i<map_height;i++){ //affichage de la map
        for(uint32_t j=0;j<map_width;j++,map++){
            value=*map;
            k=DictionaryGetKey(current_eo.d,DKV_I32(value));
            if(k){
                r=(SDL_Rect){j<<5,i<<5,TEXTURE_WIDTH,TEXTURE_HEIGHT};
                SDL_RenderCopy(renderer,k->value.p,NULL,&r);
            }
        }
    }
    SDL_RenderPresent(renderer);

    while (1){ 
        if (!WaitKeyEvent()) return;
        SDL_GetMouseState(&x,&y);

        r.x=x&TEXTURE_MASK;
        r.y=y&TEXTURE_MASK;
        ax=r.x>>5;
        
        if (y>y_pallet){ //sélection d'une texture dans la pallette
            ay=(r.y-y_pallet)>>5;
            k=DictionaryGetKeyWithIndex(current_eo.d,ax+map_width*ay);
            if(k){
                t=k->value.p;
                id=k->name.i64;
            }
        }else if (t && r.y<separation){ //remplacement d'une texture dans la map
            ay=r.y>>5;
            SDL_RenderCopy(renderer,t,NULL,&r);
            SDL_RenderPresent(renderer);
            arr[ax+map_width*ay]=id;
        }
        SDL_FlushEvent(SDL_MOUSEBUTTONDOWN);    
    }
}

void EditorSaveMap(mainconf* cf){ //sauvegarde la map
    chdir(cf->map_folder);
    WriteMap(cf->map_binary,current_eo.arr);
    chdir(cf->base_directory);
}
