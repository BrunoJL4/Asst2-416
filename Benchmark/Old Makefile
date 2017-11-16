CC = gcc
CFLAGS = -g -w

all:: parallelCal vectorMultiply externalCal

parallelCal: 
	$(CC) $(CFLAGS) -pthread -o parallelCal parallelCal.c -L ../ -lmy_pthread -lm

vectorMultiply: 
	$(CC) $(CFLAGS) -pthread -o vectorMultiply vectorMultiply.c -L ../ -lmy_pthread -lm

externalCal: 
	$(CC) $(CFLAGS) -pthread -o externalCal externalCal.c -L ../ -lmy_pthread -lm

clean:
	rm -rf parallelCal vectorMultiply externalCal *.o ./record/
