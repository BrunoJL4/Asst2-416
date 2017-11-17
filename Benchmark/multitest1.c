#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../my_pthread_t.h"

//Multithreaing to see how our schedule reacts our malloc lib
void createAndWriteArr(void * arg){
	
	printf("Entering createAndWriteArr()\n");
	
	int arrSize = 10;
	
	printf("allocating memory for thread %d\n", current_thread);
	// Request arrSize
	int * arr = (int *)malloc(arrSize * sizeof(int));
	if(arr == NULL) {
		printf("ERROR! For thread %d, malloc() returned NULL!\n", current_thread);
	}
	
	printf("populating arr for thread %d\n", current_thread);
	// Populate arr
	int i;
	for(i = 0; i < arrSize; i++){
		arr[i] = current_thread;
	}
	
	// Force context switch.
	
	printf("verifying arr for thread %d\n", current_thread);
	// Verify arr has the same bytes we wrote into it. 
	for(i = 0; i < arrSize; i++){
		if(arr[i] != i){
			printf("Verification failed, thread: %d.\n", current_thread);
			return NULL;
		}
	}
	
	printf("freeing arr for thread %d\n", current_thread);
	free(arr);
	return;
}


int main(int argc, char **argv){
	
	printf("Testing threadTest1\n");
	
	// Amount of threads we want to test
	int numberOfThreads = 32;

	// Holds the pointers to each child thread
	pthread_t * threadPointers[numberOfThreads];
	
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
	
	printf("Finished Multithreading Test 1 successfully!\n");
	
	return 0;
}