#ifndef EDITOR_H_INCLUDED
#define EDITOR_H_INCLUDED
#include <SDL2/SDL.h>
#include "loader.h"
#include <stdbool.h>

//initialise l'éditeur de map
extern bool InitEditor(editor_objects o);

//ferme l'éditeur de map
extern void CloseEditor();

//savegarde les modifications dans la map
extern void EditorSaveMap(mainconf* cf);

//édition de la map
extern void EditorEditMap();

#endif