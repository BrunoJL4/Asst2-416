// name: Bruno J. Lucarelli
//       Joseph Gormley
//       Alex Marek
// username of iLab: bjl145
// iLab Server: man.cs.rutgers.edu
#ifndef MY_MALLOC_H
#define MY_MALLOC_H

/* include lib header files that you need here: */

/* Including my_pthread_t.h for basic libraries we already used,
plus the current_thread variable externalized there*/
#include "my_pthread_t.h"
#include <signal.h>

/* Global variables. */

/* Will be accessed by scheduler for moving files around in
SIGSEGV handler. */
extern char *myBlock;
/* Will be accessed by scheduler for figuring out page ownership
by thread, in SIGSEGV handler. */
extern ThreadMetadata *threadNodeList;
/* Will be accessed by scheduler for page bookeeping, in
SIGSEGV handler. */
extern PageMetadata *PageTable;
/* Also accessed by the SIGSEGV handler. */
extern char *baseAddress;


/* Constants used in mymalloc.c will be declared here, so that
they can be accessed by other libraries. */
#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ)
#define TOTALMEM 8388608 //2^20 x 2^3 = 8 megabytes. 
#define THREADREQ 0 //User called
#define LIBRARYREQ 1 //Library called
#define PAGESIZE sysconf(_SC_PAGE_SIZE) //System page size

/* Enum declarations: */
enum blockStatus {
	/* Corresponds to the 'F' status for a block in original mymalloc
	implementation. */
	BLOCK_FREE = 0,
	/* Corresponds to the 'T' status for a block in original mymalloc
	implementation. */
	BLOCK_USED = 1
};


/* Data structure declarations: */

/*Metadata Node

A struct that holds metadata for a block of page memory.
These structs will be stored before each individual 
block of memory in myBlock. 

*/

typedef struct PageNode {
	/* The status of the current node. 0 for free/'F', 1 for allocated/'T' (used). 
	We will, in the library, refer to these as BLOCK_FREE and BLOCK_USED*/
	int used;
	
	/* Number of the next page's data for the owning thread. -1 by default.*/
	int nextPage;

	/* Thread ID of the the thread owning this page. -1 by default.  */
	my_pthread_t owner;

	/* The address of the segment that floods into this page. NULL by default, or
	if there are no segments that have overflowed their data into/through this page. 
	NOTE: This should always refer to the location of the parent segment, in its
	ORIGINAL SPOT IN MEMORY. Not its currently-stored location.*/
	char *parentSegment;

} PageMetadata;  

/*
	Like the struct above except efficiently specific for segment metadata
*/
typedef struct SegNode {
	/* Status of the current node, identical convention to "used" for PageNode. */
	int used;
	/* Size of the data allocation this segment has. */
	unsigned int size;
	/* Address of previous SegMetadata */
	SegMetadata *prev;
} SegMetadata;

/* Thread Node
	
	A struct that holds data for each thread pertaining to memory allocation.

*/
typedef struct ThreadNode {
	/* Page # of the thread's first allocated page. If Thread A allocates pages 0
	and 1, and Thread B's context swaps in and allocates pages 0 and 1, then Thread A's
	firstPage points to wherever its data for page 0 was. If it was swapped to
	Page 2, then threadA.firstPage will be 2. Similarly, PageTable[threadA.firstPage].next
	will be 3 if Thread A's data for page 1 was swapped to page 3.

	This value should be a signed int, since its value can be -2 (for the kernel)
	or -1 (for a thread with no memory allocated yet)*/
	int firstPage;

	/* Memory allocated so far for this thread. Used in operations involving both
	page shuffling and determining victims for the Swap File. */
	int memoryAllocated;
} ThreadMetadata;



/* Function Declarations: */

void *myallocate(int size, char *file, int line, int req);

void mydeallocate(void *freeptr, char *file, int line, int req);

/* Our own functions below */

/* Description */
int init_memory_manager();

#endif