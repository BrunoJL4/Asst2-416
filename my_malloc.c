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
static char * myBlock;

/* This is the threadNodeList */
ThreadMetadata *threadNodeList; 

/* This is the PageTable */
PageMetadata *PageTable;

/* This is the start address of the pages */
char * pages = myBlock + kernelSize; 

/* establish the base size of the kernel's space in memory */
int kernelSize = (2 * MAX_NUM_THREADS * sizeof(pnode)) // pnodes allocation + buffer
	 + (MAX_NUM_THREADS * sizeof(tcb)) // tcb allocation
	 + (sizeof(pnode *) + sizeof(tcb **)) // MLPQ & tcbList
	 + (MAX_NUM_THREADS * MEM) // stack allocations for child threads
	 + ((MAX_NUM_THREADS + 2) * sizeof(ThreadMetadata)) // threadNodeList
	 + ((TOTALMEM / PAGESIZE) * sizeof(PageMetadata)); // PageTable space, rounded up.
	
	
/* Figure out how many pages the kernel needs (floor/round-down), add eight more
pages to that, and then multiply that number of pages by PAGESIZE. That is the
actual size of the kernel block, and it now starts at the beginning of
an aligned page	*/
kernelSize = ((kernelSize/PAGESIZE) + 8) * PAGESIZE;

/* Base address: points to start of pages */
char * baseAddress = myBlock + kernelSize; 

/* End global variable declarations. */

/* malloc & free function implementations */

/** SMART MALLOC **/
void* myallocate(int bytes, char * file, int line, int req){
	printf("Beginning myallocate(), current_thread is: %d\n", current_thread);
	
	// initialize signal alarm struct
	memset(&mem_sig, 0, sizeof(mem_sig));
	
	
	//prot_none threads pages
	
	
	// INITIALIZE KERNEL AND CREATE PAGE ABSTRACTION(FIRST MALLOC))
	if(*myBlock == '\0'){
		printf("Initializing kernel space in memory.\n");

		myBlock = (char *)memalign(TOTALMEM, PAGESIZE);
		
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
			PageMetadata newData = {BLOCK_FREE, -1, -1, NULL};
			PageTable[i] = newData;

		}
		// Increase counter for memory allocated by kernel, now that we've allocated PageTable.
		PageTable[MAX_NUM_THREADS + 1].memoryAllocated += (threadPages * PAGESIZE)
		
		//protect data
		if((mprotect(myBlock, TOTALMEM, PROT_READ)) != 0){
			//error
		}
		if((mprotect(myBlock, TOTALMEM, PROT_WRITE)) != 0){
			//error
		}

	} //End of kernel setup and page creating

	
	//IF CALLED BY SCHEDULER
	if(LIBRARYREQ){
		
		//TO DO:
		//USE FREEBYTES VAR TO DETERMINE POINTER ASKED FOR
		//GLOBAL VARIABLE??
		
	}else{ //IF CALLED BY THREAD
		
		//FIND FREE SEGMENT WITHIN THREADS PAGE(S)
		int pageNum = pagethreadNodeList[thread].firstPage; //threads first abstraction page
		int virtualPageNum = 0; //this the first page that thread is under the illusion of iterating
		char * ptr = baseAddress + (PAGESIZE * pageNum);
		while(pageNum != -1){
			
			//If this segment has enough space, assign user requested space, set remaining to free, and return
			if(((SegMetadata *)ptr)->used == BLOCK_FREE){
				//does block have room for requested bytes
				if(((SegMetadata *)ptr)->size >= bytes)
					((SegMetadata *)ptr)->used = BLOCK_USED;
					//if entire block wasn't needed, set rest to free
					if(((SegMetadata *)ptr)->size > bytes + sizeof(SegMetadata)){
						
						//TO DO:
						//ITERATE AMOUNT OF PAGES TO PUT SEGMENT DATA FOR REST OF FREED BLOCK_FREE
						//nextPtr = ......
						//SegmentData nextSegment = {}
						
						
						
						
						
					}	
				}
			}
					
			ptr += ((SegMetadata *)ptr)->size + sizeof(SegMetadata);
			
			// leakage or end of block.
			if(ptr >= ptr + PAGESIZE){
				pageNum = PageTable[pageNum].nextPage; 
				virtualPageNum++;
				bytesLeaked = ptr - (pageNum*PAGESIZE) - baseAddress; 
				//iterate pageNum depending on how many bytes leaked
				for(int i = 0; i < ceil(bytesLeaked/PAGESIZE); i++){
					pageNum = PageTable[pageNum].nextPage;
					virtualPageNum++;
				}
				ptr = baseAddress + (pageNum * PAGESIZE) + (bytesLeaked%PAGESIZE);
			}
		}
		
		//FREE SEGMENT WAS NOT AVAILABLE WITHIN THREADS PAGE(S)
		//CREATE NEW PAGES TO GIVE SEGMENT
		
		if(((SegMetadata *)ptr->used == BLOCK_FREE)){
			//get amouunt of bytes at last block
		}
		//how many pages need to be allocated to handle request?
		int pagesToAllocate = ceil(bytes/PAGESIZE); //iterate this each page number of times. 
		//GET THE THREAD THESE PAGES! 
		if(pageTable[virtualPageNum + 1].used == BLOCK_USED) //virtual page is used by another thread
			//find free page to swap w/ page
			for(int i = 0; i < TOTALMEM / PAGESIZE; i++){
				if(pageTable[i].used == BLOCK_FREE){
					//found one!
					//TO DO
					//SETS MEMBERS OF PAGETABLE AND THREADNODELIST TO PROPER VALUES, 

					/*while(pageTable[page].nextPage != virtualPageNum + 1){ //if not, 
							page = pageTable[page].nextPage;
						}
					int page = threadNodeList[pageTable[virtualPageNum + 1].owner].firstPage; 
					if(page == virtualPageNum + 1){ //is page the threads first page?
						threadNodeList[pageTable[virtualPageNum + 1].owner].firstPage = i;
					}else{
						while(pageTable[page].nextPage != virtualPageNum + 1){ //if not, 
							page = pageTable[page].nextPage;
						}
					}
					pageTable[page].nextPage = i;
					pageTable[pageNum].nextPage = virtualPageNum + 1;
					
					//owner of threads virtual page is now i 
					if(virtualPageNum + 1 == 0)
						threadNodeList[pageTable[virtualPageNum + 1].owner].firstPage = i;
				

		
		
		
		
		
				/** AN OLD IMPLEMENTATON (FIRST PAGE ONLY), USE LOGIC TO COMPLETE ABOVE**/
					//owner of page 0's first page is now i 
					threadNodeList[pageTable[0].owner].firstPage = i;
					
					//page i is used and owned
					pageTable[i].used = BLOCK_USED;
					pageTable[i].owner = pageTable[0].owner;
					
					//set permissions and reallocate page's contents
					mprotect(baseAddress, PAGESIZE, PROT_READ);
					mprotect(baseAddress, PAGESIZE, PROT_WRITE);
					memcpy(baseAddress, baseAddress + (PAGESIZE * i));
					mprotect(baseAddress + (PAGESIZE * i), PAGESIZE, PROT_NONE);
						
					}
				}
			}

			//could not find a free page to give contents of page 0 to
			if(threadNodeList[current_thread].firstPage == -1) 
					return NULL;
					
			
		} 
	}
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
	//REMEMBER: change the size of the segment if you don't free the page containing the segment
	//Also may need to create a new segment for continuing pages
	
	// If the Segment is the size of the page AND it is at the start of a page, free the page (ONLY IF THIS IS NOT THE KERNEL)
	if (req == THREADREQ) {
		// If this segment is at least the size of a page, check if or how many pages need to be freed
		if ((((SegMetadata *)ptr)->size + sizeof(SegMetadata)) >= pagesize) {
			// Find out how many pages will need to be freed and the starting pageIndex
			int numPages;
			int remainder;
			SegMetadata * prev;
			if (ptr == index) {
				// We will free the first pageIndex
				// This makes things easy to find how many pages freed
				remainder = (((SegMetadata *)ptr)->size + sizeof(SegMetadata)) % PAGESIZE;
				numPages = (((SegMetadata *)ptr)->size + sizeof(SegMetadata) - remainder) / PAGESIZE;
				prev = ((SegMetadata *)ptr)->prev;
				
			} else {
				// pageIndex will be the next page IF we are freeing
				pageIndex = PageTable[pageIndex].nextPage;
				// figure out how much memory leaked over from the previous page segment
				// treat the nextPage like it was a new segment and do the same thing as the if condition above
				int overFlow = ptr - (index + PAGESIZE);
				overFlow = (((SegMetadata *)ptr)->size + sizeof(SegMetadata)) - overFlow;
				// overFlow is now the size of the segment in the continuing pages
				remainder = overFlow % PAGESIZE;
				numPages = (overFlow - remainder) / PAGESIZE;	
				prev = ptr;
				
				// IF there are pages being freed, then change the size of the segment
				if (numPages > 0) {
					((SegMetadata *)ptr->size = (sizeof(SegMetadata) + ptr) - (index + PAGESIZE);
				}
			}
			
			// index is now the memory address of the last remainder of the segment
			index += numPages * PAGESIZE;
			// IF pages will be freed, create new segment at start of last page
			if (numPages > 0 && remainder > sizeof(SegMetadata)) {
				SegMetadata newSeg = { BLOCK_FREE, remainder - sizeof(SegMetadata), prev };
				((SegMetadata *)index = newSeg;
			// IF there is not enough space for the new segment, combine it to prev
			} else if (numPages > 0) {
				((SegMetadata *)prev)->size += remainder;
			}
			
			// Loop through to the end of segment size and set pageIndex to pages you will free
			while (numPages > 0) {
				
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
				
				// increment pageIndex and decrement numPages
				pageIndex = PageTable[pageIndex].nextPage;
				numPages--;
			}
		}
	}
	
	return;

}