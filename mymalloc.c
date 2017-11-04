// name: Bruno J. Lucarelli
//       Joseph Gormley
//       Alex Marek
// username of iLab: bjl145
// iLab Server: man.cs.rutgers.edu


#include "mymalloc.h"
#define TOTALMEM 8388608 //2^20 x 2^3 = 8 megabytes. 
#define THREADREQ 0 //User called
#define LIBRARY 1 //Library called


/* Define global variables here. */

/* Boolean 1 if manager thread is active, otherwise 0 as globals
are initialized to by default*/
uint manager_active;

/* The global array containing the memory we are "allocating" */
static char myBlock[TOTALMEM];

/* End global variable declarations. */

/* malloc & free function implementations */

/** SMART MALLOC **/
void* myallocate(int bytes, char * file, int line, int req){
		
	//(FIRST MALLOC) initialize kernel
	if(*myBlock == '\0'){
		
		/* init kernal space - kernal space is:
		   1. Enough space for pnodes we allocate plus buffer.
		   This would be about 2 * MAX_NUM_THREADS * sizeof(pnode)
		   2. Enough space for all of the TCB's. This  would be
		   MAX_NUM_THREADS*sizeof(tcb).
		   3. Space for MLPQ and tcbList. This is the size of a
		   pointer, times 2.
		   4. Space for all of the stacks we allocate. This will
		   be, in the worst case, MAX_NUM_THREADS * MEM. MEM
		   comes from the my_pthread_t.h file.
		   5. Space  for the Page Table.  It's just space for
		   pointers  to a page, which will be used to access a 
		   page metadata block. So MAX_NUM_THREADS*size of a pointer.*/ 
		// say kernel is not free, amount of freed space will be:  
		Metadata data = {'F', MEM - (MAX_NUM_THREAD * size(pnode))};
		*(Metadata *)myBlock = data;
		
		//have page boundries understood
	}
	
	//if calling user thread does not have a page
		//assign a page
	//else retrieve the page associated with thread
	
	//start at initial page meta data
	//continue to jump from different meta data to the next
		//if block is free
			//combine with any adjacent free blocks
			//If block is big enough
				//divide into requested size and return
			//else continue
	
	if(req){ //LIB called 
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
void mydeallocate(void * ptr, char * file, int line, int req){

	//ERROR CONDITIONS
	if((void*)myBlock > ptr || ptr > (void*)(myBlock+MEM) || ptr == NULL || ((*(Metadata *)(ptr-sizeof(Metadata))).used != 'F' && (*(Metadata *)(ptr-sizeof(Metadata))).used != 'T')){ 
		fprintf(stderr, "Pointer not dynamically located! - File: %s, Line: %d.\n", file, line);
		return;
	}

	if((*(Metadata *)(ptr - sizeof(Metadata))).used == 'F'){
		fprintf(stderr, "Pointer already freed! - File: %s, Line: %d.\n", file, line);
		return;
	}
	
	if(req){ //LIB called
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

