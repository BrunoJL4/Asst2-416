
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../my_pthread_t.h"

// Here, we test our ability to allocate an integer array
// inside of our memory, for a single thread. Then we
// access all of its values and modify them.
// After that, we perform a similar test, but for variables
// of type my_pthread_t.
int main(int argc, char **argv){
	// maxVal is however many allocations we want to test.
	int maxVal = 1000;
	// allocate ptrArr
	int* ptrArr = (int*) malloc(maxVal * sizeof(int));
	int i;
	// for each cell in ptrArr, add a pointer to the number of the
	// allocation it was (starting at 0, going to maxVal - 1).
	for(i = 0; i < maxVal; i++) {
		ptrArr[i] = i; 
	}
	// verify the correctness of what's in ptrArr
	for(i = 0; i < maxVal; i++) {
		int val = i;
		int actualVal = ptrArr[i];
		if(val != actualVal) {
			printf("ERROR in test3.c! For i == %d, val is %d and actualVal is %d\n", i, val, actualVal);
			return 0;
		}
	}
	// free ptrArr
	free(ptrArr);


	// repeat the test, but with type my_pthread_t

	// allocate threadArr
	pthread_t *threadArr = (pthread_t *) malloc(maxVal * sizeof(pthread_t));
	// populate threadArr similarly to first example
	uint j;
	for(j = 0; j < maxVal; j++) {
		threadArr[j] = (pthread_t) j;
	}
	// verify the value of each item in ptrArr
	for(j = 0; j < maxVal; j++) {
		int val = j;
		int actualVal = threadArr[j];
		if(val != actualVal) {
			printf("ERROR in test3.c! For i == %d, val is %d and actualVal is %d\n", i, val, actualVal);
			return 0;
		}
	}
	// free threadArr
	free(threadArr);

	printf("Finished test 3 successfully!\n");
	
	return 0;
}