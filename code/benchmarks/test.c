#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "../thread-worker.h"

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
   pthread_exit(NULL);
}

int main(int argc, char **argv) {

	/* Implement HERE */
	pthread_t thread;
	int rc;
	long t;
	printf("In main: creating thread %ld\n", t);
	rc = pthread_create(thread, NULL, PrintHello, (void *)t);
	if (rc) {
		printf("ERROR; return code from pthread_create() is %d\n", rc);
		exit(-1);
	}
	return 0;
}
