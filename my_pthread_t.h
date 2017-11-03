// File:	my_pthread_t.h
// Author:	Yujie REN
// Date:	09/23/2017

// name: Bruno J. Lucarelli
// username of iLab: bjl145
// iLab Server: man.cs.rutgers.edu
#ifndef MY_PTHREAD_T_H
#define MY_PTHREAD_T_H

#define _GNU_SOURCE

/* To use real pthread Library in Benchmark, you have to comment the USE_MY_PTHREAD macro */
#define USE_MY_PTHREAD 1

/* include lib header files that you need here: */
#include <ucontext.h>
#include <unistd.h>
#include <signal.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

typedef uint my_pthread_t;

/* Our own enums describing thread status. */
enum threadStatus {
	THREAD_RUNNING = 0,
	THREAD_READY = 1,
	THREAD_BLOCKED = 2,
	THREAD_WAITING = 3,
	THREAD_DONE = 4,
	THREAD_INTERRUPTED = 5,
	THREAD_YIELDED = 6
};

/* Our own enums describing mutex lock status. */
enum lockStatus {
	LOCKED = 0,
	UNLOCKED = 1
};


/* Data structure for storing thread information.

This struct is used for each thread we have, to store information
about it.

*/
typedef struct threadControlBlock {
	/* The thread's current status. It tells us whether the thread
	is currently running, whether it's ready to run but isn't, 
	or whether it's blocked. Can add additional enums
	as we go forward. */
	int status;

	/* The thread's ID. Due to structure for storing tcb's
	inherently being the thread's ID, this might be redundant.*/
	my_pthread_t tid;

	/* Pointer to the stack this thread runs on. This is not
	specific to the thread, as other threads may run on
	the same stack. May be redundant with context here.*/
	char* stack;

	/* The context this thread runs on. This is specific to
	the thread, whereas multiple threads may share a stack.*/
	ucontext_t context;

	/* The number of time slices allocated to the thread.
	This is zero by default, and is allocated during the
	maintenance cycle.*/
	uint timeSlices;

	/* The thread's current priority. 0 by default, but priority
	level is decreased (the priority going from 0 to 1, 1 to 2,
	and so on) as the thread is interrupted/preempted more often*/
	uint priority;

	/* The ID of the thread waiting on this thread, if any. -1 by
	default, or if no threads are waiting on this thread. */
	my_pthread_t waitingThread;

	/* The value pointer of this thread.*/
	void **valuePtr;

	/* The number of runQueue cycles the given thread has waited without
	running. 0 by default, and increases as a thread waits for longer.
	Indicates to the Manager thread that a thread needs to be promoted.

	cyclesWaited should be incremented if, and ONLY if, a thread at priority
	level below 0 remains in the MLPQ with status THREAD_READY after
	the runQueue has been populated.

	cyclesWaited should be decreased to 0 if, and ONLY if, a thread is
	added from the MLPQ to the runQueue, OR if the thread is promoted
	in priority.*/
	uint cyclesWaited;

	/* The pointer to this tcb's function. */
	void *(*function)(void*);

} tcb; 


/* Data structure for linked lists of my_pthread_t values.

This struct will be used in the tcb to store linked lists of my_pthread_t's, or
Thread ID's/TID's for short. The list of usages of pnodes is as follows:

1. Each "bucket" of the MLPQ
2. The run queue in the manager thread
3. The "recyclable thread ID" list used in the manager thread

 */
typedef struct my_pthread_node {
        /* The ID of the thread being referenced by this pnode. */
        my_pthread_t tid;

        /* The next pnode in the list. */
        struct my_pthread_node *next;


} pnode;


/* Mutex struct definition */
typedef struct my_pthread_mutex_t {
	/* UNLOCKED or LOCKED */
	int status;

	/* Threads waiting for this lock */
	pnode *waitQueue;
	
	/* Current thread that owns this lock */
	my_pthread_t ownerID;
	
	/* Mutex attribute */
	const pthread_mutexattr_t *attr;
	
} my_pthread_mutex_t;


/* Function Declarations: */

/* create a new thread */
int my_pthread_create(my_pthread_t * thread, pthread_attr_t * attr, void *(*function)(void*), void * arg);

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield();

/* terminate a thread */
void my_pthread_exit(void *value_ptr);

/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr);

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *mutexattr);

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex);

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex);

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex);


/* Our own functions below */

/* Initializes the manager thread, with the user's calling function
being saved as a child thread which is itself managed by the 
manager thread. Returns 0 on failure, 1 on success.*/
int init_manager_thread();

/* Carries out the manager thread responsibilities.
Makes use of runQueueHelper() and maintenanceHelper() in order
to make debugging more modular. Returns 0 on failure, 1
on success. */
int my_pthread_manager();

/* Helper function which performs most of the work for
the manager thread's maintenance cycle. Returns 0 on failure,
1 on success.*/
int maintenanceHelper();

/* This function is the helper function which performs most of
the work for the manager thread's run queue. Returns 0 on failure,
1 on success. */
int runQueueHelper();

/* This is the signal handler for our timer. */
void VTALRMhandler(int signum);

/* Returns a pointer to a new tcb instance. */
tcb *createTcb(my_pthread_t id, ucontext_t context, void *(*function)(void*));

/* Returns a pointer to a new pnode instance. */
pnode *createPnode(my_pthread_t tid);

/* Inserts a given pnode into a given level of the MLPQ, such
that it is the last node in that level's list (or first, if no others12
are present). Returns 0 on failure, 1 on success. */
int insertPnodeMLPQ(pnode *input, uint level);

#ifdef USE_MY_PTHREAD
#define pthread_t my_pthread_t
#define pthread_mutex_t my_pthread_mutex_t
#define pthread_create my_pthread_create
#define pthread_exit my_pthread_exit
#define pthread_join my_pthread_join
#define pthread_mutex_init my_pthread_mutex_init
#define pthread_mutex_lock my_pthread_mutex_lock
#define pthread_mutex_unlock my_pthread_mutex_unlock
#define pthread_mutex_destroy my_pthread_mutex_destroy
#endif

#endif
