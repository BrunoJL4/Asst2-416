// name: Bruno J. Lucarelli
//       Joseph Gormley
//       Alex Marek
// username of iLab: bjl145
// iLab Server: man.cs.rutgers.edu


#include "my_malloc.h"

/* Define global variables here. */

/* Boolean 1 if manager thread is active, otherwise 0 as globals
are initialized to by default*/
uint manager_active;

/* The global array containing the memory we are "allocating" */
static char myBlock[TOTALMEM];

/* This is the page table */
char ** pageTable; 

/* End global variable declarations. */

/* malloc & free function implementations */

/** SMART MALLOC **/
void* myallocate(int bytes, char * file, int line, int req){
	int thread;
	if(req == THREADREQ) {
		thread = current_thread;
	}	
	else if(req == LIBRARYREQ) {
		thread = MAX_NUM_THREADS + 1;
	}
	else{
		printf("Error! Invalid value for req: %d\n", req);
		return NULL;
	}
	printf("Beginning myallocate(), thread is: %d\n", thread);
	//INITIALIZE KERNEL AND CREATE PAGE ABSTRACTION(FIRST MALLOC))
	if(*myBlock == '\0'){
		printf("Initializing kernel space in memory.\n");
		//CREATE KERNEL ABSTRACTION (w/ METADATA)
		int kernelSize = sizeof(Metadata) 
						 + (2 * MAX_NUM_THREADS * sizeof(pnode)) //pnodes allocation + buffer
						 + (MAX_NUM_THREADS * sizeof(tcb)) //tcb allocation
						 + (sizeof(pnode *) + sizeof(tcb **)) //MLPQ & tcbList
						 + (MAX_NUM_THREADS * MEM) //stack allocations
						 + ((MAX_NUM_THREADS + 1) * sizeof(char *)); //page table 
		int remainingMem = (TOTALMEM - kernelSize ) % (PAGESIZE);
		kernelSize += remainingMem; //remainingMem goes to kernel
		
		Metadata data = (Metadata) { BLOCK_USED, kernelSize }; 
		*(Metadata *)myBlock = data; //block is placed in front of memory
		
		pageTable = (char **)((myBlock + kernelSize) - ((MAX_NUM_THREADS + 1) * sizeof(char *))); //pt pointer
		pageTable[MAX_NUM_THREADS + 1] = myBlock; //MAX_NUM_THREADS + 1 is pages owned by scheduler
		
		//CREATE PAGE ABSTRACTION (w/ METADATA)
		char * ptr = myBlock + ((Metadata *)myBlock)->size;
		while (ptr < (myBlock + TOTALMEM)) {
			Metadata data = { BLOCK_FREE, PAGESIZE };
			*(Metadata *)ptr = data;
			ptr += ((Metadata *)ptr)->size;
		}
	} //End of kernel setup and page creating	
	//IF THREAD DOES NOT HAVE A PAGE, ASSIGN ONE IF AVAILABLE
	if (pageTable[thread] == NULL) {
		printf("Assigning page for thread %d\n", thread);
		char * ptr = myBlock + ((Metadata *)myBlock)->size; //Iterate through kernal
		while (ptr < myBlock + TOTALMEM) {
			if (((Metadata *)ptr)->used == BLOCK_FREE) { //If this page is free, claim it
				pageTable[thread] = ptr;
				break;
			}
			ptr += PAGESIZE;
		}
	}
	
	//DID MEM MANAGER FIND A FREE PAGE?
	if (pageTable[thread] == NULL) {
		printf("No free pages in pageTable, thread %d\n", thread);
		return NULL; //phaseA
	}	

	//LOOK FOR FREED SEGMENT WITHIN THREADS GIVEN PAGE & COMBINE APPLICABLE SEGMENTS
	char * ptr = pageTable[thread] + sizeof(Metadata);
	while (ptr < pageTable[thread] + PAGESIZE) {		
		printf("Checking for combinable segments in pageTable for thread: %d\n", thread);
		if (((SegMetadata *)ptr)->used == BLOCK_FREE) { //Is current segment free?
			if (ptr + ((SegMetadata *)ptr)->size + sizeof(SegMetadata) < pageTable[thread] + PAGESIZE) { //Is next segment free? (within bounds)
				printf("Combining segments in pageTable for thread: %d\n", thread);
				char * nextPtr = ptr + ((SegMetadata *)ptr)->size + sizeof(SegMetadata);
					if (((SegMetadata *)nextPtr)->used == BLOCK_FREE) {		
						((SegMetadata *)ptr)->size += ((SegMetadata *)nextPtr)->size + sizeof(SegMetadata); //Combine ptr & nextPtr segments
					}
			}	
			if (((SegMetadata *)ptr)->size <= bytes) { //Can free segment hold requested bytes?
				//IF ENTIRE SEGMENT WAS NOT NEEDED, SET REST TO FREE (REQUIRES A SEGMETADATA)
				if (((SegMetadata *)ptr)->size > (bytes + sizeof(SegMetadata))) {
					printf("Entire segment wasn't needed, setting remaining space to free/open for thread: %d\n", thread);
					char * nextPtr = ptr + sizeof(SegMetadata) + bytes;
					SegMetadata nextSegment = { BLOCK_FREE, ((SegMetadata *)ptr)->size - (bytes + sizeof(SegMetadata)) };
					*(SegMetadata *)nextPtr = nextSegment;
					((SegMetadata *)ptr)->size -= (sizeof(SegMetadata) + ((SegMetadata *)nextPtr)->size);
				}
				
				return (void *)(ptr + sizeof(SegMetadata)); //If there are no available bytes to do so, give extra to user.
			}
		}
		ptr += ((SegMetadata *)ptr)->size + sizeof(SegMetadata); //segment is not free, iterate to next segment
	}
	
	//NO SEGMENTS AVAILABLE
	printf("No segments available for thread: %d\n", thread);
	return NULL; //phase A
}

/** Smart Free **/
void mydeallocate(void * ptr, char * file, int line, int req){
	int thread;
	//ERROR CONDITIONS
	if(req == THREADREQ) {
		thread = current_thread;
	}	
	else if(req == LIBRARYREQ) {
		thread = MAX_NUM_THREADS + 1;
	}
	else{
		printf("Error! Invalid value for req: %d\n", req);
		return;
	}
	printf("Beginning mydeallocate for thread: %d\n", thread);
	if((void *)myBlock > ptr || ptr > (void*)(myBlock + TOTALMEM) || ptr == NULL || ((*(Metadata *)(ptr-sizeof(Metadata))).used == BLOCK_FREE && (*(Metadata *)(ptr-sizeof(Metadata))).used != BLOCK_USED)){ 
		fprintf(stderr, "Pointer not dynamically located! - File: %s, Line: %d.\n", file, line);
		return;
	}
	
	if((*(Metadata *)(ptr - sizeof(Metadata))).used == BLOCK_FREE){
		fprintf(stderr, "Pointer already freed! - File: %s, Line: %d.\n", file, line);
		return;
	}
	
	//IS REQUESTED SEGMENT TO BE FREE WITHIN START AND END OF ASSIGNED PAGE?
	if(pageTable[thread] < (char *)ptr && (pageTable[thread] + PAGESIZE) > (char *)ptr)
		((Metadata *)(ptr - sizeof(SegMetadata)))->used = BLOCK_FREE; //set flag
	else
		fprintf(stderr, "Segfault! - File: %s, Line: %d.\n", file, line);
		
	
	return;

}

