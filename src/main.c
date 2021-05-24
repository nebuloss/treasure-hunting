#include "main.h"
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include "loader.h"
#include "configfile2/configfile.h"
#include "logfile.h"
#include "mapgrid.h"
#include "display.h"
#include <SDL2/SDL.h>
#include "editor.h"
#include <SDL2/SDL_ttf.h>

FILE* logfile=NULL; //logfile global variable shared

static key* main_cfg=NULL;  //mainconf configfile

void CloseLogFile(void){ //close log file
    LogWrite(logfile,LOG_INFO,"closing program");
    fclose(logfile);
}

void FreeConfigFile(void){ //close and free all ressources main config file
    CfgClose(main_cfg);
}

int main(int argc,char* argv[]){ 
    char *logfile_name=NULL;
    char* mainconf_filename="assets/main.conf"; //path of main.conf
    mainconf* main_config=NULL;
    map_package* mp=NULL;
    map_objects* mo=NULL;
    character_path* p=NULL;
    SDL_Window *window1,*window2;
    SDL_Renderer *renderer1,*renderer2;
    editor_objects current_eo;

    if ((main_cfg=CfgOpen(mainconf_filename))==NULL){ //try to open main.conf
        fprintf(stderr,"Fatal Error: Unable to find %s\n",mainconf_filename); //print error in standard error stream
        return EXIT_SUCCESS; //just quit in case of failure
    }
    atexit(FreeConfigFile);
    

    logfile_name=CfgSearchStringValue(main_cfg,NULL,"log"); //searh "log" key in main.conf
    if (logfile_name==NULL) logfile_name="main.log"; //default value
    
    logfile=fopen(logfile_name,"a"); //open log file in appending mode
    
    if (logfile) atexit(CloseLogFile); //when we call exit(EXIT_SUCCESS), it will call all atexit's functions
    else fprintf(stderr,"Unable to open log file: %s\n",logfile_name);

    LogWrite(logfile,LOG_INFO,"starting program"); //write some informations
    
    
    if (TTF_Init()){ //init ttf engine
        LogWrite(logfile,LOG_ERROR,"Unable to init TTF engine"); //write error in logfile
        exit(EXIT_SUCCESS);    
    }

    atexit(TTF_Quit); 
    
    if (SDL_Init(SDL_INIT_VIDEO)){ //init sdl
        LogWrite(logfile,LOG_ERROR,"Unable to init SDL_VIDEO");
        exit(EXIT_SUCCESS);
    }
    atexit(SDL_Quit);

    main_config=LoadConf(main_cfg); //load all base objects

    if (argc==2 && !strcmp(argv[1],"editor")){ //map editor mode
        current_eo=LoadEditor(main_config); //load all ressources needed (map array+texture dictionary)
        InitEditor(current_eo); //init the window
        EditorEditMap();//edit the map
        EditorSaveMap(main_config); //save the map in a binary file
        CloseEditor(); //destroy all objects
        FreeMainConf(main_config); //free configfile
        exit(EXIT_SUCCESS);
    }

    //default mode of execution
    if ((mo=LoadObjects(main_config))==NULL){ //load main objects
        FreeMainConf(main_config); 
        exit(EXIT_SUCCESS);
    }

    if (SDL_CreateWindowAndRenderer(640,400,SDL_WINDOW_SHOWN,&window1,&renderer1)){ //create welcome window
        FreeMapObjects(mo);
        FreeMainConf(main_config);
        LogWrite(logfile,LOG_ERROR,"Unable to init window or renderer (Menu)");
        exit(EXIT_SUCCESS);
    }

    SDL_SetWindowTitle(window1,"Carte Tresor"); //settings 
    SDL_SetWindowPosition(window1,300,100); 
    SDL_SetWindowIcon(window1,main_config->icon);

    SDL_Rect r={0,80,mo->map->w,mo->map->h};

    if (SDL_CreateWindowAndRenderer(r.w,r.h+80,SDL_WINDOW_HIDDEN,&window2,&renderer2)){ //create game window
        LogWrite(logfile,LOG_ERROR,"Unable to init window or renderer (Game)");
        FreeMapObjects(mo);
        FreeMainConf(main_config);
        exit(EXIT_SUCCESS);
    }

    SDL_SetWindowTitle(window2,"Carte Tresor");
    SDL_SetWindowPosition(window2,100,50);
    SDL_SetWindowIcon(window2,main_config->icon);
    
    
    while (WelcomeMenu(renderer1,main_config)){ //while we do not quit the welcome menu
        SDL_HideWindow(window1); //switch window
        SDL_ShowWindow(window2);

        if ((mp=MenuAskForPositions(renderer2,mo,main_config))){ //ask positions of 4 chest + 1 character
            p=EvaluateCharacterPath(&mp->pos,&mo->mg); //evaluate the shortest path to go through the 4 points
            CharacterSearchTresure(renderer2,mp,p); //display character moving
            FreeCharacterPath(p); //free the path
            FreeMapPackage(mp); //free textures used
        }
    
        SDL_HideWindow(window2); //switch windows again
        SDL_ShowWindow(window1);
    }


    FreeMapObjects(mo); //free all ressources used during program execution
    FreeMainConf(main_config);

    SDL_DestroyRenderer(renderer1); 
    SDL_DestroyWindow(window1);
    SDL_DestroyRenderer(renderer2);
    SDL_DestroyWindow(window2);


    exit(EXIT_SUCCESS); //call atexit's functions
}