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

/* End global variable declarations. */

/* malloc & free function implementations */

/** SMART MALLOC **/
void* myallocate(int bytes, char * file, int line, int req){
	
    sigprocmask(SIG_BLOCK, SIGVTALRM, NULL);
    
    printf("Beginning myallocate(), current_thread is: %d\n", current_thread);
	
	// SET THREAD'S PAGES TO PROT_WRITE/READ.
	// TODO @joe: Note that this should only occur if we're working on a thread's
	// pages. Kernel space should never have to be unprotected, and we shouldn't
	// be accessing threadNodeList[MAX_NUM_THREADS+1]. On another note, this loop
	// shouldn't be necessary, since all of the current thread's pages should have RW permissions.
	if(req == THREADREQ) {
		int page;
	    for(page = threadNodeList[current_thread].firstPage; page != -1; page = pageTable[page].next){
	        memprotect(baseAddress + (page * PAGESIZE), PAGESIZE, PROT_READ)
		    memprotect(baseAddress + (page * PAGESIZE), PAGESIZE, PROT_WRITE)
	    }
	}
    
	// INITIALIZE KERNEL AND CREATE PAGE ABSTRACTION (FIRST MALLOC)
	if(*myBlock == '\0'){
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
		// Initialize the standard cells for threadNodeList.
		int i;
		for(i = 0; i < MAX_NUM_THREADS; i++) {
			// Make new ThreadMetadata struct and copy it to threadNodeList, for
			// each thread besides the kernel.
			ThreadMetadata newThreadData = {-1, 0};
			threadNodeList[i] = newThreaddata;
		}
		// Put PageTable before threadNodeList, allocating enough space for the metadata of remaining
		// pages in the memory. Meaning we get the address from subtracting the size of PageTable
		// from the address of threadNodeList.
		// Also, leave that last space at the end free for the swapping/free page, used
		// in signal handling.
		int threadPages = ((TOTALMEM - kernelSize)/(PAGESIZE)) - 1;
		PageTable = (PageMetadata *) (threadNodeList - (threadPages * sizeof(PageMetadata)));
		// Go through PageTable and create the structs at each space, initializing their space
		// to be FREE and having 0 space used.
		for(i = 0; i < threadPages - 1; i++) {
			// Make new PageMetadata struct and copy it to PageTable
			PageMetadata newData = {BLOCK_FREE, -1, -1, NULL};
			PageTable[i] = newData;

		}
		// Increase counter for memory allocated by kernel, now that we've allocated PageTable.
		PageTable[MAX_NUM_THREADS].memoryAllocated += (threadPages * PAGESIZE)
		
		// TODO @joe: before there was an erroneous set of mprotect statements here.
		// they were removed because 1) they first set permissions to R, then W, instead
		// of RW, 2) all memaligned-pages have RW permissions by default (according to
		// documentation anyways), and 3) one has to protect one page at a time, not
		// the entire memory. this would have caused a seg fault

	} //End of kernel setup and page creating

	
	// IF CALLED BY SCHEDULER
	// TODO @joe: This would have always been true, because before, it was if(LIBRARYREQ),
	// as LIBRARYREQ is 1. 
	if(req == LIBRARYREQ){
        // store first address of requested block
        char *ret = freeKernelPtr;
        // point freeKernelPtr to next free address in kernel block
        freeKernelPtr += bytes;    
        sigprocmask(SIG_UNBLOCK, SIGVTALRM, NULL);
        return (void *)ret;
	}
	//IF CALLED BY THREAD
	else if(req == THREADREQ){
		// FIND FREE SEGMENT WITHIN THREADS PAGE(S)
		// point iterator to thread's first allocated page (the actual stored data)
		int pageNum = pagethreadNodeList[thread].firstPage; 
		//this the first page that thread is under the illusion of iterating (the original space)
		int virtualPageNum = 0;
		// pointer to the head of the current stored page we're seeing
		char *ptr = baseAddress + (PAGESIZE * pageNum);
		// go check through the thread's list of currently-held pages, for an open segment
		// of sufficient size
		while(pageNum != -1){
			if(((SegMetadata *)ptr)->used == BLOCK_FREE){
				// check if the segment has sufficient room for the request
				if(((SegMetadata *)ptr)->size >= bytes)
					((SegMetadata *)ptr)->used = BLOCK_USED;
					// if the segment's size is greater than the user's allocation, plus
					// leaving room over for a SegMetadata struct
					if(((SegMetadata *)ptr)->size > bytes + sizeof(SegMetadata)){
						
						//# of bytes until end of current page
                        int bytesTillEnd = (baseAddress + (pageNum * PAGESIZE)) - ptr - sizeof(SegMetadata);
                        int bytesLeft = bytes - bytesTillEnd;
                        
                        int iterate = ceil(bytesLeft/PAGESIZE);
                        
						// iterate amount of pages necessary to insert SegMegadata
                        for(i = 0; i < iterate; i++){
                            pageNum = pageTable[pageNum].nextPage;
                        }
						
						// iterate remainder of bytes
                        char * nextPtr = baseAddress + (pageNum * PAGESIZE) + (bytesLeft%PAGESIZE);
                        
                        // add SegMetadata to set rest of block to free.
                        SegMetadata segment = {BLOCK_FREE, ((SegMetadata *)ptr)->size - bytes - sizeof(SegMetadata)};
                        *(SegMetadata *)nextPtr = segment;
                        
                        //return pointer to start of block
                        return (void *)(ptr + sizeof(SegMetadata));
						
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

        virtualPageNum++; //thread is under illusion that we are allocating next contiguous page
        int bytesTillEnd = 0;
		if(((SegMetadata *)ptr->used == BLOCK_FREE)){
			bytesTillEnd = (baseAddress + (pageNum * PAGESIZE)) - ptr - sizeof(SegMetadata);
		}
		//how many pages need to be allocated to handle request?
		int pagesToAllocate = ceil((bytes - bytesTillEnd)/PAGESIZE); //iterate this each page number of times.
        int pagesAllocated = 0;
    
        while(pagesAllocated != pagesToAllocate){
            if(pageTable[virtualPageNum ].used == BLOCK_USED){ //virtual page is used by another thread
                //find free pages to swap w/ page
                for(int i = 0; i < TOTALMEM / PAGESIZE; i++){
                    if(pageTable[i].used == BLOCK_FREE){
                        //found one!
                        
                        //1st: set values in threadNodeList
                        int threadPage = threadNodeList[pageTable[virtualPageNum].owner].firstPage;
                        if(threadPage == virtualPageNum){
                            threadNodeList[pageTable[virtualPageNum].owner].firstPage = i;
                        }
                        while(pageTable[threadPage].nextPage != -1){
                            
                            if(pageTable[threadPage].nextPage == virtualPageNum){
                                pageTable[threadPage].nextPage = i;
                            }
                            
                                threadPage = pageTable[threadPage].nextPage;
                        }
                    
                        //2nd: set values in PageTable
                        pageTable[i].owner = pageTable[virtualPageNum].owner
                        //pageTable[virtualPageNum].owner = current_thread;
                        pageTable[i].used = BLOCK_USED;

                        
                        //3rd: set permissions and reallocate page's contents
                        //set virtual page to PROT_READ/WRITE
                        mprotect(baseAddress + (PAGESIZE * virtualPageNum), PAGESIZE, PROT_READ);
                        mprotect(baseAddress + (PAGESIZE * virtualPageNum), PAGESIZE, PROT_WRITE);
                        //switches contents of virtual page to new page
                        memcpy(baseAddress + (PAGESIZE * virtualPageNum), baseAddress + (PAGESIZE * pageNum), PAGESIZE);
                        //set moved contents to PROT_NONE
                        mprotet(baseAddress + (PAGESIZE * pageNum), PAGESIZE, PROT_NONE);
                        
                    }

                }
            
            }
            // code above is the scenario if a block is used,
            // make it free. if the above code runs and the virtualPage
            // used member is still BLOCK_USED, there are no free pages
            if(pageTable[virtualPageNum].used == BLOCK_USED){
                return NULL; //no free pages were found
            }
            
            //VIRTUAL PAGE IS (NOW) FREE
            pageTable[virtualPageNum].owner = current_thread;
            if(virtualPageNum == 0){
                threadNodeList[current_thread].firstPage = virtualPageNum;
            }
            else{
                pageTable[pageNum].nextPage = virtualPageNum;
            }
            //plus one used page values
            pagesAllocated++;
            virtualPageNum++;
        }

        //UPDATE SEGMENT MEMBERS & SET REMAINING OF PAGE TO FREE
        ((SegMetadata *)ptr)->used = BLOCK_USED;
        ((SegMetadata *)ptr)->size = bytes;
    
        SegMetadata segment = { BLOCK_FREE, PAGESIZE - remainingBytes - sizeof(SegMetadata) };
        int remainingBytes = (bytes - bytesTillEnd) % PAGESIZE;
        char * nextPtr = baseAddress + (virtualPageNum * PAGESIZE) + remainingBytes;
        *(SegMetadata *)nextPtr = segment;
    
        sigprocmask(SIG_UNBLOCK, SIGVTALRM, NULL);
    
        //RETURN POINTER TO BLOCK
        return (void *)(ptr + sizeof(SegMetadata));
    }
    // invalid req parameter
    else {
    	exit(EXIT_FAILURE)
    }
}

/** Smart Free **/
void mydeallocate(void *ptr, char *file, int line, int req){
	sigprocmask(SIG_BLOCK, SIGVTALRM, NULL);
	
	int thread;
	int pageIndex
	char *index;
	int pagesize;
	//Set ptr to point to its SegMetadata
	ptr = ptr - sizeof(SegMetadata);
	if(req == THREADREQ) {
		thread = current_thread;
		printf("Beginning mydeallocate for thread: %d\n", current_thread);
	
		// Find the page of the memory
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
				char * endIndex = baseAddress + (endPageIndex * PAGESIZE)
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
		if ((ptr + ((SegMetadata *)ptr)->size + sizeof(SegMetadata)) < (index + kernelSize)) {
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
					(SegMetadata *)ptr->size = (sizeof(SegMetadata) + ptr) - (index + PAGESIZE);
				}
			}
			
			// index is now the memory address of the last remainder of the segment
			index += numPages * PAGESIZE;
			// IF pages will be freed, create new segment at start of last page
			if (numPages > 0 && remainder > sizeof(SegMetadata)) {
				SegMetadata newSeg = { BLOCK_FREE, remainder - sizeof(SegMetadata), prev };
				(SegMetadata *)index = newSeg;
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
	
	sigprocmask(SIG_UNBLOCK, SIGVTALRM, NULL);
	
	return;

}
