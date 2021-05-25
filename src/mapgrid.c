#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mapgrid.h"
#include "dpa.h"

static uint32_t usqrt(uint32_t x){ //calcule la racine carré entière. Pas besoins d'inclure math.h :-)
    uint32_t res = 0,value;
    uint32_t bit = 1;
    while (bit <= x) bit <<= 2;

    while (bit){
        value=res+bit;
        if (x >= value){
            x -= value;
            res = (res >> 1) + bit;
        } 
        else res >>= 1;
        bit >>= 2;
    }
    return res;
}

static void* Alloc2DArray(size_t width,size_t height,size_t elt_size){
    size_t part1,part2;
    void* buff;

    if ((buff=malloc(width*height*elt_size))==NULL) return NULL;
    part1=sizeof(void*)*height;
    part2=elt_size*width;

    void** arr=malloc(part1);
    for (void **s=arr,**e=((void*)s)+part1,*v=buff;s<e;v+=part2) *s++=v;
    return arr;
}

static void Free2DArray(void* arr){
    if (!arr) return;
    free(*(void**)arr);
    free(arr);
}

map_grid* InitMapGrid(map_grid* m,uint32_t width,uint32_t height){ // on initialise les grilles de la map
    void* ptr;
    if ((ptr=Alloc2DArray(width+2,height+2,sizeof(grid_case)))==NULL) return NULL;
    m->map=ptr;
    m->width=width;
    m->height=height;
    
    return m;
}

map_grid* CreateMapGrid(uint32_t width,uint32_t height){ // on crée les grilles de la map
    map_grid* m=malloc(sizeof(map_grid));
    if (InitMapGrid(m,width,height)==NULL){
        free(m);
        return NULL;
    }
    return m;
}

void FreeMapGrid(map_grid* m,bool free_ptr){ // on libere les grilles de la map
    Free2DArray(m->map);
    if (free_ptr) free(m);
}

grid_case* GetMapCase(map_grid* m,uint32_t x,uint32_t y){ // on a les coords de la case
    if (++x>m->width || ++y>m->height) return NULL;
    return m->map[y]+x;
}

static uint32_t hcost(uint32_t x1,uint32_t y1, uint32_t x2, uint32_t y2){ //calcule du heuristique
    int32_t d1, d2;
    d1=x2-x1;
    d2=y2-y1;
    return usqrt(d1*d1+d2*d2);
}

map_path* AStar(map_grid* m,uint32_t x_start,uint32_t y_start,uint32_t x_end,uint32_t y_end){
    grid_case * start_case, *end_case, *current, *c, *min,*buff;
    dpa *O;
    map_path* path;
    uint32_t g,h,min_value,f,path_lenght,total_lenght;
    uint16_t current_lenght;
    int32_t gap,width,x,y;
    void ** min_adress, **temp_end,*ptr;

    if (m->map == NULL) return NULL; // S'il n'y as map alors retourne NULL
    start_case = GetMapCase(m,x_start,y_start); // start_case prend les coords de depart
    if (start_case == NULL) return NULL; // s'il start_case NULL alors retourne NULL
    end_case = GetMapCase(m,x_end,y_end); // end_case les prend les coords de fin
    if (end_case == NULL || start_case==end_case || !start_case->value || !end_case->value ) return NULL; // s'il end_case NULL alors retourne NULL
    
    width= m->width+2; 
    
    x=++x_start;
    y=++y_start;
    x_end++;
    y_end++;
    buff=*m->map;
      
    int relative_adress[8]={-width-1,-width,-width+1,1,width+1,width,width-1,-1}; //Permet de faire visiter les voisins du noeuds courant
    int relative_x[8]={-1,0,1,1,1,0,-1,-1};
    int relative_y[8]={-1,-1,-1,0,1,1,1,0};

    current=start_case;  
    current->is_marked=true; // on marque le current comme etant parcourue
    
    path_lenght=hcost(x_start,y_start,x_end,y_end)<<1; //path_lenght est egale a heuristique calculer grace a la fonction hcost
    O=CreateDPA(path_lenght<<3); //Liste Open prend la taille path_lenght*8
    
    do{
        current->is_close=true; //le current marquer comme etant visité
        g=current->value+current->g; //la valeur est egale a la valeur du current
        for(int i=0;i<8;i++){ //On visite les voisins
            c=current+relative_adress[i];
            if(c->value && c->is_close==false){ // si la casse est franchissage et qu'elle n'est pas dans close
                if (c->is_marked==false){  // si noeud non marque
                    c->parent=i; // lie a un parent
                    c->is_marked=true; //les noeuds sont marquer comme etant parcourue
                    c->g=g;//on copie le g cost
                    
                    c->f=g+hcost(x+relative_x[i],y+relative_y[i],x_end,y_end); // on calcule le f de chaque voisin
                    DPAInsertElement(O,c); // on les ajoute dans la liste Open
                }
                else{ // s'il a deja etait marquer et non dans close
                    gap=g-c->g;
                    if (gap<0){ //on regarde s'il le g cost est inferieur a celui actuel
                        c->g+=gap;
                        c->f+=gap;
                        c->parent=i; //on le change de parent
                    } 
                }          
            }
        }
        if(O->current_size==0){ // condition fin si plus de voisins a visiter
            DPAFree(O,true); // on libere la liste Open
            return NULL; //retourne NULL
        }
        //la deuxieme etape consiste a determine la case minimal en f cost de la liste OPen
        for(min_adress=O->base_ptr,temp_end=O->current_ptr;!(*min_adress);min_adress++); // on trouve la premiere case de la liste Open
        // on la considere comme min par defaut
        min=*min_adress;
        min_value=min->f;

        for(void** temp_start=min_adress+1; temp_start<temp_end; temp_start++){ //permet de trouver le noeud de la liste Open avec le plus petit f en parcourant la liste Open
            if((c=*temp_start)){
                if((f=c->f)<min_value){
                    min_adress=temp_start;
                    min=c;
                    min_value=f;
                }else if(f==min_value && c->g>min->g){
                    min_adress=temp_start;
                    min=c;
                    min_value=f;
                }
            }
        }
        
        DPARemoveElement(O,min_adress); // on enleve le noeuds choisie prealablement dans la liste Open
        current=min; // on parcoure pour le suivant
        gap=current-buff;
        x=gap%width;
        y=gap/width;
        
    }while(current!=end_case);//temps que current pas egale a la case de fin alors on reboucle
    DPAFree(O,true); // on libere la liste Open a la fin

    //derniere etape reconstituer le chemin ( le chemin retourner est une liste dans le sens inverse. on parcoura la liste dans le sens inverse pour afficher le chemin.)
    if (!(path=malloc(sizeof(map_path)+sizeof(map_direction)*path_lenght))) return NULL; //on alloue le nécessaire pour path

    total_lenght=0; // init de la longueur total a 0
    for(h=0;current!=start_case;h++){    // temps que h different de start_case alors on aggremente 
        if (h==path_lenght){ // pour reallouer plus de case dans path si on arrive a la limite
            path_lenght<<=1; // path*2
            if (!(ptr=realloc(path,sizeof(map_path)+path_lenght*sizeof(map_direction)))){
                free(path);
                return ptr;
            } // retourne un chemin NULL
            path=ptr;
        }
        gap=current->parent; //on recupere le parents
        x=relative_x[gap]+1; //on prepare empaquetage pour les directions
        y=relative_y[gap]+1;
        current= current-relative_adress[gap]; // on trouve l'adresse du parents
        current_lenght=current->value;
        total_lenght+=current_lenght; //incrémente total_lenght avec le poids du chemin actuel
        
        path->path[h]=(x<<2)|y|(current_lenght<<4);// on stock la direction, le poids de la direction   
    }
    path->lenght=h;
    path->total_lenght=total_lenght;
    return path;// on retourne Path
}

coords MapDirectionToCoord(map_direction d){ //extraire d'une directions les coordonnées relatif de la case suivante
    return (coords){((d>>2)&0b11)-1,(d&0b11)-1};
}

uint8_t MapDirectionToLenght(map_direction d){ //On extrait le poids de la direction
    return d>>4;
}

void PrintMapPath(map_path* p){ //Fonction a titre utilitaire pour savoir si le chemin marche
    if (!p){
        puts("no path available");
        return;
    }

    char *messages[]={"UPLEFT","LEFT","DOWNLEFT","ERROR","UP","NONE","DOWN","ERROR","UPRIGHT","RIGHT","DOWNRIGHT","ERROR"};
    int32_t lenght=p->lenght,value;
    coords c;

    for (uint8_t* start=p->path+lenght-1,*end=p->path-1;start!=end;start--){
        value=*start&0b1111;
        if (value>=12){
            printf("unknown direction %d\n",value);
            continue;
        }
        c=MapDirectionToCoord(value);
        printf("%s %d %d\n",messages[value],c.x,c.y);
    }
}

static void ResetMapGrid(map_grid* mg){ //Pour reset les valeurs de la grille de la map
    if (mg==NULL || mg->map==NULL) return;
    size_t total_size=(mg->width+2)*(mg->height+2);
    for (grid_case* start=*mg->map,*end=start+total_size;start!=end;start++){
        start->is_marked=false;
        start->is_close=false;
        start->g=0;
    }
}

void FreeCharacterPath(character_path* p){ // liberer le chemin du personnage
    if (!p) return;
    for (map_path **start=p->l,**end=start+p->nbpath;start<end;start++) free(*start);
    free(p);
}

character_path* EvaluateCharacterPath(map_position* m,map_grid* mg){ //Pour savoir le chemin a choisir entre les differentes coffres
    map_path *min_path,*current_path;
    coords* pts=m->pos,current;
    uint32_t min_lenght,current_lenght,k,i;
    character_path* cp=malloc(sizeof(character_path));

    if(!m) return NULL;

    bool marked[NB_POSITION]={false,false,false,false,false}; //Pour savoir si les positions on deja etait choisi ou non
    k=m->character; // on part des coordonnée personnage
    cp->start=pts[k]; //les coordonnées de la positions de depart
    cp->find_tresure=true; //si on considere le coffre comme trouver par defaut 
    
    for(i=0;k!=m->tresure;i++){ //si la position actuel n'est pas egale au tresor alors on continue
        current=pts[k]; //on recup les coordonnées actuel
        min_path=NULL;
        marked[k]=true;
        for(int j=0;j<NB_POSITION;j++){ //on cherche le point qui a la plus petite distance par rapport a la position actuel
            
            if (marked[j]) continue; //si la positions a deja etait marquer alors on continue
            
            ResetMapGrid(mg); //on reset les valeurs de la grille de la map
            current_path=AStar(mg,current.x,current.y,pts[j].x,pts[j].y); //on calcul le chemin entre le point actuel et un autre point a partir de la fonction Astar
             
            if (!current_path) continue; // si le chemin calculé est NULL on retourne a la boucle while
            
            current_lenght=current_path->total_lenght; //current_lenght est egale a la longueurs total
            if(min_path==NULL){ //s'il n'y a pas de min on considere le chemin calculé précédemment comme etant le plus petit
                min_path=current_path; 
                min_lenght=current_lenght;
                k=j;
            }else if(current_lenght<min_lenght){ // si le chemin calculer précédemment est inferieur au min_lenght
                free(min_path); // on libere le chemin pris auparavant 
                k=j;
                min_path=current_path; // on le considere comme le nouveau min_path
            }
            else free(current_path); //si autre on libere le chemin précédemment calculer
        }
        if(!min_path){ //s'il n'y a pas de chemin min restant alors on ne pourras pas trouver le tresor
            cp->find_tresure=false;
            break;
        }
        cp->l[i]=min_path; //on met le min_path dans la liste les deplacement optimal
    }
    cp->nbpath=i;
    
    return cp; // on retourne la liste crée avec les deplacements optimal
}