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


/* Override malloc() and free() calls with our functions */
#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ)


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

A struct that holds metadata for a block of memory.
These structs will be stored before each individual 
block of memory in myBlock. 

*/
typedef struct Node {
	/* The status of the current node. 0 for free/'F', 1 for allocated/'T' (used). 
	We will, in the library, refer to these as BLOCK_FREE and BLOCK_USED*/
	int used;
	/* The size of the current node in bytes.
	TODO @all: discuss how to use this for the paging project. */
	unsigned short size; 
} Metadata;  

/* Function Declarations: */

void *myallocate(int size, char *file, int line, int req);

void mydeallocate(void *freeptr, char *file, int line, int req);

/* Our own functions below */

/* Description */
int init_memory_manager();

#endif