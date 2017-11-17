
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../my_pthread_t.h"

// Here, we test our ability to perform many small, equally-sized
// allocations (allocating ints in this case) in a single thread.
int main(int argc, char **argv){
	// maxVal is however many allocations we want to test.
	int maxVal = 100;
	// store the pointers we allocate in ptrArr
	int* ptrArr[maxVal];
	int i;
	// for each cell in ptrArr, add a pointer to the number of the
	// allocation it was (starting at 0, going to maxVal - 1).
//	printf("Setting values in ptrArr!\n");
	for(i = 0; i < maxVal; i++) {
		int val = i;
		int * pointer = (int*)malloc(sizeof(int));
		*pointer = val;
		ptrArr[i] = pointer; 
	}
	// print out the value of each cell in ptrArr, make sure it's correct
//	printf("Printing out values in ptrArr!\n");
	for(i = 0; i < maxVal; i++) {
		int val = i;
		int *pointer = ptrArr[i];
		printf("pointer's value should be %d; value is actually: %d\n", val, *pointer);
	}
	// free each item in ptrArr
//	printf("Attempting to free items in ptrArr!\n");
	for(i = 0; i < maxVal; i++) {
		printf("freeing ptrArr[%d]\n", i);
		free(ptrArr[i]);
	}

	printf("Finished test 2 successfully!\n");
	
	return 0;
}