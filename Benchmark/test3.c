
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../my_pthread_t.h"

// Here, we test our ability to allocate an integer array
// inside of our memory, for a single thread. Then we
// access all of its values and modify them.
int main(int argc, char **argv){
	// maxVal is however many allocations we want to test.
	int maxVal = 100;
	// allocate ptrArr
	printf("Attempting to allocate ptrArr!\n");
	int* ptrArr = (int*) malloc(maxVal * sizeof(int));
	int i;
	// for each cell in ptrArr, add a pointer to the number of the
	// allocation it was (starting at 0, going to maxVal - 1).
	printf("Setting values in ptrArr!\n");
	for(i = 0; i < maxVal; i++) {
		ptrArr[i] = i; 
	}
	// print out the value of each cell in ptrArr, make sure it's correct
	printf("Printing out values in ptrArr!\n");
	for(i = 0; i < maxVal; i++) {
		int val = i;
		int actualVal = ptrArr[i];
		printf("pointer's value should be %d; value is actually: %d\n", val, *pointer);
	}
	// free ptrArr
	printf("Attempting to free ptrArr!\n");
	free(ptrArr);

	printf("Finished test 3 successfully!\n");
	
	return 0;
}