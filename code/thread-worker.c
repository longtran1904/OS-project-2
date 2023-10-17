// File:	thread-worker.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "thread-worker.h" 
#include "run_queue.c"
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#define STACK_SIZE SIGSTKSZ

//Global counter for total context switches and 
//average turn around and response time
long tot_cntx_switches=0;
double avg_turn_time=0;
double avg_resp_time=0;

// INITAILIZE ALL YOUR OTHER VARIABLES HERE
// YOUR CODE HERE
static int callno = 0;
static node* runqueue = NULL;
static ucontext_t sched_ctx;
static ucontext_t main_ctx;
static volatile sig_atomic_t switch_context = 0;


/* scheduler */
static void schedule() {
	// - every time a timer interrupt occurs, your worker thread library 
	// should be contexted switched from a thread context to this 
	// schedule() function

	// - invoke scheduling algorithms according to the policy (PSJF or MLFQ)

	// if (sched == PSJF)
	//		sched_psjf();
	// else if (sched == MLFQ)
	// 		sched_mlfq();

	// YOUR CODE HERE
	printf("swapped to scheduler\n");
	while ( 1 == 1 ){
		//printf("Hello World from schedule context!!!\n");
		if (!is_empty(&runqueue)){
			// puts("queue not empty!!\n");
			node* n = queue_front(&runqueue); // pop from queue - choose job to work
			//deciphering to get and set context
			tcb* block = n->t_block;
			block->status = RUNNING;
			printf("Executing thread %p\n", block->id); 
			setcontext(block->context);

			// printf("freeing heap from thread\n");
			// free(block->context->uc_stack.ss_sp);
		} else {
			puts("queue is empty\n");
			setcontext(&main_ctx);
		}
		// printf("scheduler action: %d\n", i++);
	}
	

// - schedule policy
#ifndef MLFQ
	// Choose PSJF
#else 
	// Choose MLFQ
#endif

}

/* Pre-emptive Shortest Job First (POLICY_PSJF) scheduling algorithm */
static void sched_psjf() {
	// - your own implementation of PSJF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}


/* Preemptive MLFQ scheduling algorithm */
static void sched_mlfq() {
	// - your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
}

static void ring(int signum){
	switch_context = 1;
	printf("RING RING! The timer has gone off\n");
	swapcontext(&main_ctx, &sched_ctx);
}

int worker_init(){
	runqueue = NULL; // create scheduler queue

	// create timer signal handler
	struct sigaction sa;
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &ring; // call ring() whenever SIGPROF received
	sigaction (SIGPROF, &sa, NULL);

	// Create timer struct
	struct itimerval timer;

	// Set up what the timer should reset to after the timer goes off
	timer.it_interval.tv_usec = 0; 
	timer.it_interval.tv_sec = 1;

	// Set up the current timer to go off in 1 second
	// Note: if both of the following values are zero
	//       the timer will not be active, and the timer
	//       will never go off even if you set the interval value
	timer.it_value.tv_usec = 0;
	timer.it_value.tv_sec = 1;

	// Set the timer up (start the timer)
	setitimer(ITIMER_PROF, &timer, NULL);

	if (getcontext(&sched_ctx) < 0){
		perror("get sched_ctx");
		exit(1);
	}
	// Allocate space for stack
	void *stack=malloc(STACK_SIZE);
	
	if (stack == NULL){
		perror("Failed to allocate stack");
		exit(1);
	}
	
	/* Setup context that we are going to use */
	sched_ctx.uc_link=NULL;
	sched_ctx.uc_stack.ss_sp=stack;
	sched_ctx.uc_stack.ss_size=STACK_SIZE;
	sched_ctx.uc_stack.ss_flags=0;
	
	// Setup the context to start running at simplef
	makecontext(&sched_ctx,(void *)&schedule,0);		

	return 0;
}

/* create a new thread */
int worker_create(worker_t * thread, pthread_attr_t * attr, 
                      void *(*function)(void*), void * arg) {

		// - create Thread Control Block (TCB)
		// - create and initialize the context of this worker thread
		// - allocate space of stack for this thread to run
		// after everything is set, push this thread into run queue and 
		// - make it ready for the execution.

		// YOUR CODE HERE
		if (callno == 0) // first time calling worker_create
		{
			if (worker_init() < 0) // TODO: handle errors
			{
				perror("worker init error");
				exit(1);
			}
		}

		ucontext_t* cctx = malloc(sizeof(ucontext_t));
		if (getcontext(cctx) < 0){
			perror("getcontext");
			exit(1);
		}

		// Allocate space for stack
		void *stack=malloc(STACK_SIZE);
		
		if (stack == NULL){
			perror("Failed to allocate stack");
			exit(1);
		}
		
		/* Setup context that we are going to use */
		cctx->uc_link=&main_ctx;
		cctx->uc_stack.ss_sp=stack;
		cctx->uc_stack.ss_size=STACK_SIZE;
		cctx->uc_stack.ss_flags=0;
		
		// puts(" about to call make  context");
		
		makecontext(cctx, (void *)function, 1, (void *) arg);

		//create Thread Context Block
		tcb* block = malloc(sizeof(tcb));
		block->id = thread;
		block->context = cctx;
		block->status = READY;

		// push thread to run queue
		queue_add(&runqueue, block);

		// run schedule context
		callno = callno + 1;
	
		return 0;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield() {
	
	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context

	// YOUR CODE HERE
	ucontext_t current;
	if (getcontext(&current) < 0){
		perror("context yield");
		exit(1);	
	}
	node* n = queue_front(&runqueue);
	n->t_block->context = &current;
	n->t_block->status = READY;
	queue_pop(&runqueue);
	queue_add(&runqueue, n->t_block);
	
	swapcontext(&current, &sched_ctx);
	return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr) {
	// - de-allocate any dynamic memory created when starting this thread

	// YOUR CODE HERE
	// ucontext_t current;
	// if (getcontext(&current) < 0){
	// 	perror("getcontext_exit");
	// 	exit(1);
	// }

	// free(current.uc_stack.ss_sp);
	
	// set tcb to DONE -> ready for join()
	node* n = queue_front(&runqueue);
	n->t_block->status = DONE;
};


/* Wait for thread termination */
int worker_join(worker_t* thread, void **value_ptr) {
	
	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread
	// YOUR CODE HERE  
	node* n = queue_front(&runqueue);
	printf("waiting for thread %p\n", thread);
	while (n->t_block->id != thread || (n->t_block->status != DONE)){
		//DO NOTHING
	}

	//TODO free ucontext, block, stack
	free(n->t_block->context->uc_stack.ss_sp);
	free(n->t_block->context);
	free(n->t_block);

	printf("popping thread %p\n", n->t_block->id);
	queue_pop(&runqueue);

	if (is_empty(&runqueue))
		puts("queue is emptied");


	return 0;
};

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex, 
                          const pthread_mutexattr_t *mutexattr) {
	//- initialize data structures for this mutex

	// YOUR CODE HERE
	return 0;
};

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex) {

        // - use the built-in test-and-set atomic function to test the mutex
        // - if the mutex is acquired successfully, enter the critical section
        // - if acquiring mutex fails, push current thread into block list and
        // context switch to the scheduler thread

        // YOUR CODE HERE
        return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex) {
	// - release mutex and make it available again. 
	// - put threads in block list to run queue 
	// so that they could compete for mutex later.

	// YOUR CODE HERE
	return 0;
};


/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex) {
	// - de-allocate dynamic memory created in worker_mutex_init

	return 0;
};



//DO NOT MODIFY THIS FUNCTION
/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void) {

       fprintf(stderr, "Total context switches %ld \n", tot_cntx_switches);
       fprintf(stderr, "Average turnaround time %lf \n", avg_turn_time);
       fprintf(stderr, "Average response time  %lf \n", avg_resp_time);
}


// Feel free to add any other functions you need

// YOUR CODE HERE

