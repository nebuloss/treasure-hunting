#ifndef DPA_H_INCLUDED
#define DPA_H_INCLUDED
#include <stdint.h>
#include <stdbool.h>

typedef struct{ //struct qui donne les informations de la liste
    uint32_t total_size,current_size;
    void **base_ptr,**current_ptr,**end_ptr;
}dpa;

extern dpa* InitDPA(dpa* d,uint32_t size);// on initialise les elements a l'interieur de la liste

extern dpa* CreateDPA(uint32_t size); //crée la liste

extern void** DPAGarbageCollector(dpa* d); //On enlever les cases vides qui sont dans la liste

extern dpa* DPAResize(dpa* d,uint32_t new_size);//redimensionner la liste 

extern void* DPAInsertElement(dpa* d,void* ptr); // Inserer des elements dans la liste 

extern void DPARemoveElement(dpa* d,void** ptr_elt);//enlever des elements de la liste 

extern void DPAFree(dpa* d,bool free_ptr);//libere la liste choisi

#endif