// File:	my_pthread.c
// Author:	Yujie REN
// Date:	09/23/2017

// name: Bruno J. Lucarelli
// username of iLab: bjl145
// iLab Server: man.cs.rutgers.edu

#include "my_pthread_t.h"

/* Define global variables here. */

/* The Multi-Level Priority Queue (MLPQ).

The MLPQ is an array of pnodes. Since each "pnode" will
really just be a pointer to a pnode in this array, MLPQ is
a variable of type pointer-to-a-pnode-pointer, or pnode **MLPQ.

The initialization function for building the tcb will initialize
the MLPQ to an array of an allocated length equal to the number
of priority levels. 5 levels = 5 cells in the array.
*/
pnode **MLPQ;



/* The array that stores pointers to all Thread Control Blocks.

tcbList has one cell allocated per possible tcb. The index of
a cell in tcbList corresponds to the thread's ID; so Thread #200
should be located in tcbList[200]. tcbList will be NULL when
initialized by the manager thread, but otherwise should be treated
as an array of pointers.

*/
tcb ** tcbList;


/* This is the Run Queue.

The Run queue, or runQueue, is a linked list of pnodes that
will be run from start to finish. runQueue will be NULL by
default or if no nodes remain to be run. During the maintenance
cycle of the manager thread, runQueue will be populated with
pnodes in the order in which they are run. Threads remaining
in runQueue which haven't run yet when the maintenance cycle
begins, will have their priority increased in that cycle.
It should also be noted that runQueue is populated until
we run out of time slices to allocate to threads...
right now we're looking at 20 time slices, or quanta, of
25ms each.

The initialization function for building the tcb will initialize
runQueue to NULL. */
pnode *runQueue;


/* This is the Recyclable Queue.

The Recyclable Queue, or recyclableQueue, is a linked list of pnodes
that contains all "recyclable" thread ID's. A thread ID is recyclable
when the thread holding that ID has been destroyed and not yet
reused. recyclableQueue will be used once the max number of
threads has been exceeded.
*/
pnode *recyclableQueue;

/* Number of threads created so far.
This will be initialized to 0 when the manager thread is initialized. */
uint threadsSoFar;

/* contexts */
ucontext_t Manager;

/* info on current thread */

/*ID of the currently-running thread. MAX_NUM_THREADS+1 if manager,
otherwise then some child thread. */
my_pthread_t current_thread;

/* The status of the currently-running thread (under the manager).
Will either be THREAD_RUNNING, THREAD_INTERRUPTED, or THREAD_WAITING*/
int current_status;

/* Boolean 1 if manager thread is active, otherwise 0 as globals
are initialized to by default*/
uint manager_active;

/* Tells us whether the current thread exited. */
uint current_exited;

/* Status of currently-running thread. */
enum threadStatus currentStatus;

/* Signal action struct used by runQueueHelper() for alarms. 
Declared up here to prevent allocations from occurring
each time the runQueueHelper() runs.*/
struct sigaction sa;

/* Signal action struct used by runQueueHelper() for memprotect
violations. This will be used for the SIGSEGV handler that handles
read violations. */
struct sigaction sig_mem;

/* itimerval struct used by runQueueHelper() for alarms.
Declared up here to prevent allocations from occurring
each time the runQueueHelper() runs.*/
struct itimerval timer;

/* End global variable declarations. */

/* my_pthread and mutex function implementations */

/* create a new thread */
int my_pthread_create(my_pthread_t *thread, pthread_attr_t * attr, void *(*function)(void*), void * arg) {
	// flag that is 1 if we're initializing the manager thread,
	// 0 if not. we'll use this at the end of the function to decide
	// whether or not to swap contexts back to manager (ONLY swap
	// contexts if we've just initialized the manager!)
	int initializingManager = 0;
	//check that manager thread exists	
	//init if it does not
	if (manager_active != 1) {
		initializingManager = 1;
		init_manager_thread();
	}
	//set information for new child thread's context
	my_pthread_t tid = threadsSoFar;
	ucontext_t context;
	//initialize context, fill it in with current
	//context's information
	getcontext(&context);
	//check if we've exceeded max number of threads
	if (threadsSoFar >= MAX_NUM_THREADS) {
		//if so, check recyclableQueue, return -1 if there are no available thread ID's
		if (recyclableQueue == NULL) {
			return -1; 
		}
		//remove first available ID from queue
		pnode *ptr = recyclableQueue;
		recyclableQueue = recyclableQueue->next;
		//take the ID
		tid = ptr->tid;
		//free the pnode for the recycled ID
		mydeallocate(ptr, __FILE__,__LINE__, LIBRARYREQ);
		//make a new TCB from the gathered information
		tcb *newTcb = createTcb(tid, context, function);
		//change the tcb instance in tcbList[id] to this tcb
		tcbList[(uint) tid] = newTcb;
		// insert a pnode containing the ID at Level 0 of MLPQ
		pnode *node = createPnode(tid);
		insertPnodeMLPQ(node, 0);
		// increment number of threads so far
		threadsSoFar ++;
		newTcb->context.uc_link = &Manager;
		if (arg == NULL) {
			makecontext(&(newTcb->context), (void*)function, 0);
		} 
		else {
			makecontext(&(newTcb->context), (void*)function, 1, arg);
		}
		*thread = tid;
		return 0;
	}
	// if still using new ID's, just use threadsSoFar as the index and increment it
	tcb *newTcb = createTcb(tid, context, function);
	// add the new tcb to the tcbList at the cell corresponding to its ID
	tcbList[tid] = newTcb;
	// create new pnode for new thread
	pnode *node = createPnode(tid);
	// insert new node to Level 0 of MLPQ
	insertPnodeMLPQ(node, 0);
	// we've added another thread, so increase this
	threadsSoFar ++;
	newTcb->context.uc_link = &Manager;
	if (arg == NULL) {
		makecontext(&(newTcb->context), (void*)function, 0);
	} 
	else {
		makecontext(&(newTcb->context), (void*)function, 1, arg);
	}
	// if we've just initialized the manager thread, swap to it because
	// we're in the Main context and need to give the Manager control
	if(initializingManager == 1) {
		my_pthread_t main_thread = current_thread;
		current_thread = MAX_NUM_THREADS + 1;	
		// update Main's context in the tcbList so that it resumes from here
		// swap back to Manager
		swapcontext(&(tcbList[main_thread]->context), &Manager);
	}
	//returns the new thread id on success
	*thread = tid;
	return 0; 
}

/* give CPU pocession to other user level threads voluntarily */
int my_pthread_yield() {
	
	unsigned int numberOfThreads = 0;
	
	//check that there's other threads to yield() to (regardless of priority)
	int i;
	for(i = 0; i < MAX_NUM_THREADS; i++){
		if(tcbList[i] != NULL){
			numberOfThreads++;
		}
	}
	
	//if no other threads to yield() to
	if(numberOfThreads <= 1) {
		return 1;
	}
	
	// set thread to yield, set current_thread to manager, swap contexts.
	// manager will yield job in stage 1 of maintenance
	tcbList[(uint) current_thread]->status = THREAD_YIELDED;
	my_pthread_t prev_thread = current_thread;
	current_thread = MAX_NUM_THREADS + 1;
	swapcontext(&(tcbList[prev_thread]->context), &Manager);
	return 1;

}

/* terminate a thread and fill in the value_ptr of the
thread waiting on it, if any */
void my_pthread_exit(void *value_ptr) {
	
	current_exited = 1; //explicit exit
	
    // thread that the calling thread is joined to
    my_pthread_t waitingThread = tcbList[current_thread]->waitingThread;
	//printf("(TARGET THREAD | ID = %d): my waiting value is: %d\n", tcbList[current_thread]->tid, tcbList[current_thread]->waitingThread);
	
	
    // if thread was being waited on by another thread, set value_ptr 
    if(waitingThread != MAX_NUM_THREADS + 2) {
    	if(tcbList[waitingThread]->valuePtr != NULL) {
    		*(tcbList[waitingThread]->valuePtr) = value_ptr;
    	}
    }
	
    //swap back to the Manager context 
    current_thread = MAX_NUM_THREADS + 1;
    setcontext(&Manager);
}


/* wait for thread termination */
int my_pthread_join(my_pthread_t thread, void **value_ptr) {

	my_pthread_t targetThread = thread;
	
    // what if target thread doesn't exist?
    if((tcbList[targetThread]) == NULL){
        return -1;
    }
  
    // set target thread's waitingThread to this thread.
	// target thread will now be able to check if it's being waited on
    tcbList[targetThread]->waitingThread = current_thread;
    tcbList[current_thread]->status = THREAD_YIELDED;
	
	// this for the runQueueHelper()'s reference
	current_status = THREAD_WAITING;
	
	//set threads value exit() will later set
	tcbList[current_thread]->valuePtr = value_ptr;
	
	
	
	//swap back to manager	
	my_pthread_t tmp = current_thread;
	current_thread = MAX_NUM_THREADS + 1;
	swapcontext(&(tcbList[tmp]->context), &Manager);
	
	
    return 0; // success
}

/* initial the mutex lock */
int my_pthread_mutex_init(my_pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
	//initialize mutex
	//user already allocated space by declaring the mutex,
	//we just change its members.
	mutex->status = UNLOCKED;
	mutex->waitQueue = NULL;
	mutex->ownerID = MAX_NUM_THREADS + 1;
	mutex->attr = attr;
	return 0;
}

/* aquire the mutex lock */
int my_pthread_mutex_lock(my_pthread_mutex_t *mutex) {
//	printf("entered my_pthread_mutex_lock()!\n");
	//If mutex is locked, enter waitQueue and yield
	//NOTE: yield should set this thread status to BLOCKED
	if (mutex->status == LOCKED) {
		//Create pnode of current thread
		pnode *new = myallocate(sizeof(pnode), __FILE__, __LINE__, 0);
		new->tid = current_thread;
		new->next = NULL;
		//start a waitQueue if it is empty
		if (mutex->waitQueue == NULL) {
			mutex->waitQueue = new;
		//add to the end of the waitQueue
		} 
		else {
			pnode *ptr = mutex->waitQueue;
			while (ptr->next != NULL) {
				ptr = ptr->next;
			}
			ptr->next = new;
		}
		//set thread status to BLOCKED and change context
//		printf("pausing thread #%d: my_pthread_mutex_lock()\n", current_thread);
		my_pthread_t blocked_thread = current_thread;
		tcbList[(uint) blocked_thread]->status = THREAD_BLOCKED;
		current_status = THREAD_BLOCKED;
		//let the manager continue in the run queue
		current_thread = MAX_NUM_THREADS + 1;
		swapcontext(&(tcbList[blocked_thread]->context), &Manager);
	} 
	//continue running after the end of yielding OR did not have to yield
	//Set mutex value to locked
	mutex->status = LOCKED;
	//Set mutex owner to current thread
	mutex->ownerID = current_thread;
//	printf("finished my_pthread_mutex_lock()!\n");
	return 0;
}

/* release the mutex lock */
int my_pthread_mutex_unlock(my_pthread_mutex_t *mutex) {
//	printf("entered my_pthread_mutex_unlock()!\n");
	//If mutex is NOT initialized
	//user did something bad
	if (mutex == NULL) {
		return -1;
	//Elif mutex does not belong to us
	//we can't unlock it
	} 
	else if (mutex->ownerID != current_thread) {
		return -1;
	}
	//otherwise unlock mutex
	mutex->status = UNLOCKED;
	//Check waiting queue, destroy mutex if there is no more use
	if (mutex->waitQueue == NULL) {
//		printf("finished my_pthread_mutex_unlock()!\n");
		return 0;
	}
	//alert the next available thread & remove it from queue/add back to run queue
	pnode *ptr = mutex->waitQueue;
	mutex->waitQueue = mutex->waitQueue->next;
	//make this thread ready so it can now acquire this lock
	tcbList[(uint) ptr->tid]->status = THREAD_READY;
	mydeallocate(ptr, __FILE__, __LINE__, LIBRARYREQ);
//	printf("finished my_pthread_mutex_unlock()!\n");
	return 0;
}

/* destroy the mutex */
int my_pthread_mutex_destroy(my_pthread_mutex_t *mutex) {
	//If mutex is NOT initialized
	if (mutex == NULL) {
		return -1;
	//Elif mutex is locked
	} 
	else if (mutex->status == LOCKED) {
		return -1;
	}	
	//can't free memory used...
		
	return 0;
}


/* Essential support functions go here (e.g: manager thread) */

/* Carries out the manager thread responsibilities.
Returns 0 on failure, 1 on success. */
int my_pthread_manager() {
	// check if manager is still considered "active"
	while(manager_active == 1) {
		// perform maintenance cycle
		if(maintenanceHelper() != 0) {
			return -1;
		}
		// perform run queue functions
		if(runQueueHelper() != 0) {
			return -1;
		}
	}
	// We only reach this point when maintenanceHelper()
	// has set manager_active to 0. Leave the function.
	return 0;
}


/* Helper function which performs most of the work for
the manager thread's maintenance cycle. Returns 0 on failure,
1 on success.*/
int maintenanceHelper() {
	// first part: clearing the run queue, and performing
	// housekeeping depending on the thread's status
	pnode *currPnode = runQueue;
	while(currPnode != NULL) {
		my_pthread_t currId = currPnode->tid;
		tcb *currTcb = tcbList[(uint)currId];
		// if a runQueue thread's status is THREAD_DONE:
		if(currTcb->status == THREAD_DONE) {
			// deallocate the current thread's stack
			mydeallocate(currTcb->stack, __FILE__, __LINE__, LIBRARYREQ);
			// deallocate the thread's tcb through tcbList
			mydeallocate(currTcb, __FILE__, __LINE__, LIBRARYREQ);
			// set tcbList[tid] to NULL
			tcbList[(uint)currId] = NULL;
			// then deallocate its pnode in the run queue while
			// moving currPnode to the next node.
			pnode *temp = currPnode;
			currPnode = currPnode->next;
			mydeallocate(temp, __FILE__, __LINE__, LIBRARYREQ);
		}
		// if a runQueue thread's status is THREAD_INTERRUPTED:
		else if(currTcb->status == THREAD_INTERRUPTED) {
			// we insert the thread back into the MLPQ but at one lower
			// priority level, also changing its priority member.
			// then change its status to READY.
			if(currTcb->priority < NUM_PRIORITY_LEVELS - 1){
				currTcb->priority ++;
			}
			pnode *temp = currPnode;
			currPnode = currPnode->next;
			insertPnodeMLPQ(temp, currTcb->priority);
			currTcb->status = THREAD_READY;
		}
		// if a runQueue thread is waiting or yielding
		else if(currTcb->status == THREAD_WAITING || currTcb->status == THREAD_YIELDED || currTcb->status == THREAD_BLOCKED) {
			// put the thread into the MLPQ at the same priority level,
			// so that it can resume in subsequent runs when it's
			// set to READY as the thread it's waiting on finishes
			// execution.
			pnode *temp = currPnode;
			currPnode = currPnode->next;
			insertPnodeMLPQ(temp, currTcb->priority);
		}
		// if a runQueue thread's status isn't any of the four above:
		else{
			return -1;
		}
	}
	runQueue = NULL;

	// second part: populating the run queue and allocating time slices.
	// go through MLPQ, starting at highest priority level and going
	// down until we've given out time slices, putting valid threads
	// into the run queue and setting their time slices accordingly.
	// a "valid" thread is one that is READY; any other thread status
	// is invalid and not ready to go in the run queue.
	int timeSlicesLeft = 20;
	int i;
	for(i = 0; i < NUM_PRIORITY_LEVELS; i++) {
		// formula for priority levels v. time slices: 2^(level)
		int numSlices = level_slices(i);
		// if we don't have enough timeSlices left to distribute to any node in
		// the current level, break (prevents searching further levels)
		if(numSlices > timeSlicesLeft) {
			break;
		}
		// go through this level's queue, if at all applicable.
		pnode *currPnode = MLPQ[i];
		pnode *prev = currPnode;
		while(currPnode != NULL) {
			// don't search the current level further if not enough
			// time slices are left.
			if(numSlices > timeSlicesLeft) {
				break;
			}
			my_pthread_t currId = currPnode->tid;
			tcb *currTcb = tcbList[(uint) currId];
			// if the current pnode's thread is ready to run (either marked
			// as READY, or was YIELDED before...
			if(currTcb->status == THREAD_READY || currTcb->status == THREAD_YIELDED) {
				// make a temp ptr to the current pnode.
				pnode *tempCurr = currPnode;
				// delink it for one of two cases:
				// first case: pnode is first node in queue
				if(currPnode == MLPQ[i]) {
					// set MLPQ[i]'s pointer to the next node
					MLPQ[i] = MLPQ[i]->next;
					// navigate to next part so that prev and currPnode
					// both point to the beginning of updated MLPQ[i]
					prev = MLPQ[i];
					currPnode = MLPQ[i];
				}
				// second case: currPnode isn't first node in level (e.g. is
				// in the middle or is the last node)
				else{
					// delink current node from MLPQ
					prev->next = currPnode->next;
					currPnode = currPnode->next;
				}
				// add the tempCurr ptr to the end of the runQueue.
				pnode *temp = runQueue;
				
				//if runQueue is empty, just set runQueue to current
				if(runQueue == NULL){
					runQueue = tempCurr;
				}
				//otherwise, add to the end of queue
				else{ 
					while(temp->next != NULL) {
						temp = temp->next;
					}
					temp->next = tempCurr;
				}
				// point its next member to NULL.
				tempCurr->next = NULL;
				// set the thread's cyclesWaited to 0, as it's being
				// given a chance to run.
				currTcb->cyclesWaited = 0;;
				// give the thread the appropriate number of time slices
				currTcb->timeSlices = numSlices;
				// subtract numSlices from timeSlicesLeft
				timeSlicesLeft = timeSlicesLeft - numSlices;
				// change its corresponding thread's status to THREAD_READY.
				currTcb->status = THREAD_READY;
			}
			// if the current thread isn't ready, navigate as normal as we
			// haven't delinked anything
			else{
				prev = currPnode;
				currPnode = currPnode->next;
			}
		}
	}

	// third part: searching all non-0 levels of the MLPQ to see if any threads
	// not at P0 have THREAD_READY status and an age greater than 5.
	// if so, bump up their priority level and set their age to 0.
	// this means we add them to the next highest level, increment their
	// priority by 1, and delink them from this level.
	for(i = 1; i < NUM_PRIORITY_LEVELS; i++) {
		pnode *curr = MLPQ[i];
		pnode *prev = MLPQ[i];
		// go through current level's queue
		while(curr != NULL) {
			my_pthread_t currId = curr->tid;
			tcb *currTcb = tcbList[(uint) currId];
			// if the thread has THREAD_READY status:
			if(currTcb->status == THREAD_READY) {
				// if the thread's age is 5 cycles or greater,
				// "promote" it
				if(currTcb->cyclesWaited >=5) {
					// set its age to 0
					currTcb->cyclesWaited = 0;
					// decrement its priority member
					currTcb->priority -= 1;
					// set a temp ptr to the current thread
					pnode *temp = curr;
					// delink it from the current queue
					prev->next = curr->next;
					// insert it into the next highest level
					insertPnodeMLPQ(temp, currTcb->priority);
				}
				// otherwise, increase its age by 1
				else{ 
					currTcb->cyclesWaited ++;
				}
			}
			prev = curr;
			curr = curr->next;
		}
	}

	// final part: check if runQueue and MLPQ are both empty. if they
	// are, set manager_active to 0.
	if(runQueue == NULL) {
		int mlpq_empty = 1;
		// check and see if all queues in MLPQ are empty.
		for(i = 0; i < sizeof(MLPQ); i++) {
			if(MLPQ[i] != NULL) {
				break;
			}
			if(i == (sizeof(MLPQ) - 1)) {
				mlpq_empty = 0;
			}
		}
		if(mlpq_empty == 1) {
			manager_active = 0;
		}
	}
	// when runQueue has either been populated with valid, ready threads,
	// or we've indicated that the manager thread's job has finished,
	// return 0 to indicate success.
	return 0;
}



/* this function is the helper function which performs most of
the work for the manager thread's run queue. Returns 0 on failure,
1 on success. */
int runQueueHelper() {
	// first, check and see if the manager thread is still active after
	// the last round of maintenance
	if(manager_active == 0) {
		return 0;
	}
	if(runQueue == NULL) {
		return -1;
	}

	// call signal handler for SIGVTALRM, which should activate
	// each time we receive a SIGVTALRM
	if(sigaction(SIGVTALRM, &sa, NULL) == -1) {
		printf("Error setting up SIGVTALRMhandler in runQueueHelper!\n");
		return -1;
	}
	// call signal handler for SIGSEGV, which should activate
	// upon any segfault occurring (even ones that aren't a result
	// of memory issues.)
	if(sigaction(SIGSEGV, &sig_mem, NULL) == -1) {
		printf("Error setting up SEGVhandler in runQueueHelper!\n");
		return -1;
	}

	// it begins with a populated runQueue. it needs to iterate through
	// each thread and perform the necessary functions depending on
	// the thread's status. the only valid status for a thread it
	// encounters is THREAD_READY. it will, however, change thread
	// statuses to THREAD_DONE, THREAD_INTERRUPTED, or THREAD_WAITING
	// at some point.
	pnode *currPnode = runQueue;
	while(currPnode != NULL) {
		my_pthread_t currId = currPnode->tid;
		tcb *currTcb = tcbList[(uint) currId];
		// grab number of time slices allowed for the thread
		int slicesLeft = currTcb->timeSlices;
		// change status of current thread to running
		currTcb->status = THREAD_RUNNING;
		current_status = THREAD_RUNNING;
		// setitimer for 25ms * the number of time slices allotted
		// to this thread. set timer type to VIRTUAL_TIMER.
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = (25000) * slicesLeft;
		// TODO @all: if the currPage is out of scope of the user space, we
		// shouldn't be modifying/changing it (e.g. in a swap file). make
		// sure for phase C that this logic works.
		// un-protect all of the to-be-run thread's memory pages
		int currPage = threadNodeList[currId].firstPage;
		while(currPage != -1) {
			if(mprotect((baseAddress + (currPage * PAGESIZE)), PAGESIZE, PROT_READ|PROT_WRITE) == -1) {
				exit(EXIT_FAILURE);
			}
			currPage = PageTable[currPage].nextPage;
		}
		setitimer(ITIMER_VIRTUAL, &timer, NULL);
		// swap contexts with this child thread.
		current_thread = currId;
		// set current_exited to 0;
		current_exited = 0;
		// update child thread's uc_link to Manager
		//tcbList[currId]->context.uc_link = &Manager;
		swapcontext(&Manager, &(currTcb->context));
		// immediately turn itimer off for this thread
		timer.it_value.tv_sec = 0;
		timer.it_value.tv_usec = 0;
		// protect all of the previously-run thread's memory pages (whichever
		// those may be after the thread ran)
		currPage = threadNodeList[currId].firstPage;
		while(currPage != -1) {
			if(mprotect((baseAddress + (currPage * PAGESIZE)), PAGESIZE, PROT_NONE) == -1) {
				exit(EXIT_FAILURE);
			}
			currPage = PageTable[currPage].nextPage;
		}
		// if this context resumed and current_status is still THREAD_RUNNING,
		// then thread ran to completion before being interrupted.
		if(current_status == THREAD_RUNNING) {
			currTcb->status = THREAD_DONE;
			if(current_exited == 0) {
				//current_thread = tcbList[currId]->tid;
				tcbList[current_thread]->waitingThread = MAX_NUM_THREADS + 2;
				my_pthread_exit(NULL);
			}
		}
		// if this context resumed and current_status is THREAD_INTERRUPTED,
		// then the signal handler interrupted the child thread, which
		// didn't get to run to completion.
		else if(current_status == THREAD_INTERRUPTED){
			// Do nothing here, since thread's status was already set
		}
		// if this context resumed and current status is THREAD_WAITING,
		else if(current_status == THREAD_WAITING) {
			// Do nothing here, since thread's status was already set
		}
		else if(current_status == THREAD_BLOCKED) {
			// Do nothing here, since thread's status was already set.
		}
		// this branch shouldn't occur
		else {
			return -1;
		}
		// go to the next node in the runQueue
		currPnode = currPnode->next;
	}
	return 0;
}


/* Signal handler for SIGVTALRM.

This is the signal handler used in the case that a thread runs over
its time and needs to be interrupted.

*/
void VTALRMhandler(int signum) {
	// DO NOT PUT A PRINT MESSAGE IN A SIGNAL HANDLER!!!

	// We've interrupted a thread, so change the current_status
	// to THREAD_INTERRUPTED
	current_status = THREAD_INTERRUPTED;
	tcbList[current_thread]->status = THREAD_INTERRUPTED;
	my_pthread_t interrupted_thread = current_thread;
	// Set the current context back to Manager
	current_thread = MAX_NUM_THREADS + 1;
	swapcontext(&(tcbList[interrupted_thread]->context), &Manager);
}

/* Signal handler for SIGSEGV.

This is the signal handler used in the case that a thread reads memory
that isn't its own. 

*/
void SEGVhandler(int sig, siginfo_t *si, void *unused) {
	// TODO @bruno: refactor this for part C when we get to it.
	// ez pz
	if(mem_manager_active == 1) {
		exit(EXIT_FAILURE);
	}
	// address of the first page that was protected
	char *requestAddr = (char *) si->si_addr;
	// address of the buffer page
	char *bufferPage = &myBlock + sizeof(myBlock) - PAGESIZE;
	// Exit if the seg fault is a result of accessing space outside the user space
	// (not our problem). 
	if((requestAddr < baseAddress) || requestAddr > bufferPage - 1) {
		exit(EXIT_FAILURE);
	}
	/* Reorder all the pages all sneaky-like. */
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
	return;

}

// TODO @bruno: let this deal with case where a page is free
void swapPages(int pageA, int pageB, my_pthread_t curr) {
	// if the inputs are the same, leave
	if(pageA == pageB) {
		return;
	}
	// if neither page is currently USED and owned by the current thread, error
	if((PageTable[pageA].owner != curr && PageTable[pageA].used != BLOCK_USED) 
		&& (PageTable[pageB].owner != curr && PageTable[pageB].used != BLOCK_USED)) {
		exit(EXIT_FAILURE);
	}
	// get the address of page A
	char *pageAPtr = baseAddress + (pageA * PAGESIZE);
	// get the address of page B
	char *pageBPtr = baseAddress + (pageB * PAGESIZE);
	// unprotect page A if it's not owned by curr OR if it's free
	int freePage = -1;
	if(PageTable[pageA].owner != curr || PageTable[pageA].used == BLOCK_FREE) {
		if(mprotect(pageAPtr, PAGESIZE, PROT_READ | PROT_WRITE) == -1) {
				exit(EXIT_FAILURE);
		}
		freePage = pageA;
	}
	// unprotect Page B if it's not owned by curr OR if it's free
	else if(PageTable[pageB].owner != curr || PageTable[pageB].used == BLOCK_FREE) {
		if(mprotect(pageBPtr, PAGESIZE, PROT_READ | PROT_WRITE) == -1) {
				exit(EXIT_FAILURE);
		}
		freePage = pageB
	}
	// address of the buffer page
	char *bufferPage = &myBlock + sizeof(myBlock) - PAGESIZE;
	// copy the data from pageA to bufferPage.
	memcpy(bufferPage, pageAPtr, PAGESIZE);
	// copy the data at page B's space, to page A's original space
	memcpy(pageAPtr, pageBPtr, PAGESIZE);
	// copy the data in bufferPage, to page B's original space.
	memcpy(pageBPtr, bufferPage, PAGESIZE);
	// Our objective is to "delink" pageA and pageB from one another's
	// owning threads' references, by swapping their places.
	// If one is a free page, though, we don't care about delinking it.
	// Store the previous pages for Target A and B, the current pages we're at in the
	// iterations, and the pages that follow Target A and B.
	int prevPageA, prevPageB, currPageA, currPageB;
	// Store the owners of Target A and B.
	my_pthread_t owner_threadA, owner_threadB;
	owner_threadA = PageTable[pageA].owner;
	owner_threadB = PageTable[pageB].owner;
	// Set these values to -1 by default. If they remain as -1, then
	// their respective page is free.
	prevPageA = -1;
	prevPageB = -1;
	currPageA = -1;
	currPageB = -1; 
	// If A isn't a free page, iterate through and get the prev/next refs
	if(freePage != pageA) {
		prevPageA threadNodeList[owner_threadA].firstPage;
		currPageA = prevPageA;
		// Iterate through owner of target A until we reach target A.
		while(currPageA != pageA) {
			prevPageA = currPageA;
			currPageA = PageTable[currPageA].nextPage;
		}
		nextPageA = PageTable[currPageA].nextPage;
	}
	// If B isn't a free page, iterate through and get the prev/next refs
	if(freePage != pageB) {
		prevPageB = threadNodeList[owner_threadB].firstPage;
		currPageB = prevPageB;
		// Iterate through owner of target B until we reach target B.
		while(currPageB != pageB) {
			prevPageB = currPageB;
			currPageB = PageTable[currPageB].nextPage;
		}
		nextPageB = PageTable[currPageB].nextPage;
	}
	// if a non-free page is the first page of its owning thread,
	// then set the owning thread's first page to the opposite
	if(prevPageA == pageA && prevPageA != -1) {
		threadNodeList[owner_threadA].firstPage = currPageB;
	}
	if(prevPageB == pageB && prevPageB != -1) {
		threadNodeList[owner_threadB].firstPage = currPageA;
	}
	// delink/swap the pages from one another's lists. 
	// this logic works even if pageA is the first page
	// in its owner's list, and for pageB as well, because if
	// we incorrectly set pageA's next page to pageB, that'll
	// rectify itself on the next line.
	// if pageA isn't free, then its prev/next references
	// must be set accordingly. same for pageB.
	if(freePage != pageA) {
		PageTable[prevPageA].nextPage = pageB
		PageTable[pageA].nextPage = nextPageB
	}
	if(freePage != pageB) {
		PageTable[prevPageB].nextPage = pageA
		PageTable[pageB].nextPage = nextPageA
	}
	// swap the pages' owners
	PageTable[pageA].owner = owner_threadB;
	PageTable[pageB].owner = owner_threadA;
	// swap used/free status
	int pageAUsedStatus = PageTable[pageA].used;
	int pageBUsedStatus = PageTable[pageB].used;
	PageTable[PageA].used = pageBUsedStatus;
	PageTable[PageB].used = pageAUsedStatus;
	// set the page protections correspondingly given the
	// current thread and new page statuses
	if(PageTable[pageA].owner != curr || PageTable[pageA].used == BLOCK_FREE) {
		if(mprotect(pageAPtr, PAGESIZE, PROT_NONE) == -1) {
				exit(EXIT_FAILURE);
		}
	}
	else if(PageTable[pageA].owner == curr) {
		if(mprotect(pageAPtr, PAGESIZE, PROT_READ|PROT_WRITE) == -1) {
				exit(EXIT_FAILURE);
		}
	}

	if(PageTable[pageB].owner != curr || PageTable[pageB].used == BLOCK_FREE) {
		if(mprotect(pageBPtr, PAGESIZE, PROT_NONE) == -1) {
				exit(EXIT_FAILURE);
		}
	}
	else if(PageTable[pageB].owner == curr) {
		if(mprotect(pageBPtr, PAGESIZE, PROT_READ|PROT_WRITE) == -1) {
				exit(EXIT_FAILURE);
		}
	}
	
	return;
}


int init_manager_thread() {
	printf("Using my_pthread implementation!\n");
	// initialize global variables before adding Main's thread
	// to the manager
	MLPQ = myallocate(NUM_PRIORITY_LEVELS * (sizeof(pnode)), __FILE__, __LINE__, LIBRARYREQ);
	tcbList = myallocate(MAX_NUM_THREADS * (sizeof(tcb)), __FILE__, __LINE__, LIBRARYREQ);
	// we must be inside of Main, so set current_thread to 0.
	current_thread = 0;
	int i;
	// initialize MLPQ state
	for(i = 0; i < NUM_PRIORITY_LEVELS; i++) {
		MLPQ[i] = NULL;
	}
	// initialize tcbList state
	for(i = 0; i < MAX_NUM_THREADS; i++) {
		tcbList[i] = NULL;
	}
	// Initializing current (Main) context
	ucontext_t Main;
	getcontext(&Main);
	//now add pnode with Main thread's ID (0) to MLPQ
	pnode *mainNode = createPnode(0);
	insertPnodeMLPQ(mainNode, 0);
	// initialize tcb for main
	tcb *newTcb = createTcb(0, Main, NULL);
	tcbList[0] = newTcb;
	threadsSoFar = 1;
	runQueue = NULL;
	// set manager_active to 1
	manager_active = 1;
	// initialize manager thread's context
	getcontext(&Manager);
	// this is the stack that will be used by the manager context
	// point the manager's stack pointer to the manager_stack we just set
	Manager.uc_stack.ss_sp = myallocate(MEM, __FILE__, __LINE__, LIBRARYREQ);
	// set the manager's stack size to MEM
	Manager.uc_stack.ss_size = MEM;
	// no other context will resume after the manager leaves
	Manager.uc_link = NULL;
	// attach manager context to my_pthread_manager()
	makecontext(&Manager, (void*)&my_pthread_manager, 0);
	// initialize the signal alarm struct
	memset(&sa, 0, sizeof(sa));
	// install VTALRMhandler as the signal handler for SIGVTALRM
	sa.sa_handler = &VTALRMhandler;
	// initialize the sigaction struct for seg faults
	memset(&sig_mem, 0, sizeof(sig_mem));
	// set signal mask so that SEGVhandler() ignores SIGVTALRM
	sigset_t msegv_handler_mask;
	sigemptyset(segv_handler_mask);
	sigaddset(&segv_handler_mask, SIGVTALRM);
	sig_mem.sa_mask = segv_handler_mask;
	// set signal flag so it catches siginfo
	sig_mem.sa_flags = SA_SIGINFO;
	// install SEGVhandler() as the signal handler for SIGSEGV.
	sig_mem.sa_handler = &SEGVhandler;
	return 0;
}


tcb *createTcb(my_pthread_t tid, ucontext_t context, void *(*function)(void*)) {
	// allocate memory for tcb instance
	tcb *ret = myallocate(sizeof(tcb), __FILE__, __LINE__, LIBRARYREQ);
	// set members to inputs
	ret->status = THREAD_READY;
	ret->tid = tid;
	ret->context = context;
	// set priority to 0 by default
	ret->priority = 0;
	// set timeSlices to 0 by default
	ret->timeSlices = 0;
	// waitingThread is -1 by default
	ret->waitingThread = MAX_NUM_THREADS + 2;
	// valuePtr is NULL by default
	ret->valuePtr = NULL;
	// cyclesWaited is 0 by default
	ret->cyclesWaited = 0;
	char *stack = myallocate(MEM, __FILE__, __LINE__, LIBRARYREQ);
	// initialize stack properties of context
	ret->context.uc_stack.ss_sp = stack;
	ret->context.uc_stack.ss_size = MEM;
	ret->stack = stack;
	// return a pointer to the instance
	return ret;
}


pnode *createPnode(my_pthread_t tid) {
	pnode *ret = myallocate(sizeof(pnode), __FILE__, __LINE__, LIBRARYREQ);
	ret->tid = tid;
	ret->next = NULL;
	return ret;
}

int insertPnodeMLPQ(pnode *input, uint level) {
	if(MLPQ == NULL) {
		return -1;
	}
	if(input == NULL) {
		return -1;
	}
	if(level > NUM_PRIORITY_LEVELS) {
		return -1;
	}
	// error-checking done, begin insertion.
	// first scenario: MLPQ[level] is NULL.
	if(MLPQ[level] == NULL) {
		// insert input as head
		MLPQ[level] = input;
		// fix: make sure we're not keeping ->next values
		// from the runQueue when applicable
		input->next = NULL;
		return 0;
	}
	// second scenario: MLPQ[level] has one or more nodes.
	// go until we find the last node (temp->next == NULL)
	pnode *temp = MLPQ[level];
	while(temp->next != NULL) {
		temp = temp->next;
	}
	// set temp->next to input
	temp->next = input;
	// input->next is set to NULL (in case we inserted a thread
	// from the runQueue)
	input->next = NULL;
	return 0;
}

/* Implements getting the number of slices for a given input level.
Should be 2^(level), so 1 slice at Level 0, 2 at Level 1, 4 at Level 2,
8 at level 3, 16 at Level 4. */
int level_slices(int level) {
	// base case: level 0, give 1 slice
	if(level == 0) {
		return 1;
	}
	// recursive case: return 2 * recursive func
	else{
		return 2*(level_slices(level - 1));
	}

}
