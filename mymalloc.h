/*
 * CS214 Systems Programming
 * Joseph Gormley 
 * mymalloc.h
 */


static char myblock[5000];

#define malloc(x) mymalloc(x, __FILE__,__LINE__)
#define free(x) myfree( x, __FILE__, __LINE__ )

void *mymalloc(int size, char *file, int line);

void myfree(void *freeptr, char *file, int line);