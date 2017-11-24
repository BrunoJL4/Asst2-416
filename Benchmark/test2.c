
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../my_pthread_t.h"

// Here, we test our ability to perform many small, equally-sized
// allocations (allocating ints in this case) in a single thread.
int main(int argc, char **argv){
	// maxVal is however many allocations we want to test.
	int maxVal = 3000;
	// store the pointers we allocate in ptrArr
	int* ptrArr[maxVal];
	int i;
	// for each cell in ptrArr, add a pointer to the number of the
	// allocation it was (starting at 0, going to maxVal - 1).
	for(i = 0; i < maxVal; i++) {
		int val = i;
		int * pointer = (int*)malloc(sizeof(int));
		*pointer = val;
		ptrArr[i] = pointer; 
	}
	// verify values stored in ptrArr
	for(i = 0; i < maxVal; i++) {
		int val = i;
		int *pointer = ptrArr[i];
		if(val != *pointer) {
			printf("ERROR in test2.c! for i == %d, i does not match ptrArr[%d]\n", i, i);
			return 0;
		}
	}
	// free each item in ptrArr
	for(i = 0; i < maxVal; i++) {
		free(ptrArr[i]);
	}

	printf("Finished test 2 successfully!\n");
	
	return 0;
}