
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
	int firstCount = 1000000;
	int secondCount = 500000;
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
	printf("populating firstArr, first interval!\n");
	for(i = 0; i < firstCount; i++) {
		firstArr[i] = i;
	}
	printf("populating secondArr, first interval!\n");
	for(i = 0; i < secondCount; i++) {
		secondArr[i] = i;
	}
	// verify that their values are correct
	printf("Verifying firstArr populated correctly, first interval!\n");
	for(i = 0; i < firstCount; i++) {
		if(firstArr[i] != i) {
			printf("Error in test4! firstArr not correctly allocated for first interval.\n");
			return 0;
		}
	}
	printf("Verifying secondArr populated correctly, first interval!\n");
	for(i = 0; i < secondCount; i++) {
		if(secondArr[i] != i) {
			printf("Error in test4! secondArr not correctly allocated for first interval.\n");
			return 0;
		}
	}
	// free firstArr, then check secondArr
	printf("Attempting to free firstArr, first interval!\n");
	free(firstArr);
	printf("Verifying secondArr still fine after freeing firstArr, first interval!\n");
	for(i = 0; i < secondCount; i++) {
		if(secondArr[i] != i) {
			printf("Error in test4! secondArr not correctly allocated for first interval.\n");
			return 0;
		}
	}
	printf("Attempted to free secondArr, first interval!\n");
	free(secondArr);
	printf("Successfully passed first interval!\n");
	
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
	printf("populating firstArr, first interval!\n");
	for(i = 0; i < firstCount; i++) {
		firstArr[i] = i;
	}
	printf("populating secoindArr, first interval!\n");
	for(i = 0; i < secondCount; i++) {
		secondArr[i] = i;
	}
	// verify that their values are correct
	printf("Verifying firstArr populated correctly, first interval!\n");
	for(i = 0; i < firstCount; i++) {
		if(firstArr[i] != i) {
			printf("Error in test4! firstArr not correctly allocated for first interval.\n");
			return 0;
		}
	}
	printf("Verifying secondArr populated correctly, first interval!\n");
	for(i = 0; i < secondCount; i++) {
		if(secondArr[i] != i) {
			printf("Error in test4! secondArr not correctly allocated for first interval.\n");
			return 0;
		}
	}
	// free firstArr, then check secondArr
	printf("Attempting to free firstArr, first interval!\n");
	free(firstArr);
	printf("Verifying secondArr still fine after freeing firstArr, first interval!\n");
	for(i = 0; i < secondCount; i++) {
		if(secondArr[i] != i) {
			printf("Error in test4! secondArr not correctly allocated for first interval.\n");
			return 0;
		}
	}
	printf("Attempted to free secondArr, first interval!\n");
	free(secondArr);
	printf("Successfully passed first interval!\n");






	printf("Finished test 3 successfully!\n");
	
	return 0;
}