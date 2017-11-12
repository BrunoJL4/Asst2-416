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
char *myBlock;

/* This is the threadNodeList */
ThreadMetadata *threadNodeList; 

/* This is the PageTable */
PageMetadata *PageTable;


/* End global variable declarations. */

/* malloc & free function implementations */

/** SMART MALLOC **/
void* myallocate(int bytes, char * file, int line, int req){
	// initialize signal alarm struct
	memset(&mem_sig, 0, sizeof(mem_sig));
	// establish the base size of the kernel's space in memory
	int kernelSize = sizeof(PageMetadata) // size of its own metadata
					 + (2 * MAX_NUM_THREADS * sizeof(pnode)) // pnodes allocation + buffer
					 + (MAX_NUM_THREADS * sizeof(tcb)) // tcb allocation
					 + (sizeof(pnode *) + sizeof(tcb **)) // MLPQ & tcbList
					 + (MAX_NUM_THREADS * MEM) // stack allocations for child threads
					 + ((MAX_NUM_THREADS + 2) * sizeof(ThreadMetadata)) // threadNodeList
					 + ((TOTALMEM / PAGESIZE) * sizeof(PageMetadata)); // PageTable space, rounded up.
	// Figure out how many pages the kernel needs (floor/round-down), add eight more
	// pages to that, and then multiply that number of pages by PAGESIZE. That is the
	// actual size of the kernel block, and it now starts at the beginning of
	// an aligned page.
	kernelSize = ((kernelSize/PAGESIZE) + 8) * PAGESIZE;
	int remainingMem = (TOTALMEM - kernelSize ) % (PAGESIZE); 
	kernelSize += remainingMem;
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
		// First memalign space for the kernel.
		myBlock = (char *) memalign(PAGESIZE, TOTALMEM);
		// create PageMetadata for kernel's block... mark it as BLOCK_USED with kernelSize as size.
		PageMetadata data = (PageMetadata) { BLOCK_USED, kernelSize }; 
		// Metadata for kernel is at the beginning of the global block, and will be followed
		// by freely-usable space for kernel allocations (in the scheduler)
		*(PageMetadata *)myBlock = data;
		// threadNodeList is put in the "last" space in the kernel block... each cell stores a struct, so
		// threadNodeList is set to a pointer with size enough to store all of the ThreadMetadata structs.
		threadNodeList = (ThreadMetadata *) ((myBlock + kernelSize) - ((MAX_NUM_THREADS + 2) * sizeof(ThreadMetadata)));
		// MAX_NUM_THREADS is metadata for scheduler/kernel. Kernel's ThreadMetadata's first page is set to
		// -2, to tell myallocate() to handle its logic differently.
		// So far, we've only allocated threadNodeList.
		ThreadMetadata kernelData = {-2, sizeof(threadNodeList)}
		// Copy kernelData to the kernel's cell in threadNodeList.
		threadNodeList[MAX_NUM_THREADS + 1] = kernelData;
		// Initialize the standard cells for threadNodeList.
		int i;
		for(i = 0; i < MAX_NUM_THREADS; i++) {
			// Make new ThreadMetadata struct and copy it to threadNodeList
			ThreadMetadata newThreadData = {-1, 0};
			threadNodeList[i] = newThreaddata;
		}
		// Put PageTable before threadNodeList, allocating enough space for the remaining
		// pages in the memory. Meaning we get the address from subtracting the size of PageTable
		// from the address of threadNodeList.
		// Also, leave that last space at the end free for the swapping/free page, used
		// in signal handling.
		int threadPages = ((TOTALMEM - kernelSize)/(PAGESIZE)) - 1;
		PageTable = (PageMetadata *) (threadNodeList - (threadPages * PAGESIZE));
		// Go through PageTable and create the structs at each space, initializing their space
		// to be FREE and having 0 space used.
		for(i = 0; i < PAGESIZE - 1; i++) {
			// Make new PageMetadata struct and copy it to PageTable
			PageMetadata newData = {BLOCK_FREE, 0};
			PageTable[i] = newData;

		}
		// Increase counter for memory allocated by kernel, now that we've allocated PageTable.
		PageTable[MAX_NUM_THREADS + 1].memoryAllocated += (threadPages * PAGESIZE)
		// TODO @all: Reminder: Look at above logic for how PageTable is allocated, use BLOCK_FREE status
		// to determine whether a block should be allocated.
	} //End of kernel setup and page creating

	
	//IF CALLED BY SCHEDULER
	if(LIBRARYREQ){
		
		
		
	}else{ //IF CALLED BY THREAD
		
		char * pages = myBlock + kernelSize;
		
		//IF THREAD DOES NOT HAVE A PAGE, ASSIGN PAGE 0
		if(threadNodeList[current_thread].firstPage == -1){			
			//add page data in page table
			PageMetaData data = { BLOCK_USED, -1 }; 
			PageTable[0] = data; 				
			//add thread data in thread node list
			ThreadMetadata data = { 0, 0 }; //0 is threads first page(virtually), 0 memory allocated
			threadNodeList[current_thread] = data;	
		}
		
		
		// THREAD HAS TO HAVE AT LEAST ONE PAGE NOW
		// SEARCH THROUGH PAGE(S) FOR A FREE SEGMENT
		int pageNum = threadNodeList[current_thread].firstPage){
		while(page != -1){
			char * ptr = pages + (PAGESIZE * pageNum); //iterate to page
			while(ptr < ptr + PAGESIZE){
				//current segment is free and large enough to fit requested bytes
				if (((SegMetadata *)ptr)->used == BLOCK_FREE && ((SegMetadata *)ptr)->size >= bytes) {
					((SegMetadata *)ptr)->used = BLOCK_USED;
					// If the entire segment wasn't needed, take the remaining space and make another segment from it.
					if (((SegMetadata *)ptr)->size > (bytes + sizeof(SegMetadata))) {
						char * nextPtr = ptr + sizeof(SegMetadata) + bytes;
						SegMetadata nextSegment = { BLOCK_FREE, ((SegMetadata *)ptr)->size - (bytes + sizeof(SegMetadata)) };
						*(SegMetadata *)nextPtr = nextSegment;
						((SegMetadata *)ptr)->size -= (sizeof(SegMetadata) + ((SegMetadata *)nextPtr)->size);
					}
					// If the remaining space can't hold another segment, just give the remaining data to the user.
					return (void *)(ptr + sizeof(SegMetadata));
				}
				// segment is not free, iterate to next segment
				ptr += ((SegMetadata *)ptr)->size + sizeof(SegMetadata);
			}
			//page is not free, iterate to next page
			pageNum = PageTable[pageNum].next;
		}	
		
		// NO FREE SEGMENT, MUST ASSIGN ANOTHER PAGE
		// PAGENUM + 1 WILL BE NEXT
		
		//add page data in page table
		PageMetaData data = { BLOCK_USED, -1 }; 
			PageTable[0] = data; 	
	
	
	
	
	
	
	
	
		
}

/** Smart Free **/
void mydeallocate(void * ptr, char * file, int line, int req){
	int thread;
	int pageIndex
	char * index;
	int pagesize;
	//Set ptr to point to its SegMetadata
	ptr = ptr - sizeof(SegMetadata);
	if(req == THREADREQ) {
		thread = current_thread;
		printf("Beginning mydeallocate for thread: %d\n", current_thread);
		
		// TODO @Alex: replace "GLOBAL" with the global variable for the address of the first user space page
		// TODO @Alex: replace "KERNEL" with the kernel size
	
		// Find the page of the memory
		pageIndex = threadNodeList[thread].firstPage;
		while (pageIndex != -1) {
			// Index to the first memory address of pageIndex
			index = GLOBAL + (pageIndex * PAGESIZE)
			// If the address the user wishes to free belongs to this page, then break
			if (ptr >= index && ptr < (index + PAGESIZE)) {
				break;
			}			
			pageIndex = PageTable[pageIndex].nextPage;
		}
		// If pageIndex is -1, then we have a pagefault
		if (pageIndex == -1) {
			fprintf(stderr, "Pagefault! - File: %s, Line: %d.\n", file, line);
			exit(EXIT_FAILURE);
		}
		pagesize = PAGESIZE;
	}	
	else if(req == LIBRARYREQ) {
		thread = MAX_NUM_THREADS + 1;
		printf("Beginning mydeallocate for thread: %d\n", current_thread);
		index = &myBlock;
		pagesize = KERNEL;
	}
	else {
		printf("Error! Invalid value for req: %d\n", req);
		exit(EXIT_FAILURE);
	}	

	// If this is not a proper segment (freed or !SegMetadata), then Segfault	
	if (ptr < &myBlock || ptr >= &myBlock + sizeof(myBlock)) {
		if (((SegMetadata *)ptr)->used != BLOCK_USED) {
			fprintf(stderr, "Segfault! - File: %s, Line: %d.\n", file, line);
			exit(EXIT_FAILURE);
		}
	}
	
	// Set to free
	((SegMetadata *)ptr)->used = BLOCK_FREE;
	// Should we wipe the memory for this segment? Do it here if so
	
	// Combine next block if it is free
	// This is a user space combine, so we need to check if the next segment is in a page that belongs to this thread
	if (req == THREADREQ) {
		// Check if it is in bounds of myBlock
		if ((ptr + ((SegMetadata *)ptr)->size + sizeof(SegMetadata)) > (&myBlock + sizeof(myBlock))) {	
			// START OF CHECK
			// First check if you'll stay in pages belonging to the thread
			char * nextPtr = ptr + ((SegMetadata *)ptr)->size + sizeof(SegMetadata);
			// Find the page of the memory
			int endPageIndex = threadNodeList[thread].firstPage;
			while (endPageIndex != -1) {
				// Index to the first memory address of pageIndex
				char * endIndex = GLOBAL + (endPageIndex * PAGESIZE)
				// If the address the user is searching for belongs to this page, then break
				if (nextPtr >= endIndex && nextPtr < (endPageIndex + PAGESIZE)) {
					break;
				}			
				endPageIndex = PageTable[endPageIndex].nextPage;
			}
			// If pageIndex is -1, then we have a pagefault
			if (endPageIndex == -1) {
				fprintf(stderr, "Pagefault! - File: %s, Line: %d.\n", file, line);
				exit(EXIT_FAILURE);
			}
			// END OF CHECK

			// Confirmed next seg belongs to thread's page, so combine if free
			if (((SegMetadata *)nextPtr)->used == BLOCK_FREE) {
				((SegMetadata *)ptr)->size += sizeof(SegMetadata) + ((SegMetadata *)nextPtr)->size;
				// If we are wiping memory, wipe memory of nextPtr SegMetadata here
				
			}
		}
	}
	// This is a kernel space combine, so we don't need to check the spanning of multiple pages
	else {
		if ((ptr + ((SegMetadata *)ptr)->size + sizeof(SegMetadata)) < (index + KERNEL)) {
			char * nextPtr = ptr + ((SegMetadata *)ptr)->size + sizeof(SegMetadata);
			if (((SegMetadata *)nextPtr)->used == BLOCK_FREE) {
				((SegMetadata *)ptr)->size += sizeof(SegMetadata) + ((SegMetadata *)nextPtr)->size;
				// If we are wiping memory, wipe memory of nextPtr SegMetadata here
				
			}
		}
	}
	
	// If prev block is free, combine
	if (((SegMetadata *)ptr)->prev != NULL) {
		SegMetadata * prevPtr = ((SegMetadata *)ptr)->prev;
		if (prevPtr->used == BLOCK_FREE) {
			prevPtr->size += sizeof(SegMetadata) + ((SegMetadata *)ptr)->size;
			// If we are wiping memory, wipe memory of ptr SegMetadata here 
			
			// Set ptr to the front of the new free space
			ptr = (char *)prevPtr;
		}
	}
	
	// TODO @Alex: because segments can be multiple pages long, we could be freeing multiple pages
	
	// If the Segment is the size of the page AND it is at the start of a page, free the page (ONLY IF THIS IS NOT THE KERNEL)
	if (req == THREADREQ) {
		if ((((SegMetadata *)ptr)->size + sizeof(SegMetadata)) >= pagesize && ptr == index) {
			// pagePtr points to the first page owned by a thread
			// If this is the firstPage owned by a thread, set nextPage to firstPage
			if (pageIndex == threadNodeList[thread].firstPage) {
				threadNodeList[thread].firstPage = PageTable[threadNodeList[thread].firstPage].nextPage;
			}
			// If this is not firstPage, remove from LL
			else {
				int temp = threadNodeList[thread].firstPage;
				while (PageTable[temp].nextPage != -1) {
					if (PageTable[temp].nextPage == pageIndex) {
						break;
					}
					temp = PageTable[temp].nextPage;
				}
				if (PageTable[temp].nextPage == -1) {
					printf("Error on PageTable looping.\n");
					exit(EXIT_FAILURE);
				}
				// This line essentially "frees" the page
				PageTable[temp].nextPage = PageTable[pageIndex].nextPage;
			}
		}
	}
	
	return;

}

