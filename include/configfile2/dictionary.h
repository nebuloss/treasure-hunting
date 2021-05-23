#ifndef DICTIONARY_H_INCLUDED
#define DICTIONARY_H_INCLUDED

//a new version of the library is currently in development. Therefore there is not a lot of comments in the source code
/*This library works only with 64 bits operating systems*/

#include <stdint.h>
#include <stdbool.h>

typedef uint16_t key_type;
typedef uint64_t key_hash;

#define DK_INT64 0 //define some types id
#define DK_UINT64 1
#define DK_INT32 2
#define DK_UINT32 3
#define DK_INT16 4
#define DK_UINT16 5
#define DK_INT8 6
#define DK_UINT8 7
#define DK_BOOL 8
#define DK_POINTER 9
#define DK_DOUBLE 10
#define DK_FLOAT 11
#define DK_STRING 12
#define DK_VOID 13
#define DK_DICTIONARY 14 //exception type: mutable object
#define DK_LIST 15 //exception type: mutable object

#define DICTIONARY_MIN_BITS 3
#define DICTIONARY_MAX_BITS 22

/*Union of all the types which can be used*/
typedef union{
    int64_t i64; 
    uint64_t u64;
    int32_t i32;
    uint32_t u32;
    int16_t i16;
    uint16_t u16;
    int8_t i8;
    uint8_t u8;
    double d;
    float f;
    bool b;
    char* s;
    void* p;
}key_value;

/*little package which contains the variable and the type (size: 16 bytes)*/
typedef struct{
    key_value v;
    key_type t;
}key_data;

//some useful macros
#define DKV_I64(i) (key_data){.v.i64=i,.t=DK_INT64}
#define DKV_I32(i) (key_data){.v.i32=i,.t=DK_INT32}
#define DKV_I16(i) (key_data){.v.i16=i,.t=DK_INT16}
#define DKV_I8(i)  (key_data){.v.i8=i,.t=DK_INT8}
#define DKV_U64(u) (key_data){.v.u64=u,.t=DK_UINT64}
#define DKV_U32(u) (key_data){.v.u32=u,.t=DK_UINT32}
#define DKV_U16(u) (key_data){.v.u16=u,.t=DK_UINT16}
#define DKV_U8(u)  (key_data){.v.u8=u,.t=DK_UINT8}
#define DKV_DOUBLE(d_) (key_data){.v.d=d_,.t=DK_DOUBLE}
#define DKV_FLOAT(f_) (key_data){.v.f=f_,.t=DK_FLOAT}
#define DKV_BOOL(b_) (key_data){.v.b=b_,.t=DK_BOOL}
#define DKV_STRING(s_) (key_data){.v.s=s_,.t=DK_STRING}
#define DKV_POINTER(p_) (key_data){.v.p=p_,.t=DK_POINTER}
#define DKV_VOID (key_data){.v.p=NULL,.t=DK_VOID}

#define DICTIONARY_HASH_FUNCTION(h,c) ((h<<5)+h)+c

/*key structure. A dictionary is an array of key (size=32 bytes)*/
typedef struct{
    key_hash hash;
    key_value name,value;
    key_type name_type,value_type;
    int32_t index;
}key;

#define KEY(k) ((key*)k)

//the first argument of the function must be a key* pointer
typedef void (*key_function)();

//dictionary structure
typedef struct{
    uint32_t max_garbage_collector,bits;
    uint32_t mask,current_size;
    key** index;
    key *table,*end_table,*current_adress,*first_suppression;
    int32_t filter_min,filter_max;
    key_function func;
}dictionary;


typedef struct{

}__DictionaryClass;

#define DICTIONARY(d) ((dictionary*)d)

//allocate a dictionary of n bits
extern dictionary* AllocDictionary(uint8_t bits);

//create a dictionary which can contain "size" values before being resized
extern dictionary* CreateDictionary(uint32_t size);

//create a 3 bits dictionary (max keys=(2/3)*2^3=5)
extern dictionary* NewDictionary(void);

//free dictionary
extern void FreeDictionary(dictionary* d);

//add a key and an element in the dictionary
extern key* DictionaryAddKey(dictionary* d,key_data name,key_data value);

//execute the function for each key inside the dictionary
extern void ForEachKey(dictionary* d,key_function func);

//remove key from dictionary
extern key* DictionaryRemoveKey(dictionary* d,key_data key_);

//copy key structure inside the dictionary reserved space
extern key* DictionaryAddElement(dictionary* d,key* k,bool overwrite);

//hash an immutable object 
extern key_hash DictionaryHashKey(key_data k);

//return the first occurence of the hash or return the first free case
extern uint32_t DictionarySearchIndex(dictionary* d,key_hash h);

//resize the dictionary (expansive operation)
extern void ExtendDictionary(dictionary* d,uint8_t bits);

//dictionary garbage collector
extern void DictionaryGarbageCollector(dictionary* d);

//return a key structure with the hash
extern key* DictionaryGetValue(dictionary* d,key_hash h);

//return a key structure with a key
extern key* DictionaryGetKey(dictionary* d,key_data key_);

//create a dictionary of 3 bits with a free key function
extern dictionary* NewSpecialDictionary(key_function fkf);

//add an element without key name
extern key* DictionaryAddValue(dictionary* d,key_hash h,key_data elt);

//print the dictionary (built-in function)
extern void DictionaryPrint(dictionary* d);

//print a key (built-in function)
extern void DictionaryPrintKey(key* k);

//begin point for browsing dictionary
extern void DictionarySetBeginPoint(dictionary* d);

//end point for browsing dictionary
extern void DictionarySetEndPoint(dictionary* d);

//reset points for browsing dictionary: by default select all
extern void DictionaryResetPoints(dictionary* d);

//filter value type of dictionary
extern void ForEachKeyFilteredValue(dictionary* d,key_type kt,key_function func,void* arg);

//remove key from its hash
extern key* DictionaryRemoveValue(dictionary* d,key_hash h,bool free_elt);

//remove a key from a dictionary
extern key* DictionaryRemoveElement(dictionary* d,key* k,bool free_elt);

//search a key and remove it from the dictionary
extern key* DictionaryRemoveKey(dictionary* d,key_data key_);

//return key with the given index in the table. Failure: return NULL
extern key* DictionaryGetKeyWithIndex(dictionary* d,int32_t index);

//remove key from its index (apply free key function)
extern key* DictionaryRemoveKeyWithIndex(dictionary* d,int32_t index);

#endif
