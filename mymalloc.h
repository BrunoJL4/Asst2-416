// name: Bruno J. Lucarelli
//       Joseph Gormley
//       Alex Marek
// username of iLab: bjl145
// iLab Server: man.cs.rutgers.edu

#define malloc(x) myallocate(x, __FILE__, __LINE__, THREADREQ)
#define free(x) mydeallocate(x, __FILE__, __LINE__, THREADREQ)

/* Function Declarations: */

void *mymalloc(int size, char *file, int line);

void myfree(void *freeptr, char *file, int line);

/* Our own functions below */

/* Description */
int init_memory_manager();

