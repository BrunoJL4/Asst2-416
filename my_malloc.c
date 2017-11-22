// name: Bruno J. Lucarelli
//       Joseph Gormley
//       Alex Marek
// username of iLab: bjl145
// iLab Server: man.cs.rutgers.edu


#include "my_malloc.h"

/* Define global variables here. */

/* The global array containing the memory we are "allocating" */

// This is externed
char *myBlock = NULL;

/* This is the threadNodeList */
ThreadMetadata *threadNodeList; 

/* This is the PageTable */
PageMetadata *PageTable;

/* establish the base size of the kernel's space in memory */
int kernelSize;

/* Base address: points to start of pages */
char * baseAddress; 

/* Number of pages left in myBlock */
int numLocalPagesLeft;

/* Number of pages left in swap file */
int numSwapPagesLeft;

/* Tells us whether or not we are currently running a memory manager function. */
int memory_manager_active;

/* Global telling us how many pages each thread is allowed, at max. */
int maxThreadPages;


/* End global variable declarations. */

/* malloc & free function implementations */

/** SMART MALLOC **/
void* myallocate(int bytes, char * file, int line, int req){

	sigset_t signal;
	sigemptyset(&signal);
	sigaddset(&signal, SIGVTALRM);

	sigprocmask(SIG_BLOCK, &signal, NULL);
	memory_manager_active = 1;
    
	// INITIALIZE KERNEL AND CREATE PAGE ABSTRACTION (FIRST MALLOC)
	if(myBlock == NULL) {
		
		kernelSize = (2 * MAX_NUM_THREADS * sizeof(pnode)) // pnodes allocation + buffer
			+ (2 * MAX_NUM_THREADS * sizeof(tcb)) // tcb allocation
			+ (sizeof(pnode *) + sizeof(tcb **)) // MLPQ & tcbList
			+ (MAX_NUM_THREADS * MEM) // stack allocations for child threads
			+ ((MAX_NUM_THREADS + 1) * sizeof(ThreadMetadata)) // threadNodeList
			+ ( ((TOTALMEM / PAGESIZE) + 1) * sizeof(PageMetadata)); // PageTable space, rounded up.
			
		/* Figure out how many pages the kernel needs (floor/round-down), add eight more
		pages to that, and then multiply that number of pages by PAGESIZE. That is the
		actual size of the kernel block, and it now starts at the beginning of
		an aligned page	*/
		kernelSize = ((kernelSize/PAGESIZE) + 16) * PAGESIZE;
		
		maxThreadPages = ((TOTALMEM - kernelSize)/(PAGESIZE)) - 1;

		posix_memalign((void**)&myBlock, PAGESIZE, TOTALMEM);
		if(myBlock == NULL) {
			printf("Error, memalign didn't work.\n");
			exit(EXIT_FAILURE);
		}

		baseAddress = myBlock + kernelSize; 
		// threadNodeList is put in the "last" space in the kernel block... each cell stores a struct, so
		// threadNodeList is set to a pointer with size enough to store all of the ThreadMetadata structs.
		// TODO @all: this had MAX_NUM_THREADS + 2 before, but I think it should only be MAX_NUM_THREADS + 1.
		// We're storing space for the max number of user threads (MAX_NUM_THREADS - 1), the main thread, and
		// the kernel thread.
		threadNodeList = (ThreadMetadata *) ((myBlock + kernelSize) - ((MAX_NUM_THREADS + 1) * sizeof(ThreadMetadata)));
		// threadNodeList[MAX_NUM_THREADS] is metadata for scheduler/kernel. 
		// Kernel's ThreadMetadata's first page is set to -2, for default kernel value.
		// So far, we've only allocated threadNodeList.
		ThreadMetadata kernelData = {-2, (MAX_NUM_THREADS + 1)*sizeof(ThreadMetadata)};
		// Copy kernelData to the kernel's cell in threadNodeList.
		threadNodeList[MAX_NUM_THREADS] = kernelData;
		// Initialize the standard cells for threadNodeList.
		int i;
		for(i = 0; i < MAX_NUM_THREADS; i++) {
			// Make new ThreadMetadata struct and copy it to threadNodeList, for
			// each thread besides the kernel.
			ThreadMetadata newThreadData = {-1, maxThreadPages};
			threadNodeList[i] = newThreadData;
		}
		// Put PageTable before threadNodeList, allocating enough space for the metadata of remaining
		// pages in the memory. Meaning we get the address from subtracting the size of PageTable
		// from the address of threadNodeList.
		// Also, leave that last space at the end free for the swapping/free page, used
		// in signal handling.
		numLocalPagesLeft = maxThreadPages;
		// swap file should have all pages open to start (16MB divided by 4kb)
		numSwapPagesLeft = (16000000)/PAGESIZE;
		PageTable = (PageMetadata *) ((char *)threadNodeList - (maxThreadPages * sizeof(PageMetadata)));
		// Go through PageTable and create the structs at each space, initializing their space
		// to be FREE and having 0 space used.
		for(i = 0; i < maxThreadPages; i++) {
			// Make new PageMetadata struct and copy it to PageTable
			PageMetadata newData = {BLOCK_FREE, -1, MAX_NUM_THREADS+1};
			PageTable[i] = newData;

		}
		// manually protect every page in user space, by default
		for(i = 0; i < maxThreadPages; i++) {
			char * currAddress = baseAddress + (i * PAGESIZE);
			if(mprotect(currAddress, PAGESIZE, PROT_NONE) == -1) {
				printf("ERROR! mprotect operation didn't work!\n");
				exit(EXIT_FAILURE);
			}
		}
		// Increase counter for memory allocated by kernel, now that we've allocated PageTable.
		//PageTable[MAX_NUM_THREADS].memoryAllocated += (maxThreadPages * PAGESIZE)
		// get size of the first segment
		int firstSize = (char*) PageTable - (myBlock + sizeof(SegMetadata));
		// set first SegMetadata
		SegMetadata data = {BLOCK_FREE, firstSize, NULL};
		// add first segment for myBlock here
		*((SegMetadata*)myBlock) = data;
	} //End of kernel setup and page creating

	
	// IF CALLED BY SCHEDULER
	if(req == LIBRARYREQ){
		// get the first segment metadata (at the very beginning of myBlock)
		char *currData = myBlock;
		// iterate by pointer and size until we find a free segment big enough for
		// the allocation... don't go up to or past PageTable
		while(currData < (char*)PageTable) {
			if(((SegMetadata *)currData)->used == BLOCK_FREE && ((SegMetadata *)currData)->size >= bytes) {
				break;
			}
			currData += ((SegMetadata *)currData)->size + sizeof(SegMetadata);
		}
		// this shouldn't happen
		if(currData >= (char*) PageTable) {
			printf("ERROR! Trying to allocate data for the scheduler past PageTable's space.\n");
			sigprocmask(SIG_UNBLOCK, &signal, NULL);
			return NULL;
		}
		// save that free segment's size, change the attribute to match bytes
		int oldSegSize = ((SegMetadata *)currData)->size;
		// set char *ret to match the actual first address of that segment
		((SegMetadata *)currData)->size = bytes;
		// set the return segment to BLOCK_USED
		((SegMetadata *)currData)->used = BLOCK_USED;
		// don't change currData's prev, because that's already correct
		// if there's enough leftover space for the allocation, a NEW
		// segment's metadata, AND at least one byte, create the new segment
		if(oldSegSize > bytes + sizeof(SegMetadata) + 1) {
			// create a new SegMetadata following that, which goes from
			// the following/next free address to the address of PageTable
			// in regards to size 
			char *newData = currData + sizeof(SegMetadata) + ((SegMetadata *)currData)->size;
			// size of the next segment will be oldSegSize - bytes - sizeof(SegMetadata)
			SegMetadata data = {BLOCK_FREE, oldSegSize - (bytes + sizeof(SegMetadata)), (SegMetadata *)currData};
			*((SegMetadata *) newData) = data;
			char *nextData = newData + sizeof(SegMetadata) + ((SegMetadata *)newData)->size;
			if( (char*) nextData < (char*) PageTable ) {
				((SegMetadata *)nextData)->prev = (SegMetadata *)newData; 
			}
		}
	        // increase counter for memory allocated by kernel
	        // PageTable[MAX_NUM_THREADS].memoryAllocated += bytes;
	        memory_manager_active = 0;
	        // unmask interrupts and return the pointer
	        sigprocmask(SIG_UNBLOCK, &signal, NULL);
	        return (void *)(currData + sizeof(SegMetadata));
	}
	//IF CALLED BY THREAD
	else if(req == THREADREQ) {
		/* Part 1: Checking if thread has pages, if not, assign pages */
		// figure out how many pages the request will take
		int reqPages = ceil(((double)bytes + sizeof(SegMetadata))/PAGESIZE);

		// check if we have enough pages left in the thread's space to accomodate
		// the request (total pages... could still not have a big enough segment,
		// since pages could be fragmented)
		if(threadNodeList[current_thread].pagesLeft < reqPages) {
			memory_manager_active = 0;
			printf("Thread %d is not allowed to allocate more pages!\n", current_thread);
			sigprocmask(SIG_UNBLOCK, &signal, NULL);
			return NULL;
		}
		// The number of pages in VM is maxThreadPages
		// If the current thread doesn't have a page yet:
		if(threadNodeList[current_thread].firstPage == -1) {
			//TODO @all: this will be a swap file case if all local pages allocated
			if (reqPages > numLocalPagesLeft) {
				memory_manager_active = 0;
				printf("Thread %d can't get enough pages in virtual memory! Requested %d pages.\n", current_thread, reqPages);
				sigprocmask(SIG_UNBLOCK, &signal, NULL);
				return NULL;
			}
			int freePage = 0;
			// iterate through until we find a free page to store our data
			while (freePage < maxThreadPages) {
				if (PageTable[freePage].used == BLOCK_FREE) {
					break;
				}
				freePage++;
			}
			// This case shouldn't happen, but we're just being safe =^)
			if (freePage >= maxThreadPages) {
				memory_manager_active = 0;
				printf("ERROR in my_allocate! freePage >= maxThreadPages, 1!\n", current_thread);
				sigprocmask(SIG_UNBLOCK, &signal, NULL);
				return NULL;
			}
			// set the current thread's first page to that free page
			threadNodeList[current_thread].firstPage = freePage;
			// This next line is done by swapPages
			PageTable[freePage].used = BLOCK_USED;
			// Set the page's new owner to current thread
			PageTable[freePage].owner = current_thread;
			// Set the page's next page to -1 by default
			PageTable[freePage].nextPage = -1;
			// unprotect the thread's new first page
			if(mprotect(baseAddress + (freePage * PAGESIZE), PAGESIZE, PROT_READ|PROT_WRITE) == -1) {
				printf("ERROR! mprotect operation didn't work!\n");
				exit(EXIT_FAILURE);
			}
			// subtract 1 from the thread's remaining pages
			threadNodeList[current_thread].pagesLeft -= 1;
			numLocalPagesLeft -= 1;
			// Swap pages for first page to be page 0, ownerships swapped
			// as well if applicable
			if (freePage != 0) {
				swapPages(0, freePage, current_thread);
			}
			// Give the first page a free segment
			SegMetadata data = { BLOCK_FREE, (PAGESIZE * reqPages) - sizeof(SegMetadata), NULL };
			*((SegMetadata *)baseAddress) = data;
			// Swap the rest of the pages that will be used into place
			reqPages -= 1;
			freePage = 0;
			int replaceThisPage = 1;
			while (reqPages > 0) {
				// We now have to find the other free pages that we can use
				while (freePage < maxThreadPages) {
					if (PageTable[freePage].used == BLOCK_FREE) {
						break;
					}
					freePage++;
				}
				// This case shouldn't happen, but we're just being safe =^)
				if (freePage >= maxThreadPages) {
					memory_manager_active = 0;
					printf("ERROR in my_allocate! thread %d freePage >= maxThreadPages, 2!\n", current_thread);
					sigprocmask(SIG_UNBLOCK, &signal, NULL);
					return NULL;
				}			
				// Set the new page's attributes
				PageTable[freePage].used = BLOCK_USED;
				PageTable[freePage].owner = current_thread;
				PageTable[freePage].nextPage = -1;
				// Unprotect the new page
				if(mprotect(baseAddress + (freePage * PAGESIZE), PAGESIZE, PROT_READ|PROT_WRITE) == -1) {
					printf("ERROR! mprotect operation didn't work!\n");
					exit(EXIT_FAILURE);
				}
				// Set the pages left for the thread
				threadNodeList[current_thread].pagesLeft -= 1;
				// Tack it onto nextPage list
				PageTable[replaceThisPage - 1].nextPage = freePage;
				// Swap this page into order
				if (freePage != replaceThisPage) {
					swapPages(replaceThisPage, freePage, current_thread);
				}		
				// Decrement free pages left
				numLocalPagesLeft -= 1;
				// Replace the next page
				replaceThisPage += 1;
				reqPages -= 1;
			}
		}
		
		
		
		/* Part 2: Make sure pages are in order */
		// where our page is supposed to be
		int VMPage = 0;
		// where our page actually is
		int ourPage = threadNodeList[current_thread].firstPage;
		while (ourPage != -1) {
			// Swap pages into order
			if (VMPage != ourPage) {
				swapPages(VMPage, ourPage, current_thread);
			}
			VMPage++;
			ourPage = PageTable[ourPage].nextPage;
		}
		// now VMPage points to the first VM address outside of the current thread's
		// reach, and ourPage points to -1 
		/* Part 3: Iterate for free segments and add pages if needed */
		char * ptr = baseAddress;
		char * prev = ptr;
		// find a free segment the thread owns, that is large enough to accomodate the request,
		// AND get the previous segment to that
		while (ptr < (baseAddress + (VMPage * PAGESIZE))) {
			if (((SegMetadata *)ptr)->used == BLOCK_FREE && ((SegMetadata *)ptr)->size >= bytes) {
				break;
			}
			
			prev = ptr;
			ptr += ((SegMetadata *)ptr)->size + sizeof(SegMetadata);
		}
		// Check if we didn't find a segment (ptr went out of bounds)
		if ( (ptr != baseAddress) && (ptr >= (baseAddress + (VMPage * PAGESIZE))) ) {
			// We didn't have a segment big enough
			// We need to add more pages
			// If the prev was free, increase the size of prev
			if (((SegMetadata *)prev)->used == BLOCK_FREE) {
				reqPages = ceil((((double)bytes) - ((SegMetadata *)prev)->size)/PAGESIZE);
				if (reqPages <= numLocalPagesLeft) {
					((SegMetadata *)prev)->size += reqPages * PAGESIZE;
				}
				ptr = prev;
			} 
			// Don't need to say else for assigning reqPages because it was determined earlier
			
			// Check if we can add the number of pages needed
			if (reqPages > numLocalPagesLeft) {
				memory_manager_active = 0;
				printf("Thread %d reqPages > numLocalPagesLeft! Requested %d pages.\n", current_thread, reqPages);
				sigprocmask(SIG_UNBLOCK, &signal, NULL);
				return NULL;
			}
			// VMPage points to the first page that the thread doesn't actually own.
			// we set ourPage to be the last page that the thread DOES own.
			// Tack on pages to the end of nextPage list
			ourPage = VMPage - 1;
			// save the total number of requested pages for later, as we decrement
			// reqPages
			int sizeReqPages = reqPages;
			// look for enough pages in virtual memory to satisfy the request
			while (reqPages > 0) {
				while (VMPage < maxThreadPages) {
					// when we find a free page in VM, break
					if (PageTable[VMPage].used == BLOCK_FREE) {
						break;
					}
					// if we didn't find a free page in VM, increment
					VMPage++;
				}
				// If VMPage is out of bounds, we don't have enough memory contiguous
				// within the thread's own virtual memory for the request
				if (VMPage >= maxThreadPages) {
					memory_manager_active = 0;
					printf("ERROR in my_allocate! thread %d VMPage >= maxThreadPages, not enough contiguous memory!\n", current_thread);
					sigprocmask(SIG_UNBLOCK, &signal, NULL);
					return NULL;
				}
				// set the new, VMPage's attributes, and link it to current thread's list
				PageTable[ourPage].nextPage = VMPage;
				PageTable[VMPage].owner = current_thread;
				PageTable[VMPage].nextPage = -1;
				PageTable[VMPage].used = BLOCK_USED;
				// decrement the current thread's number of pages
				threadNodeList[current_thread].pagesLeft -= 1;
				// decrement number of global pages left
				numLocalPagesLeft -= 1;
				// Unprotect the new, VMPage
				if(mprotect(baseAddress + (VMPage * PAGESIZE), PAGESIZE, PROT_READ|PROT_WRITE) == -1) {
					printf("ERROR! mprotect operation didn't work!\n");
					exit(EXIT_FAILURE);
				}
				// if the new VM page we found was one that isn't in its proper place yet,
				// we swap it into there (the first address following ourPage)
				if ((ourPage + 1) != PageTable[ourPage].nextPage) {
					swapPages(ourPage + 1, PageTable[ourPage].nextPage, current_thread);
				}				
				ourPage = PageTable[ourPage].nextPage;
				reqPages -= 1;
			}
			// Create SegMetadata if we didn't just add memory space to prev...
			// in the space provided, we need to consider the size of SegMetadata that ptr takes up in
			// its initial space. recent fix.
			if (((SegMetadata *)prev)->used == BLOCK_USED) {
				SegMetadata data = { BLOCK_FREE, (sizeReqPages * PAGESIZE - sizeof(SegMetadata)), (SegMetadata *)prev };
				*((SegMetadata *)ptr) = data;
			}
		}
		
		/* Part 4: If the entire segment is not used, then create free segment after the space used */
		char * extraSeg = NULL;
		if (bytes < (int) ( ((SegMetadata *)ptr)->size - (sizeof(SegMetadata) + 1) ) ) {
			// Create extra segment that is free... the extra space (for the new seg) is equal to the size
			// of the segment, minus the size of the user's allocation, minus the size of another SegMetadata
			int extraSpace = ((SegMetadata*)ptr)->size - bytes - sizeof(SegMetadata);
			//int extraSpace = bytes - (((SegMetadata *)ptr)->size - sizeof(SegMetadata));
			// set the old pointer's size to bytes
			((SegMetadata *)ptr)->size = bytes;
			// set the beginning of extraSeg's metadata to be after ptr's allocation
			extraSeg = ptr + ((SegMetadata *)ptr)->size + sizeof(SegMetadata);
			SegMetadata data = { BLOCK_FREE, extraSpace, (SegMetadata *)ptr };
			*((SegMetadata *)extraSeg) = data;
			// Check if next is out of bounds
			if (((char *)extraSeg + sizeof(SegMetadata) + ((SegMetadata *)extraSeg)->size) >= (char *)(myBlock + (TOTALMEM - PAGESIZE) )) {
				char * nextSeg = (char *)extraSeg + sizeof(SegMetadata) + ((SegMetadata *)extraSeg)->size;
				// Check if next belongs to our thread
				if (PageTable[(((char *)nextSeg - (char *)baseAddress)/PAGESIZE)].owner == current_thread) {
					// create a struct that's at the head of nextSeg
					// next seg is the segment after extraSeg. We need to tell nextSeg that its prev has changed
					((SegMetadata *)nextSeg)->prev = (SegMetadata *)extraSeg;
				}
			}
		}
		// Set the ptr SegMetadata to used
		((SegMetadata *)ptr)->used = BLOCK_USED;
		
		/* Part 5: return pointer to user and end sigprocmask =^) */
		memory_manager_active = 0;
		sigprocmask(SIG_UNBLOCK, &signal, NULL);
		return ptr + sizeof(SegMetadata);
	}
}

/** Smart Free **/
void mydeallocate(void *ptr, char *file, int line, int req){
	sigset_t signal;
	sigemptyset(&signal);
	sigaddset(&signal, SIGVTALRM);

	sigprocmask(SIG_BLOCK, &signal, NULL);
	memory_manager_active = 1;
	
	/* Part 1: Make sure pages are in order , IF it's a user request */
	int VMPage = 0;  // where our page is supposed to be
	if(req == THREADREQ) {
		int ourPage = threadNodeList[current_thread].firstPage; // where our page actually is
		while (ourPage != -1) {
			// Swap pages into order
			if (VMPage != ourPage) {
				swapPages(VMPage, ourPage, current_thread);
			}
		VMPage++;
		ourPage = PageTable[ourPage].nextPage;
	}
	
	}
	/* Some declarations*/	
	int thread;
	int pageIndex;
	char *index;
	int pagesize;
	int origPtr;
	int origSize;

	//Set ptr to point to its SegMetadata
	ptr = ptr - sizeof(SegMetadata);
	
	
	/* Part 2: Set values for thread, pageIndex, index, and pagesize based on it being user or kernel threadspace */
	if(req == THREADREQ) {
		thread = current_thread;
	
		// Find the page of the memory address the user is trying to free
		pageIndex = threadNodeList[thread].firstPage;
		while (pageIndex != -1) {
			// Index to the first memory address of pageIndex
			index = baseAddress + (pageIndex * PAGESIZE);
			// If the address the user wishes to free belongs to this page, then break
			if ((char *)ptr >= (char *)index && (char *)ptr < (char *)(index + PAGESIZE)) {
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
		thread = MAX_NUM_THREADS;
		index = myBlock;
		pagesize = kernelSize;
	}
	else {
		printf("Error! Invalid value for req: %d\n", req);
		exit(EXIT_FAILURE);
	}	

	
	/* Part 3: check for segfault */
	// If this is not a proper segment (freed or !SegMetadata), then Segfault	
	if ((char *)ptr < (char *)myBlock || (char *)ptr >= (char *)myBlock + (TOTALMEM - PAGESIZE)) {
		fprintf(stderr, "Segfault! - File: %s, Line: %d.\n", file, line);
		exit(EXIT_FAILURE);
	}
	if (((SegMetadata *)ptr)->used != BLOCK_USED) {
		fprintf(stderr, "Segfault! - File: %s, Line: %d.\n", file, line);
		exit(EXIT_FAILURE);
	}
	
	/* Part 4: free the segment */	
	((SegMetadata *)ptr)->used = BLOCK_FREE;
	
	
	// 4a: Determines how many pages the user's thread gets back from freeing block
	// Check to see if the segment begins on the start of a page
	if(ptr == baseAddress + (pageIndex * PAGESIZE)){
		// (segSize/PAGESIZE) floored when forced to int, this is intended
		int segSize = ((SegMetadata *)ptr)->size;
		threadNodeList[thread].pagesLeft += ((segSize + sizeof(SegMetadata))/PAGESIZE); 
	}
	// Segment begins in middle of page
	else{
		// Get amount of bytes till end of page
		int bytesTillEnd = (baseAddress + ((pageIndex + 1) * PAGESIZE)) - (char *)ptr;
		// (Block size - bytesTillEnd)/PAGESIZE floored when forced to int, this is intended
		threadNodeList[thread].pagesLeft += ((((SegMetadata *)ptr)->size - bytesTillEnd)/PAGESIZE);	
	}
	
	// 4b: Determine how many pages the user's thread gets back from the next block
	char * nextPtr = ptr + ((SegMetadata *)ptr)->size + sizeof(SegMetadata);
	// Is the block free? 
	if(((SegMetadata *)nextPtr)->used == BLOCK_FREE){
		char * endOfNextPtr = nextPtr + ((SegMetadata *)nextPtr)->size + sizeof(SegMetadata);
		// Check to see if free block goes until end of page (To count pages towards threads pagesLeft)
		if(endOfNextPtr == baseAddress + ((pageIndex + 1) * PAGESIZE)){
			threadNodeList[thread].pagesLeft ++;
		}
	} 		
	

	/* Part 5: Combine segment with next segment if applicable or free */	
	// This is a user space combine, so we need to check if the next segment is in a page that belongs to this thread
	if (req == THREADREQ) {
		// Check if it is in bounds of myBlock
		if (((char *)ptr + ((SegMetadata *)ptr)->size + sizeof(SegMetadata)) < (char *)(myBlock + (TOTALMEM - PAGESIZE))) {	
			// START OF CHECK
			// First check if you'll stay in pages belonging to the thread
			// Find the page of the memory
			int endPageIndex = threadNodeList[thread].firstPage;
			while (endPageIndex != -1) {
				// Index to the first memory address of pageIndex
				char * endIndex = baseAddress + (endPageIndex * PAGESIZE);
				// If the address the user is searching for belongs to this page, then break
				if (nextPtr >= endIndex && nextPtr < endIndex + PAGESIZE) {
					break;
				}			
				endPageIndex = PageTable[endPageIndex].nextPage;
			}
			// If pageIndex is -1, then we have a pagefault
			if (endPageIndex != -1) {
				// Confirmed next seg belongs to thread's page, so combine if free
				if (((SegMetadata *)nextPtr)->used == BLOCK_FREE) {
					
					((SegMetadata *)ptr)->size += sizeof(SegMetadata) + ((SegMetadata *)nextPtr)->size;
					// If we are wiping memory, wipe memory of nextPtr SegMetadata here
					
				}
			}
		}
	}
	// This is a kernel space combine, so we don't need to check the spanning of multiple pages
	else {
		if (((char *)ptr + ((SegMetadata *)ptr)->size + sizeof(SegMetadata)) < (char *)(index + kernelSize)) {
			char * nextPtr = ptr + ((SegMetadata *)ptr)->size + sizeof(SegMetadata);
			if (((SegMetadata *)nextPtr)->used == BLOCK_FREE) {
				((SegMetadata *)ptr)->size += sizeof(SegMetadata) + ((SegMetadata *)nextPtr)->size;
				// If we are wiping memory, wipe memory of nextPtr SegMetadata here
				
			}
		}
	}
	
	/* Part 6: Combine segment with previous segment if free */	
	// If prev block is free, combine
	if (((SegMetadata *)ptr)->prev != NULL) {
		SegMetadata * prevPtr = ((SegMetadata *)ptr)->prev;
		if (prevPtr->used == BLOCK_FREE) {
			prevPtr->size += sizeof(SegMetadata) + ((SegMetadata *)ptr)->size;
			// If we are wiping memory, wipe memory of ptr SegMetadata here 
			
			// If this free's up another page, we need to pagesLeft ++;
			// Only scenario this can happen is if ptr starts mid-page
			// and the prev block (which is free) starts before the start of 
			// ptr's page;
			char * startOfPtrPage = baseAddress + (pageIndex * PAGESIZE);
			if(startOfPtrPage < (char *)ptr && startOfPtrPage > nextPtr){
				threadNodeList[thread].pagesLeft++;
			}
			// Set ptr to the front of the new free space
			ptr = (char *)prevPtr;
		}
	}
	

	/* Part 7: Free unused pages at the end of thread memory */
	// Only free pages if this is the user space
	if (req == THREADREQ) {
		// find some variables
		char * nextPtr = ptr + ((SegMetadata *)ptr)->size + sizeof(SegMetadata);
		int lastPage = threadNodeList[thread].firstPage;
		while (PageTable[lastPage].nextPage != -1) {
			lastPage = PageTable[lastPage].nextPage;
		}
		char * outOfBounds = baseAddress + (lastPage * PAGESIZE) + PAGESIZE;
		// If ptr is the last segment AND If ptr size is greater than or equal to a PAGESIZE, continue
		if (nextPtr >= outOfBounds && (((SegMetadata *)ptr)->size + sizeof(SegMetadata)) >= PAGESIZE) {
			// If ptr is the start of a page, free that page and onward
			if ( ((long int) ( (char*) ptr)) % PAGESIZE == 0) {
				// Remove all nextPage links
				int indexer = (((long int) ((char*) ptr)) - ((long int)baseAddress))PAGESIZE;
				//int indexer = pageIndex; //This may be more proper
				int after;
				while (PageTable[indexer].nextPage != -1) {
					after = indexer;
					// set the current block to free
					PageTable[indexer].used = BLOCK_FREE;
					printf("Page being set to free is:%d\n", indexer);
					
					// set the owner to -1
					PageTable[indexer].owner = -1;
					// increment the number of pages left in global VM
					numLocalPagesLeft += 1;
					// protect the current block
					if( mprotect(baseAddress + (PAGESIZE * indexer), PAGESIZE, PROT_NONE) == -1) {
						printf("ERROR! mprotect operation didn't work!\n");
						exit(EXIT_FAILURE);
					}
					indexer = PageTable[indexer].nextPage;
					PageTable[after].nextPage = -1;
				}
			}
			// If ptr is not the start of a page, free the pages afterwards and reduce the size of allocated segment
			else {
				// Remove all nextPage links
				int indexer = ceil( ( ((char*) ptr) - baseAddress) / PAGESIZE);
				int after;
				while (PageTable[indexer].nextPage != -1) {
					after = indexer;
					// set the current block to free
					PageTable[indexer].used = BLOCK_FREE;
					// set the owner to -1
					PageTable[indexer].owner = -1;
					// increment the number of pages left in global VM
					numLocalPagesLeft += 1;
					// protect the current block
					if( mprotect(baseAddress + (PAGESIZE * indexer), PAGESIZE, PROT_NONE) == -1) {
						printf("ERROR! mprotect operation didn't work!\n");
						exit(EXIT_FAILURE);
					}
					indexer = PageTable[indexer].nextPage;
					PageTable[after].nextPage = -1;
				}
				// Reduce the size of segment
				int pageOutOfBounds = ceil( ( (char *)ptr - baseAddress )/PAGESIZE);
				char * lastAddress = baseAddress + (PAGESIZE * pageOutOfBounds) - 1;
				((SegMetadata *)ptr)->size = lastAddress - ( ((char *)ptr) + sizeof(SegMetadata) );
			}	
			// Special case if this is the first page
			if (ptr == baseAddress) {			
				threadNodeList[thread].firstPage = -1;
			}
		}
	}
	memory_manager_active = 0;
	sigprocmask(SIG_UNBLOCK, &signal, NULL);
	
	return;

}

/** Ceiling function - performs the ceil operation on a rational number**/
int ourCeil(double num){
	// if num - a roundedDown(num) outputes a decimal
	// higher than 0, round up, else round down and return. 
	return num - (int)(num) > 0? (int)(num + 1) : (int)(num);
	
}

