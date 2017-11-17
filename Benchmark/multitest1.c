#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../my_pthread_t.h"

//Multithreaing to see how our schedule reacts our malloc lib
void createAndWriteArr(void * arg){
	
	printf("Entering createAndWriteArr()\n");
	
	int arrSize = 10;
	
	// Request arrSize
	int * arr = (int *)malloc(arrSize * sizeof(int));
	
	// Force context switch. 
	// This means a different thread 
	// will access pages it does not own.
	// Signal Handler should handle this case. 
	my_pthread_yield();
	
	// Populate arr
	int i;
	for(i = 0; i < arrSize; i++){
		arr[i] = arrSize;
	}
	
	// Force context switch.
	my_pthread_yield();
	
	// Verify arr has the same bytes we wrote into it. 
	for(i = 0; i < arrSize; i++){
		if(arr[i] != i){
			printf("Verification failed, thread: %d.\n", current_thread);
			return NULL;
		}
	}
		
	// Force context switch
	my_pthread_yield();
	
	free(arr);
	
	
	
	return;
}


int main(int argc, char **argv){
	
	printf("Testing threadTest1\n");
	
	// Amount of threads we want to test
	int numberOfThreads = 5;

	// Holds the pointers to each child thread
	pthread_t * threadPointers[numberOfThreads];
	
	// Create child threads
	int i;
	for(i = 0; i < numberOfThreads; i++){		
		if((my_pthread_create(&threadPointers[i], NULL, &createAndWriteArr, NULL)) == -1){
			printf("Failed to create thread %d\n", i);
		}
		my_pthread_join(threadPointers[i], NULL);
	}
	
	
	return 0;
}