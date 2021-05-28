#include "loader.h"
#include "logfile.h"
#include <unistd.h>
#include "mapfile.h"
#include "display.h"

#define MAP_DEFAULT_WIDTH 40 
#define MAP_DEFAULT_HEIGHT 20

typedef struct{ //chaque texture a un nom, une image , un id et une vitesse de parcours
    char *name;
    SDL_Surface* surface;
    uint64_t lenght;
}texture_info;

#define TEXTURE_INFO(i) ((texture_info*)i)

static void ExtractTexture(key* k,dictionary* d){ //traite chaque texture
    texture_info* ti;
    int64_t *ptr,id;
    uint64_t lenght;
    char* filename;
    SDL_Surface* s=NULL;

    if ((ptr=CfgSearchIntegerValue(k,NULL,"id"))==NULL){ //charge la clé id
        LogWrite(logfile,LOG_WARNING,"failed to load texture \"%s\". Cannot find any id",k->name.s);
        return;
    }
    id=*ptr;

    if ((filename=CfgSearchStringValue(k,NULL,"file"))==NULL){ //charge la clé file
        LogWrite(logfile,LOG_WARNING,"failed to load texture \"%s\". Cannot find any file",k->name.s);
        return;
    }

    if ((s=SDL_LoadBMP(filename))==NULL){
        LogWrite(logfile,LOG_WARNING,"failed to load texture \"%s\". Cannot find bitmap image \"%s\"",k->name.s,filename);
        return;
    }

    ti=malloc(sizeof(texture_info));
    ti->name=k->name.s;
    ti->surface=s;

    if ((ptr=CfgSearchIntegerValue(k,NULL,"speed"))){ 
        lenght=*(uint64_t*)ptr;
        if (lenght){
            if (lenght>5) lenght=5;
            lenght=6-lenght;
        }
        ti->lenght=lenght;

    }
    else ti->lenght=0;
    DictionaryAddKey(d,DKV_I64(id),DKV_POINTER(ti));
}

static void FreetextureDict(key* k){ //fonction qui libère chque élément du dictionnaire
    texture_info* ti=k->value.p;
    SDL_Surface* s;

    if (ti){
        s=ti->surface;
        if (s) SDL_FreeSurface(s);
        free(ti);
    }
}

void FreeMapObjects(map_objects* mo){
    void* ptr;
    SDL_Surface* lptr[]={mo->map,mo->character,mo->chest,mo->void_chest};
    for (int i=0;i<4;i++) if ((ptr=lptr[i])) SDL_FreeSurface(ptr); 
    
    if ((ptr=&mo->mg)) FreeMapGrid(ptr,false);
    free(mo);
}

map_objects* LoadObjects(mainconf* cf){
    uint32_t width,height,value,total_width,total_height;
    uint32_t *arr,*arrcpy,*end_arr;
    SDL_Surface* s;
    map_grid* mg;
    SDL_Rect r;
    grid_case* c;
    texture_info* ti;
    key* k;
    char *temp_string,*temp_string2;

    char* key_names[]={"character","chest","tresure","void_chest","error"};
    char* default_values[]={"character.bmp","chest.bmp","tresure.bmp","void.bmp","error.bmp"};

    if (chdir(cf->map_folder)==-1){
        LogWrite(logfile,LOG_WARNING,"Cannot open map directory: \"%s\". Using base directory instead: %s",cf->map_folder,cf->base_directory);
    }

    arr=ReadMap(cf->map_binary);

    if (arr==NULL){
        LogWrite(logfile,LOG_ERROR,"Cannot open map binary file: \"%s\"",cf->map_binary);
        chdir(cf->base_directory);
        return NULL;
    }

    key* conf=CfgOpen(cf->map_conf);

    if (conf==NULL){
        LogWrite(logfile,LOG_ERROR,"Cannot open map configuration file: \"%s\"",cf->map_conf);
        chdir(cf->base_directory);
        return NULL;
    }

    dictionary* d=NewSpecialDictionary(FreetextureDict); //dictionnaire de texture
    ForEachKeyFilteredValue(conf->value.p,CF_DIRECTORY,ExtractTexture,d);

    arrcpy=arr;
    width=*arrcpy++;
    height=*arrcpy++;
    end_arr=arrcpy+(width*height);
    total_width=width*TEXTURE_WIDTH;
    total_height=height*TEXTURE_HEIGHT;


    if ((s=SDL_CreateRGBSurface(0,total_width,total_height,32,0,0,0,0))==NULL){ //créé une surface vide de la taille de la map
        LogWrite(logfile,LOG_ERROR,"Failed to create a surface with a size of %uX%u",total_width,total_height);
        chdir(cf->base_directory);
        FreeDictionary(d);
        CfgClose(conf);
        free(arr);
        return NULL;
    }

    map_objects* mo=calloc(1,sizeof(map_objects)); 
    SDL_Surface** results[]={&mo->character,&mo->chest,&mo->tresure,&mo->void_chest,&mo->error};

    mo->map=s;

    if ((mg=InitMapGrid(&mo->mg,width,height))==NULL){ //initialise la grille utilisé par A star
        LogWrite(logfile,LOG_ERROR,"Failed to create a map grid with a size of %uX%u",width,height);
        FreeMapObjects(mo);
        chdir(cf->base_directory);
        FreeDictionary(d);
        CfgClose(conf);
        free(arr);
        return NULL;
    }

    c=mg->map[1]+1;

    r=(SDL_Rect){0,0,TEXTURE_WIDTH,TEXTURE_HEIGHT};

    while (arrcpy<end_arr){ //on lit le tableau d'id de texture
        value=*arrcpy;
        k=DictionaryGetKey(d,DKV_I64(value)); //on cherche la texture correspondante
        if (r.x==total_width){ //si on arrive au bout d'une ligne on passe à la ligne suivante
            r.y+=TEXTURE_HEIGHT;
            r.x=0;
            c+=2;
        }
        if (k){
            ti=TEXTURE_INFO(k->value.p);
            SDL_BlitSurface(ti->surface,NULL,s,&r); //copie la surface de la texture dans la grande surface de la map
            c->value=ti->lenght;
        }
        else{
            LogWrite(logfile,LOG_WARNING,"Texture id %u missing for coordinates x=%u y=%u",value,r.x,r.y); //en cas de texture non trouvée
            c->value=0;
        }
        arrcpy++;
        c++;
        r.x+=TEXTURE_WIDTH;
    }

    FreeDictionary(d);
    free(arr);

    for (int i=0;i<5;i++){ //on charge quelques images supplémentaire (coffre vide, personnage, trésor...)
        
        if ((temp_string=CfgSearchStringValue(conf,NULL,(temp_string2=key_names[i])))==NULL){
            temp_string=default_values[i];
            LogWrite(logfile,LOG_INFO,"Using default name for %s image: \"%s\"",temp_string2,temp_string);
        }
        if ((s=SDL_LoadBMP(temp_string))==NULL){
            LogWrite(logfile,LOG_WARNING,"Cannot open %s image: \"%s\"",temp_string2,temp_string);
            s=SDL_CreateRGBSurface(0,TEXTURE_WIDTH,TEXTURE_HEIGHT,32,0,0,0,0);
        }
        SDL_SetColorKey(s,1,SDL_MapRGB(s->format,colors.magenta.r,colors.magenta.g,colors.magenta.b));
        *results[i]=s;
    }

    CfgClose(conf);

    chdir(cf->base_directory);

    return mo;

}

mainconf* LoadConf(key* k){ //chargement du fichier main.conf
    key* current_dir;
    char* temp_string;
    TTF_Font *temp_font,*default_font;
    SDL_Surface* s;
    char* default_font_filename="assets/fonts/default.ttf";

    mainconf* cf=malloc(sizeof(mainconf));

    if ((default_font=TTF_OpenFont(default_font_filename,24))==NULL){ //chargement des polices
        LogWrite(logfile,LOG_WARNING,"Cannot load default font: \"%s\" missing",default_font_filename);
    }
    cf->default_font=default_font;

    current_dir=CfgCD(k,NULL,"fonts");
    if (current_dir==NULL) current_dir=k;

    temp_string=CfgSearchStringValue(current_dir,NULL,"text"); //police pour le texte
    if (temp_string==NULL || (temp_font=TTF_OpenFont(temp_string,24))==NULL){
        LogWrite(logfile,LOG_WARNING,"Cannot load text font: \"%s\" missing. Replacing font by \"%s\"",temp_string,default_font_filename);
        cf->text_font=default_font;
    }
    else cf->text_font=temp_font;

    temp_string=CfgSearchStringValue(current_dir,NULL,"title"); //police pour les titres
    if (temp_string==NULL || (temp_font=TTF_OpenFont(temp_string,60))==NULL){
        LogWrite(logfile,LOG_WARNING,"Cannot load title font: \"%s\" missing. Replacing font by \"%s\"",temp_string,default_font_filename);
        cf->title_font=default_font;
    }
    else cf->title_font=temp_font;

    cf->base_directory=getcwd(NULL,0);
    
    current_dir=CfgCD(k,NULL,"map");
    if (current_dir==NULL) current_dir=k;

    if ((temp_string=CfgSearchStringValue(current_dir,NULL,"folder"))==NULL){ //on charge les informations sur la map (ici le dossier de la map)
        temp_string="assets/map";
        LogWrite(logfile,LOG_INFO,"Using default map folder path: \"%s\"",temp_string);
    }
    cf->map_folder=temp_string;

    if ((temp_string=CfgSearchStringValue(current_dir,NULL,"conf"))==NULL){ //nom du fichier conf dans le dossier map
        temp_string="map.conf";
        LogWrite(logfile,LOG_INFO,"Using default map configuration file name: \"%s\"",temp_string);
    }
    cf->map_conf=temp_string;

    if ((temp_string=CfgSearchStringValue(current_dir,NULL,"data"))==NULL){ //fichier contenant la représentation de la map (avec id de textures)
        temp_string="map";
        LogWrite(logfile,LOG_INFO,"Using default map binary file name: \"%s\"",temp_string);
    }
    cf->map_binary=temp_string;

    if ((temp_string=CfgSearchStringValue(k,NULL,"background"))==NULL){ //arrière plan du menu principal
        temp_string="assets/img.bmp";
        LogWrite(logfile,LOG_INFO,"Using default welcome image path: \"%s\"",temp_string);
    }
    cf->welcome_background=temp_string;

    if ((temp_string=CfgSearchStringValue(k,NULL,"icon"))==NULL){ //icone de la fenêtre
        temp_string="assets/icon.bmp";
        LogWrite(logfile,LOG_INFO,"Using default welcome icon path: \"%s\"",temp_string);
    }

    if ((s=SDL_LoadBMP(temp_string))==NULL){
        LogWrite(logfile,LOG_WARNING,"Cannot open icon image: \"%s\"",temp_string);
        s=SDL_CreateRGBSurface(0,TEXTURE_WIDTH,TEXTURE_HEIGHT,32,0,0,0,0);
    }
    SDL_SetColorKey(s,1,SDL_MapRGB(s->format,colors.magenta.r,colors.magenta.g,colors.magenta.b));
    cf->icon=s;

    return cf;
}

void FreeMainConf(mainconf* cf){ //libère les ressources du fichier main.conf
    void *ptr1,*ptr2;
    if ((ptr1=cf->default_font)){
        if ((ptr2=cf->text_font) && ptr2!=ptr1) TTF_CloseFont(ptr2); //libère les polices
        if ((ptr2=cf->title_font) && ptr2!=ptr1) TTF_CloseFont(ptr2);
        
    }else{
        if ((ptr2=cf->title_font)) TTF_CloseFont(ptr2);
        if ((ptr2=cf->text_font)) TTF_CloseFont(ptr2);
    }
    if ((ptr1=cf->icon)) SDL_FreeSurface(ptr1);
    free(cf->base_directory);
    free(cf);
}

// EXTRA MAP EDITOR

//dans ce cas on charge seulement certaines partie du fichier conf de la map (pas besoins de speed)

static void FreeEditorDictionary(key* k){
    void* ptr;
    if ((ptr=k->value.p)) SDL_FreeSurface(ptr);
}

//on extrait les clées qui nous intéressent
static void ExtractEditorDictionary(key* k,dictionary* d){
    int64_t id,*ptr;
    SDL_Surface* s;
    char* filename;


    if ((ptr=CfgSearchIntegerValue(k,NULL,"id"))==NULL){
        LogWrite(logfile,LOG_WARNING,"failed to load texture \"%s\". Cannot find any id",k->name.s);
        return;
    }
    id=*ptr;

    if ((filename=CfgSearchStringValue(k,NULL,"file"))==NULL){
        LogWrite(logfile,LOG_WARNING,"failed to load texture \"%s\". Cannot find any file",k->name.s);
        return;
    }

    if ((s=SDL_LoadBMP(filename))==NULL){
        LogWrite(logfile,LOG_WARNING,"failed to load texture \"%s\". Cannot find bitmap image \"%s\"",k->name.s,filename);
        return;
    }
    
    DictionaryAddKey(d,DKV_I64(id),DKV_POINTER(s));
}

//on charge ici seulement le dictionnaire de textures ainsi que le tableau des id de textures.
editor_objects LoadEditor(mainconf * cf){
    editor_objects o;

    if (chdir(cf->map_folder)==-1){
        LogWrite(logfile,LOG_WARNING,"Cannot open map directory: \"%s\". Using base directory instead: %s",cf->map_folder,cf->base_directory);
    }

    o.arr=ReadMap(cf->map_binary);

    if (o.arr==NULL){
        LogWrite(logfile,LOG_ERROR,"Cannot open map binary file: \"%s\"",cf->map_binary);
        o.arr=GenerateMap(MAP_DEFAULT_WIDTH,MAP_DEFAULT_HEIGHT);
    }

    key* conf=CfgOpen(cf->map_conf);
    o.d=NewSpecialDictionary(FreeEditorDictionary);

    if (conf==NULL) LogWrite(logfile,LOG_ERROR,"Cannot open map configuration file: \"%s\"",cf->map_conf);
    else ForEachKeyFilteredValue(conf->value.p,CF_DIRECTORY,ExtractEditorDictionary,o.d);

    chdir(cf->base_directory);

    CfgClose(conf);
    return o;
}

//libération du dictionnaire et du tableau
void FreeEditorObject(editor_objects o){
    if (o.arr) free(o.arr);
    if (o.d) FreeDictionary(o.d);
}
