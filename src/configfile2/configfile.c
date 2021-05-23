#include "configfile2/configfile.h"
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <ctype.h>
#include "configfile2/parsestring.h"

//a new version of the library is currently in development. Therefore there is not a lot of comments in the source code

#define PARENT_DIRECTORY_HASH 0x61C //1564 = 0b11000011100 (DictionaryHashKey(DKV_STRING(".."))=1564)
#define CONFIGFILE(cf) ((configfile*)cf)
#define ROOT_DICTIONARY_SIZE 4
#define FSTAB_DICTIONARY_MAX_SIZE 10

typedef struct{
    key k;
    char buffer[];
}configfile;

static uint16_t cfg_tree_indentation;
static uint8_t cfg_bits_directory=3;

static char* cfg_root_folder_name="/";

static key cfg_parent_directory_key={
    .hash=PARENT_DIRECTORY_HASH,
    .name.s="..",
    .name_type=CF_STRING,
    .value_type=CF_DIRECTORY,
};

static key cfg_root_directory_key={
    .hash=0,
    .name.p=NULL,
    .name_type=DK_POINTER,
    .value_type=DK_POINTER,
    .index=0
};

static void CfgFreeKeyFunction(key* k){
    key_type kt=k->value_type;
    if (kt!=CF_DIRECTORY) return;
    
    dictionary* d=k->value.p;
    if (d->filter_min==2) free((d->table+1)->value.p);
    FreeDictionary(d);
}

dictionary* CfgInitDirectoryDictionary(key* parent_directory){
    dictionary* d=AllocDictionary(cfg_bits_directory);
    d->func=CfgFreeKeyFunction;
    d->filter_min=1;
    cfg_parent_directory_key.value.p=parent_directory->value.p;
    cfg_parent_directory_key.index=PARENT_DIRECTORY_HASH&d->mask;
    DictionaryAddElement(d,&cfg_parent_directory_key,true);
    return d;
}

dictionary* CfgInitRootDirectoryDictionary(key* root_directory){
    dictionary* d=AllocDictionary(ROOT_DICTIONARY_SIZE);
    d->func=CfgFreeKeyFunction;
    cfg_parent_directory_key.value.p=d;
    cfg_parent_directory_key.index=PARENT_DIRECTORY_HASH&d->mask;
    DictionaryAddElement(d,&cfg_parent_directory_key,true);
    cfg_root_directory_key.value.p=root_directory;
    DictionaryAddElement(d,&cfg_root_directory_key,true);
    d->filter_min=2;
    
    return d;
}

key* CfgCreateStringKey(key* directory,char* name,char* value){
    return DictionaryAddKey(directory->value.p,DKV_STRING(name),DKV_STRING(value));
}

key* CfgCreateIntegerKey(key* directory,char* name,int64_t value){
    return DictionaryAddKey(directory->value.p,DKV_STRING(name),DKV_I64(value));
}

key* CfgCreateFloatKey(key* directory,char* name,double value){
    return DictionaryAddKey(directory->value.p,DKV_STRING(name),DKV_DOUBLE(value));
}

key* CfgCreateSymbolicLink(key* directory,char* name,char* path){
    return DictionaryAddKey(directory->value.p,DKV_STRING(name),DKV_SYM_LINK(path));
}

key* CfgGetParentDirectory(key* directory){
    return DICTIONARY(directory->value.p)->table;
}

key* CfgSearchRootDirectory(key* directory){  //priorité à la complexité en temps (pas en code)
    for (key* k1=DICTIONARY(directory->value.p)->table;k1!=directory;directory=k1)
        k1=DICTIONARY(directory->value.p)->table;
    return directory;   
}

key* CfgMountDirectory(key* directory,key* mounted_directory,char* name){
    dictionary *d2=mounted_directory->value.p,*d1;

    if (d2->filter_min!=2) return NULL;

    key* t=d2->table;
    if (d2!=t->value.p) CfgUnmountDirectory(mounted_directory);
    d1=directory->value.p;
    DictionaryAddKey(d1,DKV_STRING(name),DKV_DIRECTORY(d2));
    t->value.p=d1;
    mounted_directory->name.s=name;
    return directory;
}

key* CfgUnmountDirectory(key* mounted_directory){
    dictionary *d=mounted_directory->value.p,*d2;
    if (d->filter_min!=2) return NULL;
    key* t=d->table;

    mounted_directory=(t+1)->value.p;
    mounted_directory->name.s=cfg_root_folder_name;
    d2=t->value.p;
    DictionaryRemoveValue(d2,mounted_directory->hash,false);
    t->value.p=d;

    return mounted_directory;
}

static char* CfgHashString(char* read_ptr,key_hash* hash_ptr){
    key_hash h=0;
    for (uint8_t c;(c=*read_ptr) && c!='/';read_ptr++) h=DICTIONARY_HASH_FUNCTION(h,c);
    *hash_ptr=h;
    return read_ptr;
}

key* CfgSearchPath(key* current_directory,key* root_directory,char* path){
    key_hash h;
    key_type kt;
    while(1){
        path=CfgHashString(path,&h);
        if (*path=='/'){
            if (h){
                current_directory=DictionaryGetValue(current_directory->value.p,h);
                if (current_directory==NULL) return current_directory;
                kt=current_directory->value_type;
                if (kt==CF_SYM_LINK){
                    current_directory=CfgCD(current_directory,root_directory,current_directory->value.s);
                    if (current_directory==NULL) return NULL;
                } 
                else if (kt!=CF_DIRECTORY) return NULL;
            }else{
                if (root_directory) current_directory=root_directory;
                else root_directory=CfgSearchRootDirectory(current_directory);
            }
            if (!*path) return current_directory;
        }else{
            if (h) current_directory=DictionaryGetValue(current_directory->value.p,h);
            return current_directory;
        }
        path++;
    }
}

key* CfgCD(key* current_directory,key* root_directory,char* path){
    current_directory=CfgSearchPath(current_directory,root_directory,path);
    if (current_directory==NULL) return current_directory;
    if (current_directory->value_type==CF_SYM_LINK){
        current_directory=CfgCD(current_directory,root_directory,current_directory->value.s);
        if (current_directory==NULL) return NULL;
    }
    if (current_directory->value_type==CF_DIRECTORY) return current_directory;
    return NULL;
}

static void CfgPrintTreeElement(key* k){
    key_value kv=k->value;
    for (uint16_t i=0;i<cfg_tree_indentation;i++){
        putchar('|');
        putchar(' ');
    }
    fputs(k->name.s,stdout);
    switch(k->value_type){
        case CF_DIRECTORY:
            putchar('\n');
            cfg_tree_indentation++;
            ForEachKey(kv.p,CfgPrintTreeElement);
            cfg_tree_indentation--;
            return;
        case CF_STRING:
            printf("=\"%s\"\n",kv.s);
            break;
        case CF_INT:
            printf("=%" PRIi64 "\n",kv.i64);
            break;
        case CF_FLOAT:
            printf("=%lf\n",kv.d);
            break;
        case CF_SYM_LINK:
            printf("=%s\n",kv.s);
            break;
        default:
            break;
    }
}

void CfgPrintTree(key* directory){
    cfg_tree_indentation=0;
    CfgPrintTreeElement(directory);
}

char* CfgSearchStringValue(key* current_directory,key* root_directory,char* path){
    key* k=CfgSearchPath(current_directory,root_directory,path);
    if (k && k->value_type==CF_STRING) return k->value.s;
    return NULL;
}

char* CfgSearchSymbolicLink(key* current_directory,key* root_directory,char* path){
    key* k=CfgSearchPath(current_directory,root_directory,path);
    if (k && k->value_type==CF_SYM_LINK) return k->value.s;
    return NULL;
}

int64_t* CfgSearchIntegerValue(key* current_directory,key* root_directory,char* path){
    key* k=CfgSearchPath(current_directory,root_directory,path);
    if (k && k->value_type==CF_INT) return &k->value.i64;
    return NULL;
}

double* CfgSearchFloatValue(key* current_directory,key* root_directory,char* path){
    key* k=CfgSearchPath(current_directory,root_directory,path);
    if (k && k->value_type==CF_FLOAT) return &k->value.d;
    return NULL;
}

static key* CfgCreateDirectoryWithHash(key* directory,key_hash h,char* name){
    dictionary* d=CfgInitDirectoryDictionary(directory);
    directory=DictionaryAddValue(directory->value.p,h,DKV_DIRECTORY(d));
    directory->name.s=name;
    directory->name_type=CF_STRING;
    return directory;
}

key* CfgForceCD(key* current_directory,key* root_directory,char* path_rw){
    key_hash h;
    char *current;
    key* k;

    while (1){
        current=CfgHashString(path_rw,&h);
  
        if (*current){
            *current++='\0';
            if (h){
                k=DictionaryGetValue(current_directory->value.p,h);
                if (!k || k->value_type!=CF_DIRECTORY) current_directory=CfgCreateDirectoryWithHash(current_directory,h,path_rw);
                else current_directory=k;
            }else{
                if (root_directory) current_directory=root_directory;
                else current_directory=root_directory=CfgSearchRootDirectory(current_directory);
            }
            path_rw=current;
        }else{
            if (!h) break;
            k=DictionaryGetValue(current_directory->value.p,h);
            if (k){
                if (k->value_type==CF_DIRECTORY) current_directory=k;
                else current_directory=CfgCreateDirectoryWithHash(current_directory,h,path_rw);
            }
            else current_directory=CfgCreateDirectoryWithHash(current_directory,h,path_rw);
            break;
        }
        
    }  
    return current_directory;
}

void CfgPrintKey(key* k){
    key_type kt=k->value_type;
    printf("name=\"%s\" ",k->name.s);
    if (kt==CF_DIRECTORY) fputs("type=folder",stdout);
    else{
        fputs("value=",stdout);
        if (kt==CF_STRING) printf("\"%s\" type=string",k->value.s);
        else if (kt==CF_INT) printf("%" PRIi64 " type=integer",k->value.i64);
        else if (kt==CF_FLOAT) printf("%lf type=float",k->value.d);
        else if (kt==CF_SYM_LINK) printf("%s type=symbolic_link",k->value.s);
    }
    putchar('\n');
}

void CfgLS(key* directory){
    if (directory->value_type!=CF_DIRECTORY) return;
    printf("Listing folder \"%s\":\n",directory->name.s);

    ForEachKey(directory->value.p,CfgPrintKey);
}


key* CfgCreateDirectory(key* parent_directory,char* directory_name){
    if (parent_directory->value_type==CF_SYM_LINK){
        parent_directory=CfgCD(parent_directory,NULL,parent_directory->value.s);
        if (parent_directory==NULL) return NULL;
    }
    key* nf=DictionaryAddKey(parent_directory->value.p,DKV_STRING(directory_name),
    DKV_DIRECTORY(CfgInitDirectoryDictionary(parent_directory)));

    return nf;
}

static void InitConfigFile(configfile* cf){
    cf->k=(key){
        .name.s=cfg_root_folder_name,
        .name_type=CF_STRING,
        .value.p=CfgInitRootDirectoryDictionary(KEY(cf)),
        .value_type=CF_DIRECTORY
    };
}


static key* CfgParse(key* root_directory){
    key_value kv;
    bool isfloat;
    key* current_directory=root_directory;

    if (root_directory==NULL) return NULL;
    char *buffer=CONFIGFILE(root_directory)->buffer,c,*svalue,*name;

    while (1){
        buffer=SkipBlankSpace(buffer);
        if (!buffer) break;
        c=*buffer;
        if (c=='['){
            name=SkipBlankSpace(++buffer);
            buffer=RstripString(name,']');
            current_directory=CfgForceCD(root_directory,root_directory,name);
            if (!buffer) break;
              
        }else if (c!='#' && c!='\n' && c!='\r'){
            name=buffer;
            buffer=RstripString(buffer,'=');
            if (!buffer) break;
            buffer=SkipBlankSpace(buffer);
            if (!buffer) break;
            c=*buffer;
            if (c=='"'){
                svalue=++buffer;
                buffer=DelimitString(buffer,'"');
                CfgCreateStringKey(current_directory,name,svalue);
            }else{
                buffer=ReadNumber(buffer,&kv,&isfloat);
                if (isfloat) CfgCreateFloatKey(current_directory,name,kv.d);
                else CfgCreateIntegerKey(current_directory,name,kv.i64);
            }   
        }
        buffer=SkipLine(buffer);
        if (!buffer) break;
    }

    return root_directory;
}

key* CfgLoad(char* filename){
    struct stat st;
    int fd;

    if ((fd=open(filename,O_RDONLY))==-1) return NULL;
    fstat(fd,&st);

    configfile* cf=malloc(sizeof(configfile)+st.st_size+1);
    InitConfigFile(cf);
    
    cf->buffer[read(fd,cf->buffer,st.st_size)]='\0';
    close(fd);
    
    return KEY(cf);
}

key* CfgOpenFSTAB(char* filename){
    key *root_directory=CfgLoad(filename),*mounted_directory,*mount_directory;
    if (root_directory==NULL) return NULL;

    char *buffer=CONFIGFILE(root_directory)->buffer,c,*name,*path,*value;
    int dictionary_size;

    while (1){
        buffer=SkipBlankSpace(buffer);
        if (!buffer) break;

        c=*buffer;
        if (c=='"'){
            name=++buffer;
            buffer=DelimitString(buffer,'"');
            if (!buffer) break;
            buffer=SkipBlankSpace(buffer);
            if (!buffer) break;
            c=*buffer;
            if (c=='"' && *name!='\0'){
                path=++buffer;
                buffer=DelimitString(buffer,'"');
                if (!buffer) break;
                
                for (value=buffer-2;(c=*value)!='"';value--){
                    if (c=='/'){
                        *value='\0';
                        break;
                    }
                }
                value++;

                if (value!=path) mount_directory=CfgForceCD(root_directory,root_directory,path);
                else mount_directory=root_directory;
                
                buffer=SkipBlankSpace(buffer);
                buffer=ReadInteger(buffer,&dictionary_size);

                if (dictionary_size<DICTIONARY_MIN_BITS) dictionary_size=DICTIONARY_MIN_BITS;
                else if (dictionary_size>FSTAB_DICTIONARY_MAX_SIZE) dictionary_size=FSTAB_DICTIONARY_MAX_SIZE;
                cfg_bits_directory=dictionary_size;

                mounted_directory=CfgOpen(name);
                cfg_bits_directory=DICTIONARY_MIN_BITS;
                if (mounted_directory && *value) CfgMountDirectory(mount_directory,mounted_directory,value);
            }
        }
        buffer=SkipLine(buffer);
        if (!buffer) break;
    }
    return root_directory;   
}

key* CfgOpen(char* filename){
    return CfgParse(CfgLoad(filename));
}

key* CfgCreate(void){
    configfile* cfg=malloc(sizeof(configfile));
    InitConfigFile(cfg);
    return KEY(cfg);
}

bool CfgIsDirectoryMounted(key* mount_directory){
    dictionary* d=mount_directory->value.p;
    return d!=d->table->value.p;
}

bool CfgIsMountDirectory(key* mount_directory){
    return DICTIONARY(mount_directory->value.p)->filter_min==2;
}

bool CfgIsRootDirectory(key* mount_directory){
    return mount_directory->name.s==cfg_root_folder_name;
}

void CfgClose(key* mount_directory){
    if (!mount_directory) return;
    if (!CfgIsMountDirectory(mount_directory)) return; 
    
    if (CfgIsDirectoryMounted(mount_directory)) CfgUnmountDirectory(mount_directory);
    CfgFreeKeyFunction(mount_directory);
}