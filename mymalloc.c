// name: Bruno J. Lucarelli
//       Joseph Gormley
//       Alex Marek
// username of iLab: bjl145
// iLab Server: man.cs.rutgers.edu

#include <stdio.h>
#include <stdlib.h>
#include "mymalloc.h"
#define MEM 8388608 //2^20 x 2^3 = 8 megabytes. 
#define THREADREQ 0 //User called
#define LIBRARY 1 //Library called

/* Define global variables here. */

/*Metadata Node

A struct that holds metadata for a block of memory.
These structs will be stored before each individual 
block of memory in myBlock. 
*/
typedef struct Node {
	char used; //freed or allocated
	unsigned short size; 
} Metadata;  

static char myBlock[MEM];
/* End global variable declarations. */

/* malloc & free function implementations */

/** SMART MALLOC **/
void* myMalloc(int bytes, char * file, int line, int threadreq){
	
	//ERROR CHECKS
	if(bytes < 1){
		fprintf(stderr, "Must request a positive number of bytes to allocate - FILE: %s Line: %d\n", file, line);
	}

	//SETS MEMORY TO FREE (FIRST MALLOC)
	if(*myBlock == '\0'){
		Metadata data = {'F', MEM - sizeof(Metadata)};
		*(Metadata *)myBlock = data;
	}
	
	if(threadreq){ //LIB called 
		//allocate block in sys side of mem 
	}else{ //USR called
		//check page table
		//does respective page have room for malloc call?
	}
	
	
	
	/* THIS IS IMPLEMENTATION FROM CS214
	//ITERATE MEMORY TO FIND FREE BLOCK IN PAGE
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

			ptr = ptr + sizeof(Metadata) + bytes

			//ADD NEW METADATA
			Metadata data = {'F', blockSize - bytes - sizeof(Metadata)};
			*(Metadata *)ptr = data;
						
			return (void *)(ptr - bytes);	
		}
	}
	*/
	
	
	fprintf(stderr, "Not enough memory to allocate requested bytes. - File: %s, Line: %d.\n", file, line);
	return NULL; //NO MEMORY TO ALLOCATE REQUESTED BYTES
	
}

/** Smart Free **/
void myFree(void * ptr, char * file, int line, int threadreq){

	//ERROR CONDITIONS
	if((void*)myBlock > ptr || ptr > (void*)(myBlock+MEM) || ptr == NULL || ((*(Metadata *)(ptr-sizeof(Metadata))).used != 'F' && (*(Metadata *)(ptr-sizeof(Metadata))).used != 'T')){ 
		fprintf(stderr, "Pointer not dynamically located! - File: %s, Line: %d.\n", file, line);
		return;
	}

	if((*(Metadata *)(ptr - sizeof(Metadata))).used == 'F'){
		fprintf(stderr, "Pointer already freed! - File: %s, Line: %d.\n", file, line);
		return;
	}
	
	if(threadreq){ //LIB called
		//free memory
		
	}else{ //USR called
		//check page table to see if thread is using page
		
	}
	
	

	/* THIS IS IMPLEMENTATION FROM CS214
	//SET FREE FLAG
	(*(Metadata *)(ptr - sizeof(Metadata))).used = 'F';		

	//IS NEXT BLOCK FREE?
	char * nextBlock = (ptr + (*(Metadata *)(ptr-sizeof(Metadata))).size);
	if(nextBlock < myBlock+MEM){
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
	
	*/
	return;

}

