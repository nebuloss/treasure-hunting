#ifndef CONFIGFILE_H_INCLUDED
#define CONFIGFILE_H_INCLUDED
#include "dictionary.h"

//a new version of the library is currently in development. Therefore there is not a lot of comments in the source code

#define CF_STRING DK_STRING
#define CF_VOID DK_VOID
#define CF_INT DK_INT64
#define CF_FLOAT DK_DOUBLE
#define CF_DIRECTORY DK_DICTIONARY
#define CF_MOUNT_DIRECTORY 15
#define CF_SYM_LINK 16

#define DKV_DIRECTORY(f) (key_data){.v.p=f,.t=CF_DIRECTORY}
#define DKV_SYM_LINK(sl) (key_data){.v.p=sl,.t=CF_SYM_LINK}

extern key* CfgOpen(char* filename);

extern key* CfgCreateDirectory(key* parent_folder,char* foldername);

extern void CfgLS(key* folder);

extern key* CfgCreateStringKey(key* directory,char* name,char* value);

extern key* CfgCreateIntegerKey(key* directory,char* name,int64_t value);

extern key* CfgCreateFloatKey(key* directory,char* name,double value);

extern key* CfgCreateSymbolicLink(key* directory,char* name,char* path);

extern bool CfgIsDirectoryMounted(key* directory);

extern key* CfgGetParentDirectory(key* directory);

extern bool CfgIsRootDirectory(key* mount_directory);

extern key* CfgMountDirectory(key* directory,key* mounted_directory,char* name);

extern key* CfgUnmountDirectory(key* mounted_directory);

extern bool CfgIsMountDirectory(key* mount_directory);

extern key* CfgSearchPath(key* current_directory,key* root_directory,char* path);

extern key* CfgCD(key* current_path,key* root_directory,char* path);

/*fake config file*/
extern key* CfgCreate(void);

extern key* CfgSearchRootDirectory(key* directory);

/*Close a mount directory and free all memory allocated*/
extern void CfgClose(key* mount_directory);

/*Print the tree*/
extern void CfgPrintTree(key* directory);

/*Search a string in the tree. Error value: NULL*/
extern char* CfgSearchStringValue(key* current_directory,key* root_directory,char* path);

/*Search a symbolic link in the tree. Error value: NULL*/
extern char* CfgSearchSymbolicLink(key* current_directory,key* root_directory,char* path);

/*Search an integer in the tree. Error value: NULL*/
extern int64_t* CfgSearchIntegerValue(key* current_directory,key* root_directory,char* path);

/*Search a float value in the tree. Error value: NULL*/
extern double* CfgSearchFloatValue(key* current_directory,key* root_directory,char* path);

/*the path given in arguments will be used and modified. /!\ Does not accept constant strings*/
extern key* CfgForceCD(key* current_directory,key* root_directory,char* path_rw);

/*Print a key and his type*/
extern void CfgPrintKey(key* k);

/*Load a file into a buffer and create a root directory*/
extern key* CfgLoad(char* filename);

/*
   Parse fstab
   syntax expected:

   #this is a comment
   "myconfigfile" "mount_directory" N

   # N is the default size in bits of the allocated dictionary (between 3 and 10). It is an optional argument
*/
extern key* CfgOpenFSTAB(char* filename);

#endif