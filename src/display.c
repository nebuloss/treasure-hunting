#include "display.h"
#include "main.h"

SDL_STD_COLOR colors={ //on définit quelques couleurs utiles
    .red={0xFF,0x00,0x00,0xFF},
    .green={0x00,0xFF,0x00,0xFF},
    .blue={0x00,0x00,0xFF,0xFF},
    .yellow={0xFF,0xFF,0x00,0xFF},
    .magenta={0xFF,0x00,0xFF,0xFF},
    .cyan={0x00,0xFF,0xFF,0xFF},
    .black={0x00,0x00,0x00,0xFF},
    .white={0xFF,0xFF,0xFF,0xFF},
    .pink={0xFF,0xFF,0xFF,0xFF},
    .brown={0xFF,0xFF,0xFF,0xFF},
    .grey={0xAF,0xAF,0xAF,0xFF}
};

event_type FilterEvents(SDL_Event* e){ //filtre les évènements. Voir Readme pour les types d'évènements
    uint32_t type=e->type;
    SDL_Keycode c;

    if (type==SDL_WINDOWEVENT && e->window.event == SDL_WINDOWEVENT_CLOSE) return INVALID_EVENT; //quitter la fenêtre
    else if (type==SDL_MOUSEBUTTONDOWN) return VALID_EVENT; //clic souris
    else if (type==SDL_KEYDOWN){
        c=e->key.keysym.sym;
        if (c==SDLK_ESCAPE) return INVALID_EVENT; //échap
        else if (c==SDLK_RETURN) return VALID_EVENT; //entrer
    } 
    return NONE_EVENT; //aucun évènement interresant
}

char* DisplayImage(char* filename,SDL_Renderer* renderer,SDL_Rect* r){
    SDL_Surface* s=SDL_LoadBMP(filename); //charge le bitmap dans une surface
    if (s==NULL) return NULL; 
    SDL_Texture* t=SDL_CreateTextureFromSurface(renderer,s); //créé une texture
    SDL_RenderCopy(renderer,t,NULL,r); //copie la texture dans le renderer
    SDL_DestroyTexture(t); //libère toutes les ressources utilisées
    SDL_FreeSurface(s);
    return filename; //valeur de retour à titre indicative
}

void FillRect(SDL_Rect *r,SDL_Color c,SDL_Renderer* render){ //affiche un rectangle dans le renderer
    SDL_SetRenderDrawColor(render,c.r,c.g,c.b,c.a);
    SDL_RenderFillRect(render,r);
}

char* WriteText(TTF_Font* font,SDL_Color foreground,coords c,SDL_Renderer* render,char* format,...){ //affiche du texte dans le renderer
    const size_t buffer_size=1024; //taille du buffer allouée
    va_list va;

    if (font==NULL || render==NULL || format==NULL) return NULL;
    va_start(va,format);

    char* buffer=malloc(buffer_size);
    vsnprintf(buffer,buffer_size,format,va); //accepte le formatage du texte (comme printf)
    va_end(va);
    
    SDL_Surface* s = TTF_RenderText_Solid(font,buffer,foreground); //affiche le texte dans une surface

    SDL_Rect r1={c.x,c.y,s->w,s->h};
    SDL_Texture* t=SDL_CreateTextureFromSurface(render,s); //créé une texture

    SDL_RenderCopy(render,t,NULL,&r1); //copie la texture dans le renderer
    SDL_FreeSurface(s);
    SDL_DestroyTexture(t); //libérations des ressources
    free(buffer);
    return format;
}

coords SelectLocation(SDL_Renderer* renderer,SDL_Texture* background_texture,coords background_location,SDL_Texture* valid_texture,SDL_Texture* invalid_texture,map_grid* mg){
    SDL_Event e; //event
    coords c,o;
    const uint32_t mask=TEXTURE_MASK;
    uint32_t width,height,rx,ry,x,y;
    int texture_width,texture_height,status;
    SDL_Texture* current_texture;
    SDL_Rect texture_rect={0,0,TEXTURE_WIDTH,TEXTURE_HEIGHT},dstrect=texture_rect;
    uint16_t value=0;

    if (background_location.x<0) background_location.x=0;
    if (background_location.y<0) background_location.y=0;

    
    SDL_QueryTexture(background_texture,NULL,NULL,&texture_width,&texture_height);
    width=mg->width<<5;
    height=mg->height<<5;
    if (width>texture_width) width=texture_width&mask;
    if (height>texture_height) height=texture_height&mask;

    SDL_Rect current_rect={width-TEXTURE_WIDTH,height-TEXTURE_HEIGHT,TEXTURE_WIDTH,TEXTURE_HEIGHT};
    SDL_Rect current_dst={background_location.x+current_rect.x,background_location.y+current_rect.y,TEXTURE_WIDTH,TEXTURE_HEIGHT};

    
    width--;
    height--;
    
    while (1){
        SDL_GetMouseState(&c.x,&c.y); //coordonnées de la souris

        rx=c.x-background_location.x; ry=c.y-background_location.y;
        if (rx>width) rx=width;
        if (ry>height) ry=height;
        rx&=mask; //coordonnées de la souris dans l'arrière plan
        ry&=mask;
        dstrect.x=rx+background_location.x; dstrect.y=ry+background_location.y; //corrdonnées de la souris dans la fenêtre

        if (current_rect.x!=rx || current_rect.y!=ry){ //si la souris à changée de zone (32x32 pixels)
            o.x=(rx>>5); o.y=(ry>>5); //on actualise la position du sélecteur
            x=o.x+1; y=o.y+1;
            
            SDL_RenderCopy(renderer,background_texture,&current_rect,&current_dst);  //efface le sélecteur de son ancienne position
            if ((value=mg->map[y][x].value)) current_texture=valid_texture; //sélecteur valide
            else current_texture=invalid_texture; //sélecteur invalide

            SDL_RenderCopy(renderer,current_texture,&texture_rect,&dstrect); //copie le sélecteur dans sa nouvelle position
            current_rect.x=rx;
            current_rect.y=ry;
            current_dst.x=dstrect.x;
            current_dst.y=dstrect.y;
            SDL_RenderPresent(renderer); //actualisation de la fenêtre
        }

        if (SDL_PollEvent(&e)){ //on filtre les évènements
            if ((status=FilterEvents(&e))==VALID_EVENT) return o;
            else if (status==INVALID_EVENT) return (coords){-1,-1};
        }
        SDL_Delay(10);
    }
}

void FreeMapPackage(map_package* mp){ 
    void* ptr;
    
    if (!mp) return;
    if ((ptr=mp->map)) SDL_DestroyTexture(ptr); //détruit des textures
    if ((ptr=mp->character)) SDL_DestroyTexture(ptr);

    free(mp);

}

map_package* MenuAskForPositions(SDL_Renderer* renderer,map_objects* mo,mainconf* cf){
    //liste des messages à afficher à l'écran
    char* messages[]={"Veuillez placer le tresor","Placez un second coffre...","Placez un troisieme coffre...","Placez un dernier coffre"};

    map_package* mp=calloc(1,sizeof(map_package));
    coords current_pos,*lc=mp->pos.pos,separation={0,80};

    grid_case** arr=(&mo->mg)->map;

    SDL_Surface *map=mo->map;
    SDL_Texture *map_text=mp->map=SDL_CreateTextureFromSurface(renderer,map); //on initialise les textures à afficher 
    SDL_Texture* valid_text=SDL_CreateTextureFromSurface(renderer,mo->chest);
    SDL_Texture* invalid_text=SDL_CreateTextureFromSurface(renderer,mo->error);
    
    SDL_Surface* chest=mo->chest;
    
    mp->text_rect=(SDL_Rect){separation.x,0,map->w,separation.y};
    mp->map_rect=(SDL_Rect){separation.x,separation.y,map->w,map->h};
    SDL_Rect c={0,0,TEXTURE_WIDTH,TEXTURE_HEIGHT};

    SDL_RenderCopy(renderer,map_text,NULL,&mp->map_rect); //affiche la map
    

    for (int i=0;i<NB_CHESTS;i++){
        FillRect(&mp->text_rect,colors.white,renderer); 
        WriteText(cf->title_font,colors.black,NULL_COORDS,renderer,messages[i]); //affiche un message 
        

        current_pos=SelectLocation(renderer,map_text,separation,valid_text,invalid_text,&mo->mg); //on sélectionne la position
        
        if (current_pos.x==-1){ //en cas d'abandon
            FreeMapPackage(mp);
            SDL_DestroyTexture(valid_text);
            SDL_DestroyTexture(invalid_text);
            return NULL;
        }

        c.x=current_pos.x<<5; 
        c.y=current_pos.y<<5;
        current_pos.x++;
        current_pos.y++;
        lc[i]=current_pos; //ajoute la position sélectionnée dans la structure
        arr[current_pos.y][current_pos.x].value=0; //on ne peut pas sélectionner deux fois le même emplacement
        
        SDL_UpdateTexture(map_text,&c,chest->pixels,chest->pitch);

    }

    FillRect(&mp->text_rect,colors.white,renderer);
    WriteText(cf->title_font,colors.black,NULL_COORDS,renderer,"Placez le personnage"); // sélection du personnage
    
    SDL_Texture* character=mp->character=SDL_CreateTextureFromSurface(renderer,mo->character);
    

    current_pos=SelectLocation(renderer,map_text,separation,character,invalid_text,&mo->mg);

    SDL_DestroyTexture(valid_text); 
    SDL_DestroyTexture(invalid_text);
    
    if (current_pos.x==-1){ //abandon lors de la dernière sélection
        FreeMapPackage(mp);
        return NULL;
    }

    mp->tresure=mo->tresure; //initialise les valeurs du map_package
    mp->void_chest=mo->void_chest;

    for (int i=0;i<NB_CHESTS;i++){ //on remet les case choisis à un état qui peut être parcouru
        arr[lc[i].y][lc[i].x].value=3; 
        lc[i].x--;
        lc[i].y--;
    }
    lc[NB_CHESTS].x=current_pos.x; //position du coffre
    lc[NB_CHESTS].y=current_pos.y;
    mp->pos.tresure=0; //le trésor est la première position
    mp->pos.character=NB_CHESTS; //le coffre est la dernière position dans la liste
    mp->text_font=cf->title_font;

    return mp; // retourne la structure
}

bool WaitKeyEvent(){ //attend que l'utilisateur saisise un évènement qui nous interresse
    SDL_Event e;
    int status;
    while (1){
        SDL_WaitEvent(&e);
        if ((status=FilterEvents(&e))) return status-1; //on filtre les évènements
        SDL_Delay(10);
    }
}

//cette fonction affiche les déplacements
bool CharacterSearchTresure(SDL_Renderer* renderer,map_package* mp,character_path* cp){
    coords current_position,direction;
    uint16_t nbpath;
    uint8_t lenght;

    if (mp==NULL || cp==NULL) return false;
    if ((nbpath=cp->nbpath)==0){ //s'il n'y a pas de déplacements possibles
        FillRect(&mp->text_rect,colors.white,renderer);
        WriteText(mp->text_font,colors.black,NULL_COORDS,renderer,"Le personnage est bloque :-(");
        SDL_RenderPresent(renderer);
        return WaitKeyEvent();
    }

    SDL_Texture *map=mp->map,*character=mp->character;
    SDL_Surface *void_chest=mp->void_chest,*final_surface;
    SDL_Event event;
    current_position=cp->start;

    SDL_Rect src={current_position.x<<5,current_position.y<<5,TEXTURE_WIDTH,TEXTURE_HEIGHT}; //position de départ
    SDL_Rect dst=src;
    dst.y+=mp->map_rect.y;
    dst.x+=mp->map_rect.x;

    map_path **start=cp->l,**end=start+cp->nbpath,*current;

    while (1){ //parcours la liste des chemins
        current=*start;
        for (map_direction *e=current->path,*s=e+current->lenght-1,c;s>=e;s--){ //parcours chaque déplacement dans un chemin
            c=*s;
            direction=MapDirectionToCoord(c); //extrait les coordonnées relatifs (ex: (-1,1),(0,1)...)
            lenght=(MapDirectionToLenght(c)<<3)+10; //extrait le poids de la direction
            direction.x<<=1; //on actualise tout les 2 pixels
            direction.y<<=1;
            for (int j=0;j<TEXTURE_WIDTH;j+=2){ //affiche le déplacement
                SDL_RenderCopy(renderer,map,&src,&dst);
                src.x+=direction.x;
                src.y+=direction.y;
                dst.x+=direction.x;
                dst.y+=direction.y;
                SDL_RenderCopy(renderer,character,NULL,&dst);
                SDL_RenderPresent(renderer);
                
                if (SDL_PollEvent(&event) && FilterEvents(&event)==INVALID_EVENT) return false; //si l'utilisateur souhaite quitter
                
                SDL_Delay(lenght);
            }
        }
        start++;
        if (start>=end) break;
       
        SDL_UpdateTexture(map,&src,void_chest->pixels,void_chest->pitch); //affiche un coffre ouvert vide à la place du coffre fermée
        SDL_RenderCopy(renderer,map,&src,&dst);
        SDL_RenderCopy(renderer,character,NULL,&dst);
        SDL_RenderPresent(renderer);
        SDL_Delay(500); //attend 500ms avant de repartir
    }
    FillRect(&mp->text_rect,colors.white,renderer);
    if (cp->find_tresure){ //fin du trajet: 2 cas possibles
        final_surface=mp->tresure;
        WriteText(mp->text_font,colors.black,NULL_COORDS,renderer,"Le personnage a trouve le tresor!"); //réussite
    }else{
        final_surface=void_chest;
        WriteText(mp->text_font,colors.black,NULL_COORDS,renderer,"Le personnage n'a pas trouve le tresor :-("); //râté
    }
    SDL_UpdateTexture(map,&src,final_surface->pixels,final_surface->pitch);
    SDL_RenderCopy(renderer,map,&src,&dst);
    SDL_RenderCopy(renderer,character,NULL,&dst);
    
    SDL_RenderPresent(renderer);
    return WaitKeyEvent();

}

void WelcomeMenu(SDL_Renderer* renderer,mainconf* cf){ //menu démarrer
    SDL_Rect r = {0,0,640,400};
    coords t = {400,0};
    coords T = {0,370};
    coords b = {180,0};

    DisplayImage(cf->welcome_background,renderer,&r); //affichage de l'image d'arrière plan
    t.y=280;
    WriteText(cf->text_font,colors.green,t,renderer,"Guillaume CHAYE"); //affichage du texte
    t.y=310;
    WriteText(cf->text_font,colors.green,t,renderer,"Anthony OUM");
    t.y=340;
    WriteText(cf->text_font,colors.green,t,renderer,"Thibault VIALLES");
    WriteText(cf->title_font,colors.red,b,renderer,"Bienvenue");
    WriteText(cf->default_font,colors.white,T,renderer,"Appuyez sur entrer pour continuer ou echap pour quitter"); 

    SDL_RenderPresent(renderer);
}