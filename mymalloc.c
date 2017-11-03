/*
 * CS214 Systems Programming
 * Joseph Gormley & Sylvia Zhang
 * mymalloc.c
 */ 
#include <stdio.h>
#include <stdlib.h>
#include "mymalloc.h"

static char myBlock[5000];

typedef struct Node {
	char used; //freed or allocated
	unsigned short size; 
} Metadata;  

/** SMART MALLOC **/
void* myMalloc(int bytes, char * file, int line){
	
	//ERROR CHECKS
	if(bytes < 1){
		fprintf(stderr, "Must request a positive number of bytes to allocate - FILE: %s Line: %d\n", file, line);
	}

	//SETS MEMORY TO FREE
	if(*myBlock == '\0'){
		Metadata data = {'F', sizeof(myBlock) - sizeof(Metadata)};
		*(Metadata *)myBlock = data;
	}
	
	//ITERATE MEMORY TO FIND FREE BLOCK
	char * ptr = NULL;
	for(ptr = myBlock; ptr - myBlock <= sizeof(myBlock); ptr = ptr + sizeof(Metadata) + (*(Metadata *)ptr).size){
		if((*(Metadata *)ptr).used == 'F' && bytes <= (*(Metadata *)ptr).size){
			
			int blockSize = (*(Metadata *)ptr).size;			


			if(blockSize - bytes <= 4){
				(*(Metadata *)ptr).used = 'T';
				return (void*)(ptr + sizeof(Metadata));
			} 
	
			//UPDATE METADATA
			(*(Metadata *)ptr).used = 'T';	
			(*(Metadata *)ptr).size = bytes;

			ptr = ptr + sizeof(Metadata) + bytes;

			//ADD NEW METADATA
			Metadata data = {'F', blockSize - bytes - sizeof(Metadata)};
			*(Metadata *)ptr = data;
						
			return (void *)(ptr - bytes);	
		}
	}
	fprintf(stderr, "Not enough memory to allocate requested bytes. - File: %s, Line: %d.\n", file, line);
	return 0; //NO MEMORY TO ALLOCATE REQUESTED BYTES
}

/** Smart Free **/
void myFree(void * ptr, char * file, int line){

	//ERROR CONDITIONS
	if((void*)myBlock > ptr || ptr > (void*)(myBlock+5000) || ptr == NULL || ((*(Metadata *)(ptr-sizeof(Metadata))).used != 'F' && (*(Metadata *)(ptr-sizeof(Metadata))).used != 'T')){ 
		fprintf(stderr, "Pointer not dynamically located! - File: %s, Line: %d.\n", file, line);
		return;
	}

	if((*(Metadata *)(ptr - sizeof(Metadata))).used == 'F'){
		fprintf(stderr, "Pointer already freed! - File: %s, Line: %d.\n", file, line);
		return;
	}
	
	//SET FREE FLAG
	(*(Metadata *)(ptr - sizeof(Metadata))).used = 'F';		


	//IS NEXT BLOCK FREE?
	char * nextBlock = (ptr + (*(Metadata *)(ptr-sizeof(Metadata))).size);
	if(nextBlock < myBlock+5000){
		if((*(Metadata *)nextBlock).used == 'F'){
			//COMBINE BLOCKS
			(*(Metadata *)(ptr - sizeof(Metadata))).size = (*(Metadata *)(ptr - sizeof(Metadata))).size + sizeof(Metadata) + (*(Metadata *)nextBlock).size;  
		}
	}
	//IS PREVIOUS BLOCK FREE?
	char * prevBlock = NULL;    
	for(prevBlock = myBlock; prevBlock - myBlock <= sizeof(myBlock); prevBlock = prevBlock + sizeof(Metadata) + (*(Metadata *)prevBlock).size){	
		if(prevBlock + (sizeof(Metadata)*2) + (*(Metadata *)prevBlock).size == ptr && (*(Metadata *)prevBlock).used == 'F'){
			//COMBINE BLOCKS
		 	(*(Metadata *)prevBlock).size = (*(Metadata *)prevBlock).size + sizeof(Metadata) + (*(Metadata * )(ptr - sizeof(Metadata))).size;
		}
	}
	return;

}

