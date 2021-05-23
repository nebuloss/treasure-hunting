#ifndef MAPGRID_H_INCLUDED
#define MAPGRID_H_INCLUDED
#include <stdint.h>
#include <stdbool.h>

#define NB_CHESTS 4
#define NB_POSITION 5

typedef struct{ //structure pour les coordonnées x et y
    int32_t x,y;
}coords;

#define NULL_COORDS (coords){0,0}
#define INVALID_COORDS (coords){-1,-1}

typedef uint8_t map_direction; 

typedef struct{ //struct pour les information du chemin
    uint32_t lenght,total_lenght;
    map_direction path[];
}map_path;

typedef struct grid_case grid_case;

struct grid_case{ //struct pour les informations liée a la case de la grille
    uint32_t f,g;
    uint8_t value,parent;
    bool is_marked,is_close; 
};

typedef struct{ //struct pour pour avoir les informations consernant la map
    uint32_t width,height;
    grid_case** map;
}map_grid;

typedef struct{ //struct pour les informtation consernant la positions des coffres et personnage
    coords pos[NB_CHESTS+1];
    uint8_t tresure,character;
}map_position;

typedef struct{ // struct pour avoir le chemin du charactere
  coords start;
  uint16_t nbpath;
  bool find_tresure;
  map_path *l[NB_CHESTS];
}character_path;

extern map_grid* InitMapGrid(map_grid* m,uint32_t width,uint32_t height); //initialisation des grilles de la map

extern map_grid* CreateMapGrid(uint32_t width,uint32_t height); // crée les grilles de la map

extern void FreeMapGrid(map_grid* m,bool free_ptr); // liberer les grilles de la map

extern grid_case* GetMapCase(map_grid* m,uint32_t x,uint32_t y); // avoir les information de la case choisie

extern void PrintMapPath(map_path* p); //avoir check si le chemin crée est valide (pour les test)

extern character_path* EvaluateCharacterPath(map_position* m,map_grid* mg); //Pour savoir le chemin à choisir entre les differentes coffres

extern uint8_t MapDirectionToLenght(map_direction d); //On extrait le poids de la direction

extern map_path* AStar(map_grid* m,uint32_t x_start,uint32_t y_start,uint32_t x_end,uint32_t y_end); // la fonction A*

extern void FreeCharacterPath(character_path* p);// liberer le chemin du personnage

extern coords MapDirectionToCoord(map_direction d);//extraire d'une directions les coordonnées relatif de la case suivante

#endif