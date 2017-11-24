
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../my_pthread_t.h"

// Here, we test our ability to perform a very large number of
// small, variable-sized allocations (ints and chars)
// within a single thread.
int main(int argc, char **argv){
	// maxVal is however many allocations we want to test.
	// keep in mind that we'll be allocating a LOT of memory
	// per char-allocation once we get to the higher numbers
	// (100+).
	int maxVal = 20;
	// store the pointers we allocate in ptrArr. they have
	// type void* as they will have different types.
	void* ptrArr[maxVal];
	int i;
	// debugging var
//	int bytesSoFar = 0;
	// for each cell in ptrArr, we allocate a char
	// if i is a multiple of 3, OR we allocate
	// an int otherwise.
	for(i = 0; i < maxVal; i++) {
//		printf("On allocation: %d\n", i);
//		printf("bytesSoFar: %d\n", bytesSoFar);
		// allocate the char 'a' i times into an array
		// in VM
		if( (i % 3 == 0) && (i != 0)) {
			int numChars = i;
			// allocate enough space for the chars
//			bytesSoFar += numChars;
			char *pointer = (char*)malloc(numChars * sizeof(char));
			// read the chars into memory
			int j;
			for(j = 0; j < numChars; j++) {
				pointer[j] = 'a';
			}
			// add the pointer to ptrArr
			ptrArr[i] = (void *) pointer;
		}
		// allocate the value of 'i' otherwise
		else {
			// allocate enough space for the int
			int *pointer = (int*)malloc(sizeof(int));
//			bytesSoFar += 4;
			// set the int to i
			*pointer = i;
			// add pointer to ptrArr
			ptrArr[i] = (void *) pointer; 
		}
	}
	// go through ptrArr and verify that the values are there as intended
	for(i = 0; i < maxVal; i++) {
		// if at a multiple of 3 that isn't 0:
		if( (i % 3 == 0) && (i != 0) ) {
			int numChars = i;
			// grab the current pointer
			char *pointer = (char *) ptrArr[i];
			int j;
			// verify that the a's were allocated
			for(j = 0; j < numChars; j++) {
				if(pointer[j] != 'a') {
					printf("ERROR in test5.c! Incorrect character allocated. \n");
					return 0;
				}
			}
			// we've successfully verified a char array
		}
		// otherwise:
		else {
			// verify that the int value is correct
			int *pointer = (int *) ptrArr[i];
			int val = *pointer;
			if(val != i) {
				printf("ERROR in test5.c! Incorrect value allocated.");
				return 0;
			}
		}
	}

	// free each item in ptrArr
	for(i = 0; i < maxVal; i++) {
		free(ptrArr[i]);
	}

	printf("Finished test 5 successfully!\n");
	
	return 0;
}