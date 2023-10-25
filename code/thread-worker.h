// File:	worker_t.h

// List all group member's name: Long Tran, Darren Jiang
// username of iLab: lht21
// iLab Server: vi.cs.rutgers.edu -> ssh cs416f23-51

#ifndef WORKER_T_H
#define WORKER_T_H

#define _GNU_SOURCE

/* To use Linux pthread Library in Benchmark, you have to comment the USE_WORKERS macro */
#define USE_WORKERS 1

/* include lib header files that you need here: */
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>  //for measuring elapsed quantum of threads
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ucontext.h>

typedef uint worker_t;

typedef enum {
	READY = 0,
	RUNNING = 1,
	BLOCKED = 2,
	DONE = 3
} t_status;

typedef enum {
	UNLOCKED = 0,
	LOCKED = 1
} mut_status;

typedef struct TCB {
	/* add important states in a thread control block */
	// thread Id
	// thread status
	// thread context
	// thread stack
	// thread priority
	// And more ...

	// YOUR CODE HERE
	int testno;
	int quantum_counter;
	int priority_quantum_counter; //how many quantums a thread has been in 1 priority
	struct timespec time_arrival;
	struct timespec time_finish;
	struct timespec time_response;
	bool rant;
	int priority;   // 0 is highest, 3 is lowest
	worker_t* id;
	t_status status;
	ucontext_t* context;
} tcb; 

/* mutex struct definition */
typedef struct worker_mutex_t {
	/* add something here */
	// YOUR CODE HERE
	tcb* thread_block;
	mut_status status;
} worker_mutex_t;

/* define your data structures here: */
// Feel free to add your own auxiliary data structures (linked list or queue etc...)

// YOUR CODE HERE


/* Function Declarations: */

/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, void
    *(*function)(void*), void * arg);

/* give CPU pocession to other user level worker threads voluntarily */
int worker_yield();

/* terminate a thread */
void worker_exit(void *value_ptr);

/* wait for thread termination */
int worker_join(worker_t thread, void **value_ptr);

/* initial the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, const pthread_mutexattr_t
    *mutexattr);

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex);

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex);

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex);


/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void);

#ifdef USE_WORKERS
#define pthread_t worker_t
#define pthread_mutex_t worker_mutex_t
#define pthread_create worker_create
#define pthread_exit worker_exit
#define pthread_join worker_join
#define pthread_mutex_init worker_mutex_init
#define pthread_mutex_lock worker_mutex_lock
#define pthread_mutex_unlock worker_mutex_unlock
#define pthread_mutex_destroy worker_mutex_destroy
#endif

#endif
