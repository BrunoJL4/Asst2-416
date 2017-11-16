// File:	vectorMultiply.c
// Author:	Yujie REN
// Date:	09/23/2017

#include <stdio.h>
#include <unistd.h>

#include <pthread.h>

#include "../my_pthread_t.h"

#define DEFAULT_THREAD_NUM 2

#define VECTOR_SIZE 1000

#define BILLION 1000000000L

pthread_mutex_t   mutex;

int thread_num;

int* counter;
pthread_t *thread;

int r[VECTOR_SIZE];
int s[VECTOR_SIZE];
int res = 0;

/* A CPU-bound task to do vector multiplication */
void vector_multiply(void* arg) {
	int i = 0;
	int n = *((int*) arg);
	
	for (i = n; i < VECTOR_SIZE; i += thread_num) {
		pthread_mutex_lock(&mutex);
		res += r[i] * s[i];
		pthread_mutex_unlock(&mutex);		
	}
}

void verify() {
	int i = 0;
	res = 0;
	for (i = 0; i < VECTOR_SIZE; i += 1) {
		res += r[i] * s[i];	
	}
	printf("verified res is: %d\n", res);
}

int main(int argc, char **argv) {
	struct timespec start, end;
    uint64_t diff;

    clock_gettime(CLOCK_MONOTONIC, &start);
	int i = 0;


	if (argc == 1) {
		thread_num = DEFAULT_THREAD_NUM;
	} else {
		if (argv[1] < 1) {
			printf("enter a valid thread number\n");
			return 0;
		} else
			thread_num = atoi(argv[1]);
	}

	// initialize counter
	counter = (int*)malloc(thread_num*sizeof(int));
	for (i = 0; i < thread_num; ++i) {
		counter[i] = i;
		printf("counter[i] is: %d\n", counter[i]);
	}

	// initialize pthread_t
	thread = (pthread_t*)malloc(thread_num*sizeof(pthread_t));

	// initialize data array
	for (i = 0; i < VECTOR_SIZE; ++i) {
		r[i] = i;
		s[i] = i;
	}

	pthread_mutex_init(&mutex, NULL);

	for (i = 0; i < thread_num; ++i)
		pthread_create(&thread[i], NULL, &vector_multiply, &counter[i]);

	for (i = 0; i < thread_num; ++i)
		pthread_join(thread[i], NULL);

	printf("res is: %d\n", res);

	pthread_mutex_destroy(&mutex);

	// feel free to verify your answer here:
	verify();

	// Free memory on Heap
	free(thread);
	free(counter);

	clock_gettime(CLOCK_MONOTONIC, &end);
    diff = BILLION * (end.tv_sec - start.tv_sec) + end.tv_nsec - start.tv_nsec;
    printf("elapsed time = %llu nanoseconds\n", (long long unsigned int) diff);

	return 0;
}
