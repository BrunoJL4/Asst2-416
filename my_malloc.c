// name: Bruno J. Lucarelli
//       Joseph Gormley
//       Alex Marek
// username of iLab: bjl145
// iLab Server: man.cs.rutgers.edu


#include "my_malloc.h"

/* Define global variables here. */

/* The global array containing the memory we are "allocating" */
static char * myBlock;

/* This is the threadNodeList */
ThreadMetadata *threadNodeList; 

/* This is the PageTable */
PageMetadata *PageTable;

/* establish the base size of the kernel's space in memory */
int kernelSize = (2 * MAX_NUM_THREADS * sizeof(pnode)) // pnodes allocation + buffer
	 + (MAX_NUM_THREADS * sizeof(tcb)) // tcb allocation
	 + (sizeof(pnode *) + sizeof(tcb **)) // MLPQ & tcbList
	 + (MAX_NUM_THREADS * MEM) // stack allocations for child threads
	 + ((MAX_NUM_THREADS + 1) * sizeof(ThreadMetadata)) // threadNodeList
	 + ( ((TOTALMEM / PAGESIZE) + 1) * sizeof(PageMetadata)); // PageTable space, rounded up.
	
	
/* Figure out how many pages the kernel needs (floor/round-down), add eight more
pages to that, and then multiply that number of pages by PAGESIZE. That is the
actual size of the kernel block, and it now starts at the beginning of
an aligned page	*/
kernelSize = ((kernelSize/PAGESIZE) + 8) * PAGESIZE;

/* Base address: points to start of pages */
char * baseAddress = myBlock + kernelSize; 

/* Address of freed block in kernel, will be allocating kernel from L to R */
char * freeKernelPtr;

/* Number of pages left in myBlock */
int numLocalPagesleft;

/* Number of pages left in swap file */
int numSwapPagesLeft;

/* End global variable declarations. */

/* malloc & free function implementations */

/** SMART MALLOC **/
void* myallocate(int bytes, char * file, int line, int req){
	
    sigprocmask(SIG_BLOCK, SIGVTALRM, NULL);
    
    printf("Beginning myallocate(), current_thread is: %d\n", current_thread);
    
	// INITIALIZE KERNEL AND CREATE PAGE ABSTRACTION (FIRST MALLOC)
	if(*myBlock == '\0') {
		printf("Initializing kernel space in memory.\n");

		myBlock = (char *)memalign(TOTALMEM, PAGESIZE);
        freeKernelPtr = myBlock; //to keep track of free space in kernel, will do allocations from L to R

		// threadNodeList is put in the "last" space in the kernel block... each cell stores a struct, so
		// threadNodeList is set to a pointer with size enough to store all of the ThreadMetadata structs.
		// TODO @all: this had MAX_NUM_THREADS + 2 before, but I think it should only be MAX_NUM_THREADS + 1.
		// We're storing space for the max number of user threads (MAX_NUM_THREADS - 1), the main thread, and
		// the kernel thread.
		threadNodeList = (ThreadMetadata *) ((myBlock + kernelSize) - ((MAX_NUM_THREADS + 1) * sizeof(ThreadMetadata)));
		// threadNodeList[MAX_NUM_THREADS] is metadata for scheduler/kernel. 
		// Kernel's ThreadMetadata's first page is set to -2, for default kernel value.
		// So far, we've only allocated threadNodeList.
		ThreadMetadata kernelData = {-2, sizeof(threadNodeList)}
		// Copy kernelData to the kernel's cell in threadNodeList.
		threadNodeList[MAX_NUM_THREADS] = kernelData;
		int threadPages = ((TOTALMEM - kernelSize)/(PAGESIZE)) - 1;
		// Initialize the standard cells for threadNodeList.
		int i;
		for(i = 0; i < MAX_NUM_THREADS; i++) {
			// Make new ThreadMetadata struct and copy it to threadNodeList, for
			// each thread besides the kernel.
			ThreadMetadata newThreadData = {-1, threadPages};
			threadNodeList[i] = newThreaddata;
		}
		// Put PageTable before threadNodeList, allocating enough space for the metadata of remaining
		// pages in the memory. Meaning we get the address from subtracting the size of PageTable
		// from the address of threadNodeList.
		// Also, leave that last space at the end free for the swapping/free page, used
		// in signal handling.
		numLocalPagesLeft = threadPages;
		// swap file should have all pages open to start (16MB divided by 4kb)
		numSwapPagesLeft = (16000000)/PAGESIZE;
		PageTable = (PageMetadata *) (threadNodeList - (threadPages * sizeof(PageMetadata)));
		// Go through PageTable and create the structs at each space, initializing their space
		// to be FREE and having 0 space used.
		for(i = 0; i < threadPages; i++) {
			// Make new PageMetadata struct and copy it to PageTable
			PageMetadata newData = {BLOCK_FREE, -1, MAX_NUM_THREADS+1, NULL};
			PageTable[i] = newData;

		}
		// Increase counter for memory allocated by kernel, now that we've allocated PageTable.
		PageTable[MAX_NUM_THREADS].memoryAllocated += (threadPages * PAGESIZE)


	} //End of kernel setup and page creating

	
	// IF CALLED BY SCHEDULER
	if(req == LIBRARYREQ){
        // store first address of requested block
        char *ret = freeKernelPtr;
        // point freeKernelPtr to next free address in kernel block
        freeKernelPtr += bytes;    
        // increase counter for memory allocated by kernel
        PageTable[MAX_NUM_THREADS].memoryAllocated += bytes;
        // unmask interrupts and return the pointer
        sigprocmask(SIG_UNBLOCK, SIGVTALRM, NULL);
        return (void *)ret;
	}
	//IF CALLED BY THREAD
	else if(req == THREADREQ) {
		/* Part 1: Checking if thread has pages, if not, assign pages */
		// figure out how many pages the request will take
		int reqPages = ceil((bytes + sizeof(SegMetadata))/PAGESIZE);
		// The number of pages in VM
		int threadPages = ((TOTALMEM - kernelSize)/(PAGESIZE)) - 1;
		// If the current thread doesn't have a page yet:
		if(threadNodeList[current_thread].firstPage == -1) {
			//TODO @all: this will be a swap file case
			if (reqPages > numLocalPagesLeft) {
				sigprocmask(SIG_UNBLOCK, SIGVTALRM, NULL);
				return NULL;
			}
			int freePage = 0;
			while (freePage < threadPages) {
				if (PageTable[freePage].used == BLOCK_FREE) {
					break;
				}
				freePage++;
			}
			// This case shouldn't happen, but we're just being safe =^)
			if (freePage >= threadPages) {
				sigprocmask(SIG_UNBLOCK, SIGVTALRM, NULL);
				return NULL;
			}
			threadNodeList[current_thread].firstPage = freePage;
			// This next line is done by swapPages
			PageTable[freePage].used = BLOCK_USED;
			numLocalPagesLeft--;
			// Swap pages for first page to be page 0
			if (freePage != 0) {
				swapPages(0, freePage, current_thread);
			}
			// Give the first page a free segment
			SegMetadata data = { BLOCK_FREE, (PAGESIZE * reqPages) - sizeof(SegMetadata), NULL }
			(SegMetadata *)baseAddress = data;
			// Swap the rest of the pages that will be used into place
			reqPages--;
			freePage = 0;
			int replaceThisPage = 1;
			while (reqPages > 0) {
				// We now have to find the other free pages that we can use
				while (freePage < threadPages) {
					if (PageTable[freePage].used == BLOCK_FREE) {
						break;
					}
					freePage++;
				}
				// This case shouldn't happen, but we're just being safe =^)
				if (freePage >= threadPages) {
					sigprocmask(SIG_UNBLOCK, SIGVTALRM, NULL);
					return NULL;
				}
				// Tack it onto nextPage list
				PageTable[replaceThisPage - 1].nextPage = freePage;
				// Swap this page into order
				if (freePage != replaceThisPage) {
					swapPages(replaceThisPage, freePage, current_thread);
				}
				// Decrement free pages left
				numLocalPagesLeft--;
				// Replace the next page
				replaceThisPage++;
				reqPages--;
			}
		}
		
		/* Part 2: Make sure pages are in order */
		int VMPage = 0;  // where our page is supposed to be
		int ourPage = threadNodeList[current_thread].firstPage; // where our page actually is
		while (ourPage != -1) {
			// Swap pages into order
			if (VMPage != ourPage) {
				swapPages(VMPage, ourPage, current_thread);
			}
			VMPage++;
			ourPage = PageTable[ourPage].nextPage;
		}
		
		/* Part 3: Iterate for free segments and add pages if needed */
		char * ptr = baseAddress;
		char * prev = ptr;
		while (ptr < (baseAddress + (VMPage * PAGESIZE))) {
			if (((SegMetadata *)ptr)->used == BLOCK_FREE && ((SegMetadata *)ptr)->size >= bytes) {
				break;
			}
			prev = ptr;
			ptr += ((SegMetadata *)ptr)->size + sizeof(SegMetadata);
		}
		// Check if we found a segment
		if (ptr >= (baseAddress + (VMPage * PAGESIZE))) {
			// We didn't have a segment big enough
			// We need to add more pages
			// If the prev was free, increase the size of prev
			if (((SegMetadata *)prev)->used == BLOCK_FREE) {
				reqPages = ceil((bytes - ((SegMetadata *)prev)->size)/PAGESIZE);
				((SegMetadata *)prev)->size += reqPages * PAGESIZE;
				ptr = prev;
			} 
			
			// Check if we can add the number of pages needed
			if (reqPages > numLocalPagesLeft) {
				sigprocmask(SIG_UNBLOCK, SIGVTALRM, NULL);
				return NULL;
			}
			// Tack on pages to the end of nextPage list
			ourPage = VMPage - 1;
			int sizeReqPages = reqPages;
			while (reqPages > 0) {
				while (VMPage < threadPages) {
					if (PageTable[VMPage].used == BLOCK_FREE) {
						break;
					}
					VMPage++;
				}
				// This shouldn't happen but we're being safe
				if (VMPage >= threadPages) {
					sigprocmask(SIG_UNBLOCK, SIGVTALRM, NULL);
					return NULL;
				}
				PageTable[ourPage].nextPage = VMPage;
				PageTable[VMPage].used = BLOCK_USED;
				if ((ourPage + 1) != PageTable[ourPage].nextPage) {
					swapPages(ourPage + 1, PageTable[ourPage].nextPage, current_thread);
				}				
				ourPage = PageTable[ourPage].nextPage;
				reqPages--;
			}
			// Create SegMetadata if we didn't just add memory space to prev
			if (((SegMetadata *)prev)->used != BLOCK_FREE) {
				SegMetadata data = { BLOCK_FREE, sizeReqPages * PAGESIZE, prev };
				(SegMetadata *)ptr = data;
			}
		}
		
		/* Part 4: If the entire segment is not used, then create free segment after the space used */
		char * extraSeg = NULL;
		if (bytes < (((SegMetadata *)ptr)->size - (sizeof(SegMetadata) + 1))) {
			// Create extra segment that is free
			int extraSpace = bytes - (((SegMetadata *)ptr)->size - sizeof(SegMetadata));
			((SegMetadata *)ptr)->size = bytes;
			extraSeg = ptr + ((SegMetadata *)ptr)->size;
			SegMetadata data = { BLOCK_FREE, extraSpace, ptr };
			(SegMetadata *)extraSeg = data;
			
			char * nextSeg = extraSeg + sizeof(SegMetadata) + ((SegMetadata *)extraSeg)->size;
			((SegMetadata *)nextSeg)->prev = extraSeg;
		}
		// Set the ptr SegMetadata to used
		((SegMetadata *)ptr)->used = BLOCK_USED;
		
		/* Part 5: Set the parent segment fields for both the extra free space (if any) and the new segment */
		// Do this for ptr
		// Find the first child page (may not be applicable) of ptr and the number of pages ptr spans
		int childPage = ceil((ptr - baseAddress)/PAGESIZE);
		
		int memoryBeforPtr = ptr % PAGESIZE
		int memoryAfterPtr = PAGESIZE - memoryBeforPtr;
		int memoryOnPage = memoryAfterPtr - sizeof(SegMetadata);
		int memoryOverflow = ((SegMetadata *)ptr)->size - memoryOnPage;
		int numChildren = ceil(memoryOverflow / PAGESIZE);
		
		while (numChildren > 0) {
			PageTable[childPage].parentSegment = ptr + sizeof(SegMetadata);
			childPage++;
			numChildren--;
		}
		
		// Repeat the logic above for extraSeg if it exists
		if (extraSeg != NULL) {
			childPage = ceil((extraSeg - baseAddress)/PAGESIZE);
		
			memoryBeforPtr = extraSeg % PAGESIZE
			memoryAfterPtr = PAGESIZE - memoryBeforPtr;
			memoryOnPage = memoryAfterPtr - sizeof(SegMetadata);
			memoryOverflow = ((SegMetadata *)extraSeg)->size - memoryOnPage;
			numChildren = ceil(memoryOverflow / PAGESIZE);
			
			while (numChildren > 0) {
				PageTable[childPage].parentSegment = extraSeg + sizeof(SegMetadata);
				childPage++;
				numChildren--;
			}
		}
		
		/* Part 6: return pointer to user and end sigprocmask =^) */
		sigprocmask(SIG_UNBLOCK, SIGVTALRM, NULL);
		return ptr + sizeof(SegMetadata);
}

/** Smart Free **/
void mydeallocate(void *ptr, char *file, int line, int req){
	sigprocmask(SIG_BLOCK, SIGVTALRM, NULL);
	
	/* Part 1: Make sure pages are in order */
	int VMPage = 0;  // where our page is supposed to be
	int ourPage = threadNodeList[current_thread].firstPage; // where our page actually is
	while (ourPage != -1) {
		// Swap pages into order
		if (VMPage != ourPage) {
			swapPages(VMPage, ourPage, current_thread);
		}
		VMPage++;
		ourPage = PageTable[ourPage].nextPage;
	}
	
	/* Some declarations*/	
	int thread;
	int pageIndex
	char *index;
	int pagesize;	
	//Set ptr to point to its SegMetadata
	ptr = ptr - sizeof(SegMetadata);
	
	
	/* Part 2: Set values for thread, pageIndex, index, and pagesize based on it being user or kernel threadspace */
	if(req == THREADREQ) {
		thread = current_thread;
		printf("Beginning mydeallocate for thread: %d\n", current_thread);
	
		// Find the page of the memory address the user is trying to free
		pageIndex = threadNodeList[thread].firstPage;
		while (pageIndex != -1) {
			// Index to the first memory address of pageIndex
			index = baseAddress + (pageIndex * PAGESIZE)
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
		pagesize = kernelSize;
	}
	else {
		printf("Error! Invalid value for req: %d\n", req);
		exit(EXIT_FAILURE);
	}	

	
	/* Part 3: check for segfault */
	// If this is not a proper segment (freed or !SegMetadata), then Segfault	
	if (ptr < &myBlock || ptr >= &myBlock + sizeof(myBlock)) {
		fprintf(stderr, "Segfault! - File: %s, Line: %d.\n", file, line);
		exit(EXIT_FAILURE);
	}
	if (((SegMetadata *)ptr)->used != BLOCK_USED) {
		fprintf(stderr, "Segfault! - File: %s, Line: %d.\n", file, line);
		exit(EXIT_FAILURE);
	}
	
	/* Part 4: free the segment */	
	((SegMetadata *)ptr)->used = BLOCK_FREE;
	// Should we wipe the memory for this segment? Do it here if so
	
	
	/* Part 5: Combine segment with next segment if applicable or free */	
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
				char * endIndex = baseAddress + (endPageIndex * PAGESIZE)
				// If the address the user is searching for belongs to this page, then break
				if (nextPtr >= endIndex && nextPtr < (endPageIndex + PAGESIZE)) {
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
		if ((ptr + ((SegMetadata *)ptr)->size + sizeof(SegMetadata)) < (index + kernelSize)) {
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
			
			// Set ptr to the front of the new free space
			ptr = (char *)prevPtr;
		}
	}
	

	/* Part 7: Free unused pages at the end of thread memory */
	// Only free pages if this is the user space
	if (req == THREADREQ) {
		// find some variables
		char * nextPtr = ptr + sizeof(SegMetadata) + ((SegMetadata *)ptr)->size;
		int lastPage = threadNodeList[thread].firstPage;
		while (PageTable[lastPage].nextPage != -1) {
			lastPage = PageTable[lastPage].nextPage;
		}
		char * outOfBounds = baseAddress + (lastPage * PAGESIZE) + PAGESIZE;
		// If ptr is the last segment AND If ptr size is greater than or equal to a PAGESIZE, continue
		if (nextPtr >= outOfBounds && (((SegMetadata *)ptr)->size + sizeof(SegMetadata)) >= PAGESIZE) {
			// If ptr is the start of a page, free that page and onward
			if (ptr % PAGESIZE == 0) {
				// Remove all nextPage links
				int indexer = (ptr - baseAddress)/PAGESIZE;
				int after;
				while (PageTable[indexer].nextPage != -1) {
					after = indexer;
					PageTable[indexer].used = BLOCK_FREE;
					mprotect(baseAddress + (PAGESIZE * indexer), PAGESIZE, PROT_NONE);
					indexer = PageTable[indexer].nextPage;
					PageTable[after].nextPage = -1;
				}
			}
			// If ptr is not the start of a page, free the pages afterwards and reduce the size of segment
			else {
				// Remove all nextPage links
				int indexer = ceil((ptr - baseAddress)/PAGESIZE);
				int after;
				while (PageTable[indexer].nextPage != -1) {
					after = indexer;
					PageTable[indexer].used = BLOCK_FREE;
					mprotect(baseAddress + (PAGESIZE * indexer), PAGESIZE, PROT_NONE);
					indexer = PageTable[indexer].nextPage;
					PageTable[after].nextPage = -1;
				}
				// Reduce the size of segment
				int pageOutOfBounds = ceil((ptr - baseAddress)/PAGESIZE);
				char * lastAddress = baseAddress + (PAGESIZE * pageOutOfBounds) - 1;
				((SegMetadata *)ptr)->size = lastAddress - (ptr + sizeof(SegMetadata));
			}	
			// Special case if this is the first page
			if (ptr == baseAddress) {			
				threadNodeList[thread].firstPage = -1;
			}
		}
	}
	
	sigprocmask(SIG_UNBLOCK, SIGVTALRM, NULL);
	
	return;

}
