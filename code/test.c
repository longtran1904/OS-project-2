#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdatomic.h>
#include "thread-worker.h"
#define NUM_THREADS     5
#define YELLOW "\033[0;33m"
#define GREEN "\e[0;32m"
#define RESET  "\033[0m"

/* A scratch program template on which to call and
 * test thread-worker library functions as you implement
 * them.
 *
 * You can modify and use this program as much as possible.
 * This will not be graded.
 */

atomic_flag lock = ATOMIC_FLAG_INIT;
static int bank_account = 0; 
static int temp = 0;

void *PrintHello(void *threadid)
{
   long tid;
   tid = (long)threadid;

   printf("Hello World! It's me, thread #%ld!\n", tid);

   worker_exit(NULL);
}

void *PrintHello1s(void *threadid)
{
	long tid;
	tid = (long)threadid;

	int sum = 0;
	for (int i = 0; i < 500000000; i++)
	{
		sum++;
	}
	printf("Hello World! It's me, 1s thread #%ld! \n", tid);

	worker_exit(NULL);
}

void *PrintHello4s(void *threadid) {
	long tid;
	tid = (long)threadid;

	int sum = 0;
	for (int i = 0; i < 2000000000; i++)
	{
		sum++;
	}
	printf("Hello World! It's me, 4s thread #%ld!\n", tid);

	worker_exit(NULL);	
}

void *PrintHello10s(void *threadid) {
	long tid;
	tid = (long)threadid;

	int sum = 0;
	for (int i = 0; i < 2000000000; i++)
	{
		sum++;
	}
	printf("Hello World! It's me, 10s thread #%ld!\n", tid);

	worker_exit(NULL);	
}
void *PrintHelloYield(void *threadid){
	long tid;
	tid = (long)threadid;
	printf("Hello World! It's me, thread #%ld!\n", tid);

	worker_yield();

	printf("Hello! It's thread #%ld again!\n", tid);

	worker_exit(NULL);	
}
void critical(worker_mutex_t* mutex) {
    // Spin lock: loop forever until we get the lock; we know the lock was
    // successfully obtained after exiting this while loop because the 
    // test_and_set() function locks the lock and returns the previous lock 
    // value. If the previous lock value was 1 then the lock was **already**
    // locked by another thread or process. Once the previous lock value
    // was 0, however, then it indicates the lock was **not** locked before we
    // locked it, but now it **is** locked because we locked it, indicating
    // we own the lock.
	
	printf(YELLOW "!!!!obtaining lock in thread 0!!!!\n" RESET);
	worker_mutex_lock(mutex);
    // while (atomic_flag_test_and_set(&lock));  
    for (int i = 0; i < 5000000; i++) 
		temp++;
	bank_account = 5000000;
	worker_yield();

    // release lock when finished with the critical section
	worker_mutex_unlock(mutex);
	// atomic_flag_clear(&lock);
	printf(YELLOW "!!!!released lock in thread 0!!!!\n" RESET);

	worker_exit(NULL);
}
void critical2(worker_mutex_t* mutex) {
    // Spin lock: loop forever until we get the lock; we know the lock was
    // successfully obtained after exiting this while loop because the 
    // test_and_set() function locks the lock and returns the previous lock 
    // value. If the previous lock value was 1 then the lock was **already**
    // locked by another thread or process. Once the previous lock value
    // was 0, however, then it indicates the lock was **not** locked before we
    // locked it, but now it **is** locked because we locked it, indicating
    // we own the lock.
	printf(YELLOW "!!!!obtaining lock in thread!!!!\n" RESET);
    // while (atomic_flag_test_and_set(&lock));  
	worker_mutex_lock(mutex);
    for (int i = 0; i < 2000000; i++) 
	temp++;
	bank_account = 2000000;

    // release lock when finished with the critical section
	// atomic_flag_clear(&lock);
	worker_mutex_unlock(mutex);
	printf(YELLOW "!!!!released lock in thread!!!!\n" RESET);

	worker_exit(NULL);
}


int main(int argc, char **argv) {

	/* Implement HERE */

	worker_t thread1, thread2, thread3, thread4;
	int stat;
	// stat = worker_create(&thread1, NULL, (void*) PrintHelloYield, (void *)0);
	// if (stat == 0){
	// 	printf("thread %d created!! value: %d\n", thread1, thread1);
	// }
	// stat = worker_create(&thread2, NULL, (void*) PrintHello, (void *)1);
	// if (stat == 0){
	// 	printf("thread %d created!! value: %d\n", thread2, thread2);
	// }

	// stat = worker_join(thread1, NULL);
	// stat = worker_join(thread2, NULL);
	worker_mutex_t mutex;

	stat = worker_create(&thread1, NULL, (void*) critical, (void*) &mutex);
	stat = worker_create(&thread2, NULL, (void*) critical2, (void*) &mutex);
	stat = worker_create(&thread3, NULL, (void*) critical, (void*) &mutex);	
	stat = worker_create(&thread4, NULL, (void*) critical2, (void*) &mutex);	

	stat = worker_join(thread1, NULL);
	if (stat == 0) {
		// printf("bank account updated: %d\n", bank_account);
	}	
	stat = worker_join(thread2, NULL);
	if (stat == 0) {
		// printf("bank account updated: %d\n", bank_account);
	}
	stat = worker_join(thread3, NULL);
	if (stat == 0) {
		// printf("bank account updated: %d\n", bank_account);
	}		
    stat = worker_join(thread4, NULL);
	if (stat == 0) {
		// printf("bank account updated: %d\n", bank_account);
	}		

	puts("finished\n");

	return 0;
}


// #include <stdio.h>
// #include <unistd.h>
// #include <pthread.h>
// #include "thread-worker.h"
// #include <stdlib.h>
// /* A scratch program template on which to call and
 
// test thread-worker library functions as you implement
// them.
// *
// You can modify and use this program as much as possible.
// This will not be graded.
// */

// pthread_t t1, t2;
// pthread_mutex_t mutex;
// long x = 0;


// void add_counter(void* args){

//     int i;
//     //pthread_mutex_lock(&mutex);

//     for(i = 0; i < 4000; i++){
//     // for (i = 0; i < 1000000000; i++){

//         worker_mutex_lock(&mutex);
//         x = x+1;
//         worker_mutex_unlock(&mutex);

//     }
// 	//pthread_mutex_unlock(&mutex);
//     pthread_exit(NULL);
// }

// int main(int argc, char *argv) {



//     pthread_mutex_init(&mutex, NULL);

//     // Implement HERE */
//     // printf("starting main...\n");
//     worker_create(&t1, NULL, (void*) add_counter, NULL);
//     worker_create(&t2, NULL, (void*) add_counter, NULL);

//     // printf("finished creating threads...\n");
//     pthread_join(t1, NULL);
//     // printf("worker join 2\n");
//     // worker_yield();
//     pthread_join(t2, NULL);
//     // printf("joined threads");
//     print_app_stats();

//     printf("\nX = %ld\n", x);
//     pthread_mutex_destroy(&mutex);


//     return 0;
// }