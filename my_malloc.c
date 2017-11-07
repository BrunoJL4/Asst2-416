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
	// establish the base size of the kernel's space in memory
	int kernelSize = sizeof(Metadata) 
					 + (2 * MAX_NUM_THREADS * sizeof(pnode)) // pnodes allocation + buffer
					 + (MAX_NUM_THREADS * sizeof(tcb)) // tcb allocation
					 + (sizeof(pnode *) + sizeof(tcb **)) // MLPQ & tcbList
					 + (MAX_NUM_THREADS * MEM) // stack allocations for child threads
					 + ((MAX_NUM_THREADS + 2) * sizeof(char *)); //page table 
	// "thread" var represents the calling thread's ID
	int thread;
	// "pagesize" var represents bound size of the current page (kernel vs user)
	int pagesize;
	if(req == THREADREQ) {
		thread = current_thread;
		pagesize = PAGESIZE;
	}	
	else if(req == LIBRARYREQ) {
		thread = MAX_NUM_THREADS + 1;
		pagesize = kernelSize;
	}
	else{
		printf("Error! Invalid value for req: %d\n", req);
		return NULL;
	}
	printf("Beginning myallocate(), current_thread is: %d\n", current_thread);
	// INITIALIZE KERNEL AND CREATE PAGE ABSTRACTION(FIRST MALLOC))
	if(*myBlock == '\0'){
		printf("Initializing kernel space in memory.\n");
		// the memory that would be "left over" after dividing the global block into pages,
		// is lumped into the kernel's space
		int remainingMem = (TOTALMEM - kernelSize ) % (PAGESIZE); 
		kernelSize += remainingMem;
		// create Metadata for kernel's block... mark it as BLOCK_USED with kernelSize as size
		Metadata data = (Metadata) { BLOCK_USED, kernelSize }; 
		// Metadata for kernel is at the beginning of the global block, and will be followed
		// by freely-usable space for kernel allocations
		*(Metadata *)myBlock = data;
		// pageTable is put in the "last" space in the kernel block... each cell stores a pointer, so
		// pageTable is set to a casted pointer-array at this space in memory.
		pageTable = (char **)((myBlock + kernelSize) - ((MAX_NUM_THREADS + 2) * sizeof(char *)));
		// MAX_NUM_THREADS + 1 is pages owned by scheduler/kernel, which means that its entry in
		// pageTable should point to the Metadata for the kernel... which is at the beginning of myBlock.
		pageTable[MAX_NUM_THREADS + 1] = myBlock;
		// Go through myBlock and create Metadata structs at the beginning of each page, essentially
		// "initializing" myBlock for allocations.
		char * ptr = myBlock + ((Metadata *)myBlock)->size;
		while (ptr < (myBlock + TOTALMEM)) {
			// each block is free by default and will always be of size PAGESIZE.
			Metadata data = { BLOCK_FREE, PAGESIZE };
			*(Metadata *)ptr = data;
			// initialize struct for the first segment's metadata, which takes up the remaining
			// space in the page to start
			SegMetadata segment = {BLOCK_FREE, PAGESIZE - sizeof(SegMetadata) - sizeof(Metadata) };
			// copy the struct to the block
			*(SegMetadata *)(ptr + sizeof(Metadata)) = segment;
			// go to the next page
			ptr += ((Metadata *)ptr)->size;
		}
	} //End of kernel setup and page creating	
	
	//IF THREAD DOES NOT HAVE A PAGE, ASSIGN ONE IF AVAILABLE
	if (pageTable[thread] == NULL) {
		printf("Assigning page for thread %d\n", thread);
		// Go through myBlock and find the first free page
		char * ptr = myBlock + ((Metadata *)myBlock)->size;
		while (ptr < myBlock + TOTALMEM) {
			// Claim the first free/available page
			if (((Metadata *)ptr)->used == BLOCK_FREE) {
				((Metadata *)ptr)->used = BLOCK_USED;
				pageTable[thread] = ptr;
				break;
			}
			// Try the next page
			ptr += PAGESIZE;
		}
	}
	
	//DID MEM MANAGER FIND A FREE PAGE?
	if (pageTable[thread] == NULL) {
		printf("No free pages in pageTable, thread %d\n", thread);
		return NULL; //phaseA
	}	

	// Combine any consecutive, free segments in the current thread's page.
	// At the same time, look and see if any free segments can store the user's request.
	char * ptr = pageTable[thread] + sizeof(Metadata);
	while (ptr < pageTable[thread] + pagesize) {		
		printf("Checking for combinable segments in page for thread: %d\n", thread);
		// If the current segment is free:
		if (((SegMetadata *)ptr)->used == BLOCK_FREE) {
			// If there is a following segment within the page:
			if ((ptr + ((SegMetadata *)ptr)->size + sizeof(SegMetadata)) < (pageTable[thread] + pagesize)) {
				char * nextPtr = ptr + ((SegMetadata *)ptr)->size + sizeof(SegMetadata);
					// If that following segment is free, combine the segments.
					// Loop for further free segments
					while (((SegMetadata *)nextPtr)->used == BLOCK_FREE) {		
						((SegMetadata *)ptr)->size += ((SegMetadata *)nextPtr)->size + sizeof(SegMetadata); //Combine ptr & nextPtr segments
						// If there is another segment in the page, increment, otherwise break
						if ((nextPtr + ((SegMetadata *)nextPtr)->size + sizeof(SegMetadata)) < (pageTable[thread] + pagesize)) {
							nextPtr = nextPtr + ((SegMetadata *)nextPtr)->size + sizeof(SegMetadata);
						} else {
							break;
						}
					}
			}	
			// If the current segment can hold the data, use it.
			if (((SegMetadata *)ptr)->size >= bytes) {
				((SegMetadata *)ptr)->used = BLOCK_USED;
				printf("Allocated thread: %d's requested space.\n", current_thread);
				// If the entire segment wasn't needed, take the remaining space and make another segment from it.
				if (((SegMetadata *)ptr)->size > (bytes + sizeof(SegMetadata))) {
					printf("Entire segment wasn't needed, setting remaining space to free/open for thread: %d\n", thread);
					char * nextPtr = ptr + sizeof(SegMetadata) + bytes;
					SegMetadata nextSegment = { BLOCK_FREE, ((SegMetadata *)ptr)->size - (bytes + sizeof(SegMetadata)) };
					*(SegMetadata *)nextPtr = nextSegment;
					((SegMetadata *)ptr)->size -= (sizeof(SegMetadata) + ((SegMetadata *)nextPtr)->size);
				}
				// If the remaining space can't hold another segment, just give the remaining data to the user.
				// TODO @all: Is this a valid approach? It could cause segfaults.
				return (void *)(ptr + sizeof(SegMetadata));
			}
		}
		// segment is not free, iterate to next segment
		ptr += ((SegMetadata *)ptr)->size + sizeof(SegMetadata);
	}
	
	// NO SEGMENTS AVAILABLE
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
	printf("Beginning mydeallocate for thread: %d\n", current_thread);
	if((void *)myBlock > ptr || ptr > (void*)(myBlock + TOTALMEM) || ptr == NULL || ((*(SegMetadata *)(ptr-sizeof(SegMetadata))).used == BLOCK_FREE && (*(SegMetadata *)(ptr-sizeof(SegMetadata))).used != BLOCK_USED)){ 
		fprintf(stderr, "Pointer not dynamically located! - File: %s, Line: %d.\n", file, line);
		return;
	}
	
	if((*(SegMetadata *)(ptr - sizeof(SegMetadata))).used == BLOCK_FREE){
		fprintf(stderr, "Pointer already freed! - File: %s, Line: %d.\n", file, line);
		return;
	}
	
	//THIS WILL CHANGE WHEN WE DO SWAP FILE
	//IS REQUESTED SEGMENT TO BE FREE WITHIN START AND END OF ASSIGNED PAGE?
	if(pageTable[thread] < (char *)ptr && (pageTable[thread] + PAGESIZE) > (char *)ptr)
		((Metadata *)(ptr - sizeof(SegMetadata)))->used = BLOCK_FREE; //set flag
	else
		fprintf(stderr, "Segfault! - File: %s, Line: %d.\n", file, line);
		
	
	return;

}

