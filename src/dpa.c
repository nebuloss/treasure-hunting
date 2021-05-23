#include "dpa.h"
#include <stdlib.h>
#include <stdio.h>

dpa* InitDPA(dpa* d,uint32_t size){ // on initialise les elements a l'interieur de la liste
    void** p;
    size_t total_size_allocated=size*sizeof(void*);

    if ((p=malloc(total_size_allocated))==NULL) return NULL;
    d->base_ptr=p;
    d->current_ptr=p;
    d->end_ptr=((void*)p)+total_size_allocated;
    d->total_size=size;
    d->current_size=0;
    return d;
}

dpa* CreateDPA(uint32_t size){  //crée la liste
    dpa* d=malloc(sizeof(dpa));
    if (InitDPA(d,size)==NULL){
        free(d);
        return NULL;
    }
    return d;
}

void** DPAGarbageCollector(dpa* d){ //On enlever les cases vides qui sont dans la liste
    void **rp=d->base_ptr,**wp,**stop,*value;
    if (d->current_size==0) return (d->current_ptr=rp);

    stop=d->current_ptr;
    
    while (*rp) rp++;
    wp=rp++;
    
    while (rp<stop){
        if ((value=*rp++)) *wp++=value;
    }
    d->current_ptr=wp;
    return wp;
}

dpa* DPAResize(dpa* d,uint32_t new_size){ //redimensionner la liste 
    void **newptr;
    size_t total_allocated_memory,current_size=d->current_ptr-d->base_ptr;

    if (new_size<=current_size) new_size=current_size<<1;

    total_allocated_memory=new_size*sizeof(void*);

    if ((newptr=realloc(d->base_ptr,total_allocated_memory))==NULL) return NULL;
    d->end_ptr=((void*)newptr)+total_allocated_memory;
    d->current_ptr=newptr+current_size;
    d->base_ptr=newptr;
    d->total_size=new_size;
    
    return d;
}

void* DPAInsertElement(dpa* d,void* ptr){ // Inserer des elements dans la liste 
    uint32_t temp_value;

    if (ptr==NULL) return ptr;
    if (d->current_ptr==d->end_ptr){
        temp_value=d->total_size<<1;
        if (d->current_size>temp_value/3){
            if (DPAResize(d,temp_value)==NULL) return NULL;
        }
        else DPAGarbageCollector(d);
    }
    d->current_size++;
    *(d->current_ptr++)=ptr;
    return ptr;
}

void DPARemoveElement(dpa* d,void** ptr_elt){ //enlever des elements de la liste 
    *ptr_elt=NULL;
    d->current_size--;
}

void DPAFree(dpa* d,bool free_ptr){ //libere la liste choisi
    free(d->base_ptr);
    if (free_ptr) free(d);
}