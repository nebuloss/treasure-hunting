#include "mapfile.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

uint32_t* ReadMap(char *filename){
	size_t lenght,total_lenght,lcpy;
	uint32_t width, height,*buffer;
	FILE* f;

	f=fopen(filename,"rb"); //ouvre le fichier en mode binaire
	if (f==NULL) return NULL;
	fseek(f,0,SEEK_END); //calcule la longueur totale du fichier
	lenght=ftell(f);

	if(lenght<8){ //longueur trop courte pour extraire la largeur et la hauteur
		fclose(f);
		return NULL;
	}
	rewind(f); //on revient au début du fichier

	buffer=malloc(lenght); //alloue un tableau de taille suffisante pour recopier le fichier
	if (buffer==NULL){
		fclose(f);
		return NULL;
	}
	lcpy=lenght>>2;
	fread(buffer,4,lcpy,f); //on copie le fichier
	fclose(f);

	width= *buffer;
	height= buffer[1];
	total_lenght=(width*height+2)<<2;

	if(lenght<total_lenght){ //longueur non valide
		free(buffer);
		return NULL;
	}
	return buffer;
}


uint32_t* WriteMap(char* filename, uint32_t* bytes){
    size_t total_size=(bytes[0]*bytes[1]+2); //écrit dans un fichier la map
    FILE* f=fopen(filename,"wb");
    if(f==NULL) return NULL;
	
	fwrite(bytes,4,total_size,f);
	fclose(f);

	return bytes;

}

void PrintMap(uint32_t* bytes){ //affiche la map sous forme de tableau (pour vérifier s'il n'y a pas d'erreur)
	uint32_t width=*bytes++,height=*bytes++;
	for(int i =0;i<height;i++){
		for(int j=0;j<width;j++){
			printf("%9u ",*bytes++);
		}
		putchar('\n');
	}
}

uint32_t* GenerateMap(uint32_t width,uint32_t height){ //génère une map remplie de 0
	size_t total= width*height+2;
	uint32_t* bytes=calloc(total,sizeof(uint32_t)),*ptr=bytes;
	*bytes++=width;
	*bytes++=height;
	return ptr;	
}