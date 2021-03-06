
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../my_pthread_t.h"

// Here, we test our ability to allocate an enormous integer
// array (~4MB), then a smaller integer array (~1MB),
// then free the two of them, then allocate a larger array
// (~5MB), then allocate a smaller array (~400b), then free
// them.
int main(int argc, char **argv){
	// the two arrays we'll use at a time
	int *firstArr;
	int *secondArr;
	// the number of integers in each of the two
	int firstCount = 2000;
	int secondCount = 400;
	// allocate the arrays
	firstArr = (int *) malloc(firstCount * sizeof(int));
	secondArr = (int *) malloc(secondCount * sizeof(int));
	if(firstArr == NULL) {
		printf("Error! firstArr allocation didn't work, first interval\n");
		return 0;
	}
	if(secondArr == NULL) {
		printf("Error! secondArr allocation didn't work, first interval\n");
		return 0;
	}
	// populate each of them
	int i;
	for(i = 0; i < firstCount; i++) {
		firstArr[i] = i;
	}
	for(i = 0; i < secondCount; i++) {
		secondArr[i] = i;
	}
	// verify that their values are correct
	for(i = 0; i < firstCount; i++) {
		if(firstArr[i] != i) {
			printf("Error in test4! firstArr not correctly allocated for first interval.\n");
			return 0;
		}
	}
	for(i = 0; i < secondCount; i++) {
		if(secondArr[i] != i) {
			printf("Error in test4! secondArr not correctly allocated for first interval.\n");
			return 0;
		}
	}
	// free firstArr, then check secondArr to be sure secondArr wasn't freed
	free(firstArr);
	for(i = 0; i < secondCount; i++) {
		if(secondArr[i] != i) {
			printf("Error in test4! secondArr not correctly allocated for first interval.\n");
			return 0;
		}
	}
	free(secondArr);
	
	// begin second part of the test, it's basically identical to
	// the first part but with modified firstCount and secondCount values
	firstCount = 1250000;
	secondCount = 100;
	// allocate the arrays
	firstArr = (int *) malloc(firstCount * sizeof(int));
	secondArr = (int *) malloc(secondCount * sizeof(int));
	if(firstArr == NULL) {
		printf("Error! firstArr allocation didn't work, second interval\n");
		return 0;
	}
	if(secondArr == NULL) {
		printf("Error! secondArr allocation didn't work, second interval\n");
		return 0;
	}
	// populate each of them
	for(i = 0; i < firstCount; i++) {
		firstArr[i] = i;
	}
	for(i = 0; i < secondCount; i++) {
		secondArr[i] = i;
	}
	// verify that their values are correct
	for(i = 0; i < firstCount; i++) {
		if(firstArr[i] != i) {
			printf("Error in test4! firstArr not correctly allocated for first interval.\n");
			return 0;
		}
	}
	for(i = 0; i < secondCount; i++) {
		if(secondArr[i] != i) {
			printf("Error in test4! secondArr not correctly allocated for first interval.\n");
			return 0;
		}
	}
	// free firstArr, then check secondArr
	free(firstArr);
	for(i = 0; i < secondCount; i++) {
		if(secondArr[i] != i) {
			printf("Error in test4! secondArr not correctly allocated for first interval.\n");
			return 0;
		}
	}
	free(secondArr);


	printf("Finished test 4 successfully!\n");
	
	return 0;
}