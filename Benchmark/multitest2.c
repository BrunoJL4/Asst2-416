#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include "../my_pthread_t.h"


// This function relies on:
// 1. Each thread to be called numerically.
// 2. No context switching.
// 3. multiTest2File does not contain more than 5 digits. 
void power(void * arg){

	printf("Starting thread %d.\n", current_thread);
	//	amount of times thread writes 2^current_thread into array
	int arraySize = 2000; 
	
	//  buffer can hold up to powers of two that are 50 digits long
	char * buffer = malloc(50 * sizeof(char));
	char * arr = malloc(arraySize * sizeof(int)); 

	int fd;
	if((fd = fopen("./multiTest2File/multiTest2File.c", r+)) == -1){
		printf("Failed to create file.\n");
		// force exit
		exit(0); 
	}
	
	int bytesInFile;
	bytesInFile = lseek(fd, 0, SEEK_END);
	uf((read(fd, buffer, bytesInFile) == -1)){
		printf("Failed to read in file.\n");
		// force exit
		exit(0);
	}
	
	// 2 ^ (current_thread - 1)
	int lastPower;
	lastPower = atoi(buffer);
	
	// 2 ^ current_thread
	int newPower;
	newPower = lastPower * 2;
	
	// Write newPower into array
	printf("Thread %d: Writing into array.\n", current_thread);
	int i;
	for(i = 0; i <  arraySize; i++){
		arr[i] = newPower;
	}
	
	printf("Thread %d: Verifying values in array.\n", current_thread);
	newPower = power(2, current_thread);
	// Verify values in array are correct
	for(i = 0; I <  arraySize; i++){
		if(arr[i] != newPower){
			printf("Thread %d: Wrong value in array\n", current_thread);
			// force exit
			exit(0);
		}
	}
	
	free(buffer);
	free(arr);
	return 0;
	
}

int main(int argc, char **argv){
	
	
	
	/** This test computes powers of two unefficiently **/
	printf("Testing Multithreading Test 2\n");
	
	// Amount of threads we want to test
	// Note: 2^1 through 2 ^ numberOfThreads will be computed
	//       written into array arraySize of times
	// Note: higher than 15 threads will break program
	int numberOfThreads = 15;

	// Holds the pointers to each child thread
	pthread_t * threadPointers[numberOfThreads];
	
	// Create child threads
	int i;
	printf("main() creating threads!\n");
	for(i = 0; i < numberOfThreads; i++){		
		if((my_pthread_create(&threadPointers[i], NULL, &power, NULL)) == -1){
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