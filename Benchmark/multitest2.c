#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../my_pthread_t.h"

// This test checks how our library deals with threads making a number of
// small allocations, and then reading from them in between being interrupted
// and having their pages swapped around. It ends upon the user pressing CTRL+C
// to terminate the process. We have free() statements, but none of them
// should ever be reached.
// NOTE! Please set MAX_NUM_THREADS in my_pthread_t.h to one value greater
// than the numberOfThreads variable below.


pthread_mutex_t   mutex;


void createAndWriteArr(void * arg){
	
	printf("Entering createAndWriteArr()\n");
	
	
	int arrSize = 50000;
	
	printf("allocating memory for thread %d\n", current_thread);
	// Request arrSize
	int * arr = (int *)malloc(arrSize * sizeof(int));
	if(arr == NULL) {
		printf("ERROR! For thread %d, malloc() returned NULL!\n", current_thread);
		return;
	}
	
	printf("populating arr for thread %d\n", current_thread);
	// Populate arr
	int i;
	for(i = 0; i < arrSize; i++){
		arr[i] = current_thread;
	}
	while(1) {
		printf("verifying arr for thread %d\n", current_thread);
		printf("numLocalPagesLeft is: %d\n", numLocalPagesLeft);
		printf("numSwapPagesLeft is: %d\n", numSwapPagesLeft);
		// Verify arr has the same bytes we wrote into it. 
		for(i = 0; i < arrSize; i++){
			if(arr[i] != current_thread){
				printf("Verification failed, thread: %d.\n", current_thread);
				return NULL;
			}
		}
	}
	
	printf("freeing arr for thread %d\n", current_thread);
	free(arr);
	return;
}


int main(int argc, char **argv){
	
	printf("Testing Multithreading Test 2\n");
	
	// Amount of threads we want to test
	int numberOfThreads = 32;

	// Holds the pointers to each child thread
	pthread_t * threadPointers[numberOfThreads];

	pthread_mutex_init(&mutex, NULL);
	
	// Create child threads
	int i;
	printf("main() creating threads!\n");
	for(i = 0; i < numberOfThreads; i++){		
		if((my_pthread_create(&threadPointers[i], NULL, &createAndWriteArr, NULL)) == -1){
			printf("Failed to create thread %d\n", i);
		}
	}
	printf("main() joining threads!\n");
	for(i = 0; i < numberOfThreads; i++) {
		my_pthread_join(threadPointers[i], NULL);
	}
	
	printf("Finished Multithreading Test 2 successfully!\n");
	
	return 0;
}
