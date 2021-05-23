#include "configfile2/dictionary.h"
#include <stdlib.h> //malloc free calloc
#include <string.h> //memcpy
#include <inttypes.h>  //printf format
#include <stdio.h>

//a new version of the library is currently in development. Therefore there is not a lot of comments in the source code

typedef int (*io_dictionary_function)(FILE*,const char*,...); //trick to bypass compiler warnings

const io_dictionary_function pf=fprintf; //we just create a copy of printf function with another name :-)

#define EMPTY_CASE -1

#define PERTURB_SHIFT 5

static const size_t dictionary_size_key_data=(sizeof(key_value)<<1)+4;

static const char* dk_types_name[]={
    "int64",
    "uint64",
    "int32",
    "uint32",
    "int16",
    "uint16",
    "int8",
    "uint8",
    "bool",
    "pointer",
    "double",
    "float",
    "string",
    "void",
    "dictionary",
    "list"
};

static const char* dk_format_name[]={
    "%" PRIi64, //int64_t
    "%" PRIu64, //uint64_t
    "%" PRIi32, //...
    "%" PRIu32,
    "%" PRIi16,
    "%" PRIu16,
    "%" PRIi8,
    "%" PRIu8,
    "%hhu",
    "%p",
    "%lf",
    "%f",
    "\"%s\"",
    "void",
    "%p"
};


void DictionaryPrintKeyData(key_value v,key_type t,FILE* stream){
    if (t<DK_DOUBLE) pf(stream,dk_format_name[t],v);
    else if (t==DK_DOUBLE) pf(stream,"%lf",v.d);
    else if (t==DK_FLOAT) pf(stream,"%f",v.f);
    else if (t<=DK_DICTIONARY) pf(stream,dk_format_name[t],v);
    else fputs("unknown",stream);

    fputs(" type=",stream);
    if (t<15) fputs(dk_types_name[t],stream);
    else fputs("unknown",stream);
}

void DictionaryPrintKey(key* k){
    printf("hash=%" PRIx64 " name=",k->hash);
    DictionaryPrintKeyData(k->name,k->name_type,stdout);
    fputs(" value=",stdout);
    DictionaryPrintKeyData(k->value,k->value_type,stdout);
    fputc('\n',stdout);
}

void DictionaryPrint(dictionary* d){
    ForEachKey(d,DictionaryPrintKey);
}

key_hash DictionaryHashKey(key_data k){
    key_hash h;
    if (k.t<DK_STRING) return k.v.u64; //general hash
    else if (k.t==DK_STRING){ //special hash
        h=0;
        for (uint8_t c;(c=*k.v.s);k.v.s++) h=DICTIONARY_HASH_FUNCTION(h,c);
        return h;
    }
    else return 0;
}

uint32_t DictionarySearchIndex(dictionary* d,key_hash h){
    uint64_t perturb=h;
    uint32_t mask=d->mask;
    key** index=d->index;
    int j=h&mask;
    key* k=index[j];
    while (k){
        if (k->hash==h) return j;
        perturb>>=PERTURB_SHIFT;
        j*=5;
        j++;
        j+=perturb;
        j&=mask;
        k=index[j];
    }
    return j;
}

/*same function but does not return the same information: equivalent => return d->index[DictionarySearchIndex(d,h)]
but cloning the function gives better performance in the search time
*/
key* DictionaryGetValue(dictionary* d,key_hash h){
    uint64_t perturb=h;
    uint32_t mask=d->mask;
    key** index=d->index;
    int j=h&mask;
    key* k=index[j];
    while (k){
        if (k->hash==h) return k;
        perturb>>=PERTURB_SHIFT;
        j*=5;
        j++;
        j+=perturb;
        j&=mask;
        k=index[j];
    }
    return NULL;
}

uint8_t log_2(uint32_t n){
    uint8_t i=0;
    while (n>1){
        n>>=1;
        i++;
    }
    return i;
}

static dictionary* InitDictionary(dictionary* d,uint8_t bits){
    size_t table,index,index_size;
    void *index_ptr,*table_ptr;

    index_size=1<<bits;
    table=sizeof(key)*((index_size<<1)/3);
    index=sizeof(key*)*index_size;
    if ((index_ptr=malloc(index+table))==NULL) return NULL;

    d->index=index_ptr;
    d->mask=index_size-1;
    d->bits=bits;
    d->max_garbage_collector=(index_size<<2)/9;
    d->current_adress=d->table=table_ptr=index_ptr+index;
    d->current_size=0;
    d->first_suppression=d->end_table=table_ptr+table;

    memset(index_ptr,0,index);
    return d;
}


dictionary* AllocDictionary(uint8_t bits){
    if (bits<DICTIONARY_MIN_BITS) bits=DICTIONARY_MIN_BITS;
    if (bits>DICTIONARY_MAX_BITS) bits=DICTIONARY_MAX_BITS;

    dictionary* d=malloc(sizeof(dictionary));
    if (InitDictionary(d,bits)==NULL){
        free(d);
        return NULL;
    }
    d->filter_min=0;
    d->filter_max=-1;
    d->func=NULL;
    return d;
}

dictionary* CreateDictionary(uint32_t size){
    return AllocDictionary(log_2((size*3)>>1)+1);
}

dictionary* NewDictionary(void){
    return AllocDictionary(DICTIONARY_MIN_BITS);
}

dictionary* NewSpecialDictionary(key_function fkf){
    dictionary* d=NewDictionary();
    d->func=fkf;
    return d;
}

void FreeDictionary(dictionary* d){
    if (!d) return;
    key_function kf=d->func;
    if (kf) ForEachKey(d,kf);
    free(d->index);
    free(d);
}

void DictionaryGarbageCollector(dictionary* d){
    key *k=d->table,*e=d->end_table,*w,**index_table=d->index;
    int32_t index;
    
    if (d->first_suppression==d->end_table) return;
    w=k=d->first_suppression;
    k++;
    while (k<e){
        if ((index=k->index)>=0){
            memcpy(w,k,sizeof(key));
            index_table[index]=w;
            w++;
        }
        k++;
    }
    d->current_adress=w;
    d->first_suppression=e;
    d->filter_max=-1;
}

void ExtendDictionary(dictionary* d,uint8_t bits){
    key **ptr=d->index,*table=d->table,*end_table=d->current_adress;
    uint32_t index,dim=d->current_size,shift=(void*)d->first_suppression-(void*)table;

    InitDictionary(d,d->bits+bits); 
    key *dst=d->table,**index_dst=d->index;

    memcpy(dst,table,(void*)end_table-(void*)table); //copy all the table

    for (;table!=end_table;table++,dst++){
        if (table->index!=EMPTY_CASE){
            table->index=index=DictionarySearchIndex(d,table->hash);
            *(index_dst+index)=dst;        
        }
    }
    d->current_adress=dst;
    d->current_size=dim;
    d->first_suppression=(void*)dst+shift;
    free(ptr);
}

key* DictionaryAddElement(dictionary* d,key* k,bool overwrite){
    key** i=d->index+k->index,*c=*i;
    
    if (c){
        if (overwrite){
            if (d->func) d->func(c);
            memcpy(&c->value,&k->value,dictionary_size_key_data);
        }
    }else{
        if (d->current_adress==d->end_table){
            if (d->current_size>d->max_garbage_collector) ExtendDictionary(d,3);
            else DictionaryGarbageCollector(d);
            k->index=DictionarySearchIndex(d,k->hash);
            return DictionaryAddElement(d,k,true);
        }else{
            *i=c=d->current_adress;
            memcpy(c,k,sizeof(key));
            d->current_size++;
            d->current_adress++;
        }
    }
    return c;
}

key* DictionaryAddValue(dictionary* d,key_hash h,key_data elt){
    int32_t j=DictionarySearchIndex(d,h);
        key k={
        .hash=h,
        .value=elt.v,
        .name_type=DK_VOID,
        .value_type=elt.t,
        .index=j
    };
    return DictionaryAddElement(d,&k,true);
}


key* DictionaryAddKey(dictionary* d,key_data key_,key_data elt){
    if (key_.t>DK_VOID) return NULL;
    key_hash h=DictionaryHashKey(key_);
    int32_t j=DictionarySearchIndex(d,h);
    key k={
        .hash=h,
        .name=key_.v,
        .value=elt.v,
        .name_type=key_.t,
        .value_type=elt.t,
        .index=j
    };
    return DictionaryAddElement(d,&k,true);
}

key* DictionaryGetKey(dictionary* d,key_data key_){
    return DictionaryGetValue(d,DictionaryHashKey(key_));
}

key* DictionaryRemoveElement(dictionary* d,key* k,bool free_elt){
    key* last_element=d->current_adress-1;

    if (k==NULL) return NULL;
    if (free_elt && d->func) d->func(k);
    d->index[k->index]=NULL;
    k->index=EMPTY_CASE;
    if (k<d->first_suppression) d->first_suppression=k;
    else if (k==last_element) d->current_adress=last_element;
    d->current_size--;
    return k;
}

key* DictionaryRemoveValue(dictionary* d,key_hash h,bool free_elt){
    return DictionaryRemoveElement(d,DictionaryGetValue(d,h),free_elt);
}

key* DictionaryRemoveKey(dictionary* d,key_data key_){
    return DictionaryRemoveElement(d,DictionaryGetKey(d,key_),true);
}

void ForEachKey(dictionary* d,key_function func){ 
    int32_t min=d->filter_min,max=d->filter_max,current=0;
    key *k=d->table,*e=d->current_adress;

    for(;current<min;k++) if (k->index!=-1) current++;
    if (max==-1){
        for(;k<e;k++) if (k->index!=-1) func(k);
    }else{
        for(;current<max;k++){
            if (k->index!=-1){
                func(k);
                current++;
            }
        }
    }
}

void ForEachKeyFilteredValue(dictionary* d,key_type kt,key_function func,void* arg){ //fork of ForEachKey
    int32_t min=d->filter_min,max=d->filter_max,current=0;
    key *k=d->table,*e=d->current_adress;

    for(;current<min;k++) if (k->index!=-1) current++;
    if (max==-1){
        for(;k<e;k++) if (k->index!=-1 && k->value_type==kt) func(k,arg);
    }else{
        for(;current<max;k++){
            if (k->index!=-1){
                if (k->value_type==kt) func(k,arg);
                current++;
            }
        }
    }
}

key* DictionaryGetKeyWithIndex(dictionary* d,int32_t index){
    int32_t min=d->filter_min,max=d->filter_max,current,dim=d->current_size;
    key *k=d->table,*e=d->current_adress;

    if (max==-1) max=dim;
    
    if (e-k==dim){ //random access
        if (index>=0){
            index+=min;
            if (index>max) return NULL;
        }else{
            index+=max;
            if (index<min) return NULL;
        }
        return k+index;
    }
    if (index>=0){    //sequential access
        index+=min;
        if (index>max) return NULL; 
        dim=d->first_suppression-k;
        if (index<dim) return k+index; //exception 
        for(current=0;current<index;k++) if (k->index!=-1) current++;
        return k;
    }else{
        index+=max;
        if (index<min) return NULL;
        for(current=dim-1,e--;current>index;e--) if (e->index!=-1) current--;
        return e;
    }
}

key* DictionaryRemoveKeyWithIndex(dictionary* d,int32_t index){
    return DictionaryRemoveElement(d,DictionaryGetKeyWithIndex(d,index),true);
}

void DictionarySetBeginPoint(dictionary* d){
    d->filter_min=d->current_size;
}

void DictionarySetEndPoint(dictionary* d){
    d->filter_max=d->current_size;
}

void DictionaryResetPoints(dictionary* d){
    d->filter_max=-1;
    d->filter_min=0;
}
