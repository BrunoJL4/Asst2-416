// name: Bruno J. Lucarelli
//       Joseph Gormley
//       Alex Marek
// username of iLab: bjl145
// iLab Server: man.cs.rutgers.edu


#include "mymalloc.h"
#define TOTALMEM 8388608 //2^20 x 2^3 = 8 megabytes. 
#define THREADREQ 0 //User called
#define LIBRARY 1 //Library called
#define PAGESIZE sysconf(_SC_PAGE_SIZE) //System page size


/* Define global variables here. */

/* Boolean 1 if manager thread is active, otherwise 0 as globals
are initialized to by default*/
uint manager_active;

/* The global array containing the memory we are "allocating" */
static char myBlock[TOTALMEM];

/* This is the page table */
char * pageTable[MAX_NUM_THREADS + 2]; //+2 so that our pthread library "manager" index is not out-of-bounds

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
		   MAX_NUM_THREADS*sizeof(tcb) size of a
		   pointer, times 2.
		   4. Space for all of the stacks we allocate. This will
		   be, in the worst case, MAX_NUM_THREADS * MEM. MEM
		   comes from the my_pthread_t.h file.
		   5. Space  for the Page Table.  It's just space for
		   pointers  to a page, which will be used to access a 
		   page metadata block. So MAX_NUM_THREADS*size of a pointer.*/
		// say kernel is not free, amount of freed space will be: 

		//Kernel memory space is defined by this metadata
		int kernalSize = (sizeof(Metadata) + (2 * MAX_NUM_THREADS * size(pnode)) + (MAX_NUM_THREADS * sizeof(tcb)) + (sizeof(pnode *) + sizeof(tcbList *)) + (MAX_NUM_THREADS * MEM) + (MAX_NUM_THREADS * sizeof(int *)));
		Metadata data = { BLOCK_USED, kernalSize};
		//Throw this meta data to the front of our memory block
		*(Metadata *)myBlock = data;
		
		//Find out the remainder of mem space after adding pages
		//This is the remaining mem modulo size of the page
		//Remaining mem is MEM - kernel space + metadata
		int remainingMem = (MEM - kernalSize) % (PAGESIZE);
		(Metadata *)myBlock->size += remainingMem;
		pageTable[MAX_NUM_THREADS + 1] = &myBlock;
		
		//Create metadata for each page
		//Continue jumping by pagesize
		char * ptr = &myBlock + myBlock->size();
		while (ptr < &myBlock + TOTALMEM) {
			data = { BLOCK_FREE, PAGESIZE };
			*(Metadata *)ptr = data;
			ptr += (Metadata *)ptr->size;
		}
		
		//End of initial page and kernel setup	
	}
	
	//Check if the calling thread has a page
	//If so, search for available space
	if (pageTable[current_thread] != NULL) {
		//This thread has a page, look for segment
		char * ptr = pageTable[current_thread] + sizeof(Metadata);
		while (ptr < pageTable[current_thread] + PAGESIZE) {
			//If the segment is free, check if the next is also free, then check if it is enough space
			if ((SegMetadata *)ptr->used == BLOCK_FREE) {
				//If nextPtr is still in bounds, check if it is free
				if (ptr + (SegMetadata *)ptr->size < pageTable[current_thread] + PAGESIZE) {
					char * nextPtr = ptr + (SegMetadata *)ptr->size;
					//If nextPtr is free, combine with ptr
					if ((SegMetadata *)nextPtr->used == BLOCK_FREE) {
						(SegMetadata *)ptr->size += (SegMetadata *)nextPtr->size + sizeof(SegMetadata); //Treat metadata for segments as if being outside the segment
						//@all (mostly @Bruno): wipe the metadata of nextPtr
					}
				}
				//If this segment has enough space, assign user requested space, set remaining to free, and return
				if ((SegMetadata *)ptr->size <= bytes) {
					SegMetadata segment = { BLOCK_USED, bytes };
					//If all bytes were not used, create another free segment (+/-sizeof(SegMetadata) considering leakage)
					if ((SegMetadata *)ptr->size < (bytes + sizeof(SegMetadata))) {
						char * nextPtr = ptr + sizeof(SegMetadata) + (SegMetadata *)ptr->size;
						SegMetadata nextSegment = { BLOCK_FREE, (SegMetadata *)ptr->size - (bytes + sizeof(SegMetadata)) };
						*(SegMetadata *)nextPtr = nextSegment;
						(SegMetadata *)ptr->size -= sizeof(SegMetadata) + (SegMetadata *)nextPtr;
					}
					return ptr + sizeof(SegMetadata);
				}
			}
			ptr += (SegMetadata *)ptr->size;
		}
		//If we reached this point, no segments available, return NULL (Phase A)
		return NULL;
		
	} else { //Assign a page, return NULL if none available (Phase A)
		char * ptr = &myBlock + myBlock->size();
		while (ptr < &myBlock + TOTALMEM) {
			//If this page is not being used, claim it
			if ((Metadata *)ptr->used == BLOCK_FREE) {
				pageTable[current_thread] = ptr;
				break;
			}
			ptr += PAGESIZE;
		}
		//If no page was found, return NULL (Phase A)
		if (pageTable[current_thread] == NULL) {
			return NULL;
		}
		
		//We have assigned a page, give memory segment
		SegMetadata segment = { BLOCK_USED, bytes }
		ptr = pageTable[current_thread] + sizeof(Metadata);
		*(SegMetadata *)ptr = segment;
		//Set remaining page to free segment
		char * nextPtr = ptr + (SegMetadata *)ptr->size + sizeof(SegMetadata);
		int remainingPageSpace = PAGESIZE - ((nextPtr - 1) - pageTable[current_thread]);
		segment = { BLOCK_FREE, remainingPageSpace };
		*(SegMetadata *)nextPtr = segment;
		
		//Return pointer to user requested segment
		return ptr + sizeof(SegMetadata);
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

