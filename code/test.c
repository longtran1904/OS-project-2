#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include "thread-worker.h"
#define NUM_THREADS     5

/* A scratch program template on which to call and
 * test thread-worker library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */

void *PrintHello(void *threadid)
{
   long tid;
   tid = (long)threadid;
   printf("Hello World! It's me, thread #%ld!\n", tid);
   worker_exit(NULL);
}

int main(int argc, char **argv) {

	/* Implement HERE */

	worker_t thread1, thread2, thread3, thread4;
	int stat;
	stat = worker_create(&thread1, NULL, (void*) PrintHello, (void *)1);
	if (stat == 0){
		printf("thread 1 created!! value: %p\n", &thread1);
	}
	stat = worker_create(&thread2, NULL, (void*) PrintHello, (void *)2);
	if (stat == 0){
		printf("thread 2 created!! value: %p\n", &thread2);
	}
	// stat = worker_create(&thread3, NULL, (void*) PrintHello, (void *)3);
	// if (stat == 0){
	// 	puts("thread 3 created!!\n");
	// }
	// stat = worker_create(&thread4, NULL, (void*) PrintHello, (void *)4);
	// if (stat == 0){
	// 	puts("thread 4 created!!\n");
	// }

	stat = worker_join(&thread1, NULL);
	stat = worker_join(&thread2, NULL);

	puts("finished\n");

	return 0;
}