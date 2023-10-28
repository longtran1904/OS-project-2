// File:	thread-worker.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "thread-worker.h"
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>
#include "run_queue.c"
#include "block_list.c"

#define STACK_SIZE SIGSTKSZ

#define YELLOW "\033[0;33m"
#define GREEN "\e[0;32m"
#define RESET "\033[0m"
#define QUANTUM 10

static void sched_psjf();
static void sched_mlfq();

// Global counter for total context switches and
// average turn around and response time
long tot_cntx_switches = 0;
double avg_turn_time = 0;
double avg_resp_time = 0;

// INITAILIZE ALL YOUR OTHER VARIABLES HERE
// YOUR CODE HERE
static int callno = 0;
static node *runqueue = NULL;
static node *mlfq[4];
static int S = 50;			 // to reset priorities for mlfq
static int mlfq_counter; // to compare to S ^
static int mlfq_done;
static block_node *blocklist = NULL;
static ucontext_t sched_ctx;
static ucontext_t caller_sched; // context of caller that switched to scheduler aka last thread running before ring
static ucontext_t main_ctx;
static volatile sig_atomic_t switch_context = 0;

struct sigaction sa;
struct itimerval timer;
struct itimerval save_timer;
struct itimerval pause_timer;


// to calculate global statistics
static double total_turn_sum;
static double total_resp_sum;
static int total_worker_threads;

/* scheduler */
static void schedule()
{
// - schedule policy
#ifndef MLFQ
	// Choose PSJF
	sched_psjf();
#else
	// Choose MLFQ
	sched_mlfq();
#endif
}

void pauseTimer() {
    // Set both timer values to zero to pause the timer
    setitimer(ITIMER_PROF, &pause_timer, NULL);
}

void resumeTimer() {

    // Set both timer values to zero to pause the timer

    timer.it_interval.tv_usec = QUANTUM;
    timer.it_interval.tv_sec = 0;
    timer.it_value.tv_usec = 5;
    timer.it_value.tv_sec = 0;
	// timer = save_timer;
	// timer = save_timer;
    setitimer(ITIMER_PROF, &timer, NULL);
}

/* Pre-emptive Shortest Job First (POLICY_PSJF) scheduling algorithm */
static void sched_psjf()
{
	// - your own implementation of PSJF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
	while (1 == 1)
	{
		pauseTimer();
		// printf("Hello World from psjf schedule context!!!\n");
		if (!is_empty(&runqueue))
		{
			move_lowest_quantum_to_front(&runqueue);

			node *n = queue_front_node(&runqueue); // pop from queue - choose job to work
			// printf("Lowest quantum_counter node is %d with %d-quanta\n", n->t_block->num_thread, n->t_block->quantum_counter);
			// deciphering to get and set context)
			tcb *block = n->t_block;
			
			if (block->status == READY)
			{
				if (block->rant == false)
				{
					block->rant = true;
					struct timespec response_time, diff;
					// Record the start time
					clock_gettime(CLOCK_MONOTONIC, &response_time);
					block->time_response = response_time;
					diff.tv_sec = response_time.tv_sec - block->time_arrival.tv_sec;
                    diff.tv_nsec = response_time.tv_nsec - block->time_arrival.tv_nsec;
					double elapsed_microseconds = (diff.tv_sec * 1000) + (diff.tv_nsec / 1000000);
					total_resp_sum += elapsed_microseconds;
					avg_resp_time = total_resp_sum/total_worker_threads;

				}
				block->status = RUNNING;
				// printf("Executing thread %d\n", *(block->id));
				resumeTimer();
				setcontext(block->context);
			}
			else if (block->status == DONE)
			{
				// puts("queue not empty!!\n");
				printf("thread %d is done! pushing it back...\n", *(block->id));
				n->t_block->quantum_counter++;
				queue_pop_node(&runqueue);
				queue_add_node(&runqueue, n);
				// show_queue(&runqueue);
				resumeTimer();
				tot_cntx_switches++;
				setcontext(&main_ctx);
			}
			else if (block->status == RUNNING)
			{
				printf("thread %d is running... continue...\n", *(block->id));
				resumeTimer();
				tot_cntx_switches++;
				setcontext(&caller_sched);
			}
			// printf("freeing heap from thread\n");
			// free(block->context->uc_stack.ss_sp);
		}
		else
		{
			puts("queue is empty\n");
			// resumeTimer();
			tot_cntx_switches++;
			setcontext(&main_ctx);
		}
	}
}

/* Preemptive MLFQ scheduling algorithm */
// Rule 1: If Priority(A) > Priority(B), A runs (B doesn’t).
// Rule 2: If Priority(A) = Priority(B), A & B run in RR
// Rule 3: When a job enters the system, it is placed at the highest
// priority (the topmost queue).
// • Rule 4a: If a job uses up its allotment while running, its priority is
// reduced (i.e., it moves down one queue).
// • Rule 4b: If a job gives up the CPU (for example, by performing
// an I/O operation) before the allotment is up, it stays at the same
// priority level (i.e., its allotment is reset)
// Rule 5: After some time period S, move all the jobs in the system
// to the topmost queue

// MODIFIED Rule 4: Once a job uses up its time allotment at a given level (regardless of how many times it has given up the CPU), its priority is
// reduced (i.e., it moves down one queue).
static void sched_mlfq()
{
	// - your own implementation of MLFQ
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
	while (1)
	{
		pauseTimer();
		if (mlfq_counter == S)
		{ // resets priorities
			queue_moveNodes(&mlfq[0], &mlfq[1], 0);
			queue_moveNodes(&mlfq[0], &mlfq[2], 0);
			queue_moveNodes(&mlfq[0], &mlfq[3], 0);
			mlfq_counter = 0;
		}
		node *nextToRun = NULL;
		// printf("   RUNQUEUE STATUS   \n");
		// show_queue(&runqueue); printf("\n");
		printf("   MLFQ STATUS    \n");
		printf(" queue[0] "); show_queue(&mlfq[0]); printf("\n");
		printf(" queue[1] "); show_queue(&mlfq[1]); printf("\n");
		printf(" queue[2] "); show_queue(&mlfq[2]); printf("\n");
		printf(" queue[3] "); show_queue(&mlfq[3]); printf("\n");
		for (int i = 0; i < 4 && nextToRun == NULL; i++)
		{
			nextToRun = queue_front_node(&mlfq[i]);
			if (nextToRun != NULL)
			{
				tcb *block = nextToRun->t_block;
				if (block->status != READY)
				{
					queue_add_node(&mlfq[i], nextToRun);
					queue_pop_node(&mlfq[i]);
					nextToRun == NULL;
				}
			}
		}
		if (nextToRun == NULL)
		{
			// mlfq_done = 1; // nothing to run -> mlfq is done
			printf("MLFQ has nothing to run...\n");
			// rotate runqueue for worker_join()
			node* n = queue_front_node(&runqueue);
			if (n != NULL) {
				queue_add_node(&runqueue, n);
				queue_pop_node(&runqueue);
				printf("pushing thread %d to back...\n", n->t_block->num_thread);
			}
			// printf(" RUNQUEUE \n");
			// show_queue(&runqueue);
			resumeTimer();
			setcontext(&main_ctx);
		}
		else
		{
			tcb *block = nextToRun->t_block;
			if (block->rant == false)
				{
					block->rant = true;
					struct timespec response_time, diff;
					// Record the start time
					clock_gettime(CLOCK_MONOTONIC, &response_time);
					block->time_response = response_time;
					diff.tv_sec = response_time.tv_sec - block->time_arrival.tv_sec;
                    diff.tv_nsec = response_time.tv_nsec - block->time_arrival.tv_nsec;
					double elapsed_microseconds = (diff.tv_sec * 1000) + (diff.tv_nsec / 1000000);
					total_resp_sum += elapsed_microseconds;
					avg_resp_time = total_resp_sum/total_worker_threads;

				}
		    // timer.it_interval.tv_usec = QUANTUM*(nextToRun->t_block->priority+1);
			// setitimer(ITIMER_PROF, &timer, NULL);
			nextToRun->t_block->status = RUNNING;
			queue_pop_node(&mlfq[nextToRun->t_block->priority]);
			add_front(&runqueue, nextToRun);
			// show_queue(&mlfq[nextToRun->t_block->priority]);
			// increase mlfq_counter to reset priority
			mlfq_counter++;
			resumeTimer();
			setcontext(nextToRun->t_block->context);
		}
	}
}

static void ring(int signum)
{
	pauseTimer();
	switch_context = 1;
	tot_cntx_switches++;
	printf(YELLOW "RING RING! The timer has gone off\n" RESET);
	node *n = queue_front_node(&runqueue);
	// printf("   RUNQUEUE STATUS   \n");
	// show_queue(&runqueue); printf("\n");
	// printf("   MLFQ STATUS    \n");
	// printf(" queue[0] "); show_queue(&mlfq[0]); printf("\n");
	// printf(" queue[1] "); show_queue(&mlfq[1]); printf("\n");
	// printf(" queue[2] "); show_queue(&mlfq[2]); printf("\n");
	// printf(" queue[3] "); show_queue(&mlfq[3]); printf("\n");

	if ((n != NULL) && (n->t_block->status == RUNNING))
	{
#ifndef MLFQ
		
		queue_pop_node(&runqueue);

		// update new context
		n->t_block->status = READY;
		n->t_block->quantum_counter++;

		queue_add_node(&runqueue, n);
		
		resumeTimer();
		tot_cntx_switches++;
		swapcontext(n->t_block->context, &sched_ctx);
#else
	
		queue_pop_node(&runqueue);

		n->t_block->status = READY;
		n->t_block->quantum_counter = n->t_block->quantum_counter++;
		if (n->t_block->priority < 3)
		{
			n->t_block->priority = n->t_block->priority + 1;
			n->t_block->priority_quantum_counter = 0;
		}
		queue_add_node(&mlfq[n->t_block->priority], n);
		tot_cntx_switches++;
		resumeTimer();

		swapcontext(n->t_block->context, &sched_ctx);
#endif
	}
	else {
		resumeTimer();
		tot_cntx_switches++;
		swapcontext(&main_ctx, &sched_ctx);
	}
	    
}

int worker_init()
{
	runqueue = NULL; // create scheduler queue
	total_worker_threads = 0;

#ifdef MLFQ // sets up mlfq array with 4 queues
	mlfq_counter = 0;
	for (int i = 0; i < 4; i++)
	{
		node *head = NULL;
		mlfq[i] = head; // Initialize to NULL or create the queues/nodes as needed
	}
#endif

	// create timer signal handler
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &ring; // call ring() whenever SIGPROF received
	sigaction(SIGPROF, &sa, NULL);

	// initialize pause_time
	pause_timer.it_interval.tv_usec = 0;
	pause_timer.it_interval.tv_sec = 0;
	pause_timer.it_value.tv_usec = 0;
	pause_timer.it_value.tv_sec = 0;

	// Set up what the timer should reset to after the timer goes off
	timer.it_interval.tv_usec = QUANTUM;
	timer.it_interval.tv_sec = 0;

	// Set up the current timer to go off in 1 second
	// Note: if both of the following values are zero
	//       the timer will not be active, and the timer
	//       will never go off even if you set the interval value
	timer.it_value.tv_usec = 10;
	timer.it_value.tv_sec = 0;

	// Set the timer up (start the timer)
	setitimer(ITIMER_PROF, &timer, NULL);

	if (getcontext(&sched_ctx) < 0)
	{
		perror("get sched_ctx");
		exit(1);
	}
	// Allocate space for stack
	void *stack = malloc(STACK_SIZE);

	if (stack == NULL)
	{
		perror("Failed to allocate stack");
		exit(1);
	}

	/* Setup context that we are going to use */
	sched_ctx.uc_link = NULL;
	sched_ctx.uc_stack.ss_sp = stack;
	sched_ctx.uc_stack.ss_size = STACK_SIZE;
	sched_ctx.uc_stack.ss_flags = 0;

	// Setup the context to start running at simplef
	makecontext(&sched_ctx, (void *)&schedule, 0);

	return 0;
}

/* create a new thread */
int worker_create(worker_t *thread, pthread_attr_t *attr,
				  void *(*function)(void *), void *arg)
{

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

	ucontext_t *cctx = malloc(sizeof(ucontext_t));
	if (getcontext(cctx) < 0)
	{
		perror("getcontext");
		exit(1);
	}

	// Allocate space for stack
	void *stack = malloc(STACK_SIZE);

	if (stack == NULL)
	{
		perror("Failed to allocate stack");
		exit(1);
	}

	/* Setup context that we are going to use */
	cctx->uc_link = &main_ctx; // after thread completes, link to main
	cctx->uc_stack.ss_sp = stack;
	cctx->uc_stack.ss_size = STACK_SIZE;
	cctx->uc_stack.ss_flags = 0;

	// puts(" about to call make  context");

	makecontext(cctx, (void *)function, 1, (void *)arg);

	// create Thread Context Block
	tcb *block = malloc(sizeof(tcb));
	// create id for thread
	*thread = callno;
	block->id = thread;
	block->context = cctx;
	block->status = READY;

	struct timespec arrival_time;
	// Record the start time
	clock_gettime(CLOCK_MONOTONIC, &arrival_time);
	block->time_arrival = arrival_time;
	block->quantum_counter = 0;
	block->rant = false;
	block->priority = 0;
	block->num_thread = total_worker_threads;
	total_worker_threads++;

	resumeTimer(); // Start timer

// push thread to run queue
#ifndef MLFQ // then PSJF
	queue_add(&runqueue, block);
		// run schedule context
	callno = callno + 1;
#else // MLFQ
	queue_add(&mlfq[0], block);
	mlfq_done = 0;
		// run schedule context
	callno = callno + 1;
#endif


	return 0;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield()
{
	pauseTimer();
    tot_cntx_switches++;
	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context

	// YOUR CODE HERE

	node *n = queue_front_node(&runqueue);
	queue_pop_node(&runqueue);

	n->t_block->status = READY;
	n->t_block->quantum_counter++;

#ifndef MLFQ
	queue_add_node(&runqueue, n);
#else
	n->t_block->priority_quantum_counter++;
	if (n->t_block->priority_quantum_counter > 5)
	{
		if (n->t_block->priority < 3)
		{
			n->t_block->priority++;
			n->t_block->priority_quantum_counter = 0;
		}
	}
	queue_add_node(&mlfq[n->t_block->priority], n);
#endif

	printf(GREEN "thread #%d yielding...\n" RESET, *(n->t_block->id));

    resumeTimer();
	swapcontext(n->t_block->context, &sched_ctx);
	return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr)
{
	// - de-allocate any dynamic memory created when starting this thread

	// set tcb to DONE -> ready for join()
	node *n = queue_front_node(&runqueue);
	tcb *block = n->t_block;
	block->status = DONE;
	struct timespec finish_time, diff;
	clock_gettime(CLOCK_MONOTONIC, &finish_time);
    block->time_finish = finish_time;
	// pauseTimer();

	printf(" finished from worker_exit thread %d\n", *(n->t_block->id));
	// queue_add_node(&runqueue, n);
	// queue_pop_node(&runqueue);
	
	// resumeTimer();
    diff.tv_sec = finish_time.tv_sec - block->time_arrival.tv_sec;
	diff.tv_nsec = finish_time.tv_nsec - block->time_arrival.tv_nsec;
	double elapsed_microseconds = (diff.tv_sec * 1000) + (diff.tv_nsec / 1000000);
	total_turn_sum += elapsed_microseconds;
	avg_turn_time = total_turn_sum/total_worker_threads;



#ifdef MLFQ

#endif

	// printf(GREEN "thread %d exited!\n" RESET, *(n->t_block->id));
};

/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr)
{

	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread
	// YOUR CODE HERE
	node *n = queue_front_node(&runqueue);
	printf("waiting for thread %d\n", thread);
	if (getcontext(&main_ctx) < 0)
	{
		perror("get main_ctx");
		exit(1);
	}
	while ((n == NULL) || *(n->t_block->id) != thread || (n->t_block->status != DONE))
	{
		// DO NOTHING
		// #ifdef MLFQ
		// 	// worker_join needs to handle runqueue rotation by it own
		// 	// because sched_mlfq doesn't control runqueue rotation
		// 	if (n!=NULL && n->t_block->status == DONE){
		// 		printf("thread #%d is done, pushing it back...\n", n->t_block->num_thread);
		// 		pauseTimer();
		// 		queue_pop_node(&runqueue);
		// 		queue_add_node(&runqueue, n);
		// 		resumeTimer();
		// 		show_queue(&runqueue); printf("\n\n\n");
		// 	}
		// #endif
		n = queue_front_node(&runqueue);
		printf("waiting for thread %d \n", thread);
		if (n!=NULL && *(n->t_block->id) != thread){
			printf("got thread %d\n", n->t_block->num_thread);
			swapcontext(&main_ctx, &sched_ctx);
		}
		// printf("returned to worker_join()!!! spin lock!!!");
	}

	// TODO free ucontext, block, stack
	pauseTimer();
	printf(GREEN "popping thread %d\n" RESET, *(n->t_block->id));

	queue_pop_node(&runqueue);

	free(n->t_block->context->uc_stack.ss_sp);
	free(n->t_block->context);
	free(n->t_block);
	free(n);

	printf(GREEN "pop finished!\n" RESET);
	// show_queue(&runqueue);

	// queue_pop(&runqueue);
	resumeTimer();

#ifndef MLFQ
	if (is_empty(&runqueue)){
		puts("runqueue is emptied");
		pauseTimer();
	}
#else 
	mlfq_done = 0;
	node* nextToRun = NULL;
	for (int i = 0; i < 4 && nextToRun == NULL; i++)
			nextToRun = queue_front_node(&mlfq[i]);

	if (nextToRun == NULL && is_empty(&runqueue)){
		puts("MLFQ is emptied | Nothing left to run");
		pauseTimer();
	}
#endif
		

	return 0;
};

/* initialize the mutex lock */
int worker_mutex_init(worker_mutex_t *mutex,
					  const pthread_mutexattr_t *mutexattr)
{
	//- initialize data structures for this mutex

	// YOUR CODE HERE
	mutex = malloc(sizeof(worker_mutex_t));
	mutex->status = UNLOCKED;

	return 0;
};

/* aquire the mutex lock */
int worker_mutex_lock(worker_mutex_t *mutex)
{

	// - use the built-in test-and-set atomic function to test the mutex
	// - if the mutex is acquired successfully, enter the critical section
	// - if acquiring mutex fails, push current thread into block list and
	// context switch to the scheduler thread

	// YOUR CODE HERE
	
	node *n = queue_front_node(&runqueue);
	tcb *block = n->t_block;
	if (mutex->status == UNLOCKED || *(mutex->thread_block->id) == *(block->id))
	{
		mutex->status = LOCKED;
		// record which thread is holding lock
		mutex->thread_block = block;
	}
	else
	{

		// printf("LOCK OBTAINED BY THREAD %d!!!\n", *(mutex->thread_block->id));
		pauseTimer();
		block_node *new_block_node = malloc(sizeof(block_node));
		queue_pop_node(&runqueue); // remove thread from runqueue
		new_block_node->mutex = mutex;
		new_block_node->current_thread = n;
		new_block_node->current_thread->next = NULL;
		new_block_node->current_thread->t_block->status = BLOCKED;
		new_block_node->next = NULL;
		list_add(&blocklist, new_block_node); // record whole thread node on queue
		resumeTimer();
		tot_cntx_switches++;
		swapcontext(n->t_block->context, &sched_ctx);
	}

	return 0;
};

/* release the mutex lock */
int worker_mutex_unlock(worker_mutex_t *mutex)
{
	// - release mutex and make it available again.
	// - put threads in block list to run queue
	// so that they could compete for mutex later.
	
	if (mutex->status == LOCKED)
	{
		block_node *pop;
		pauseTimer();
		while (!list_empty(&blocklist) && ((pop = list_find(&blocklist, mutex)) != NULL))
		{
			// release blocked threads to runqueue again
			pop->current_thread->t_block->status = READY;
			#ifndef MLFQ
				queue_add_node(&runqueue, pop->current_thread);
			#else 
				queue_add_node(&mlfq[pop->current_thread->t_block->priority], pop->current_thread);
			#endif
			free(pop);
		}
		resumeTimer();
		mutex->status = UNLOCKED;
	}
	// YOUR CODE HERE
	return 0;
};

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex)
{
	// - de-allocate dynamic memory created in worker_mutex_init
	// free(mutex);
	return 0;
};

// DO NOT MODIFY THIS FUNCTION
/* Function to print global statistics. Do not modify this function.*/
void print_app_stats(void)
{

	fprintf(stderr, "Total context switches %ld \n", tot_cntx_switches);
	fprintf(stderr, "Average turnaround time %lf \n", avg_turn_time);
	fprintf(stderr, "Average response time  %lf \n", avg_resp_time);
}

// Feel free to add any other functions you need

// YOUR CODE HERE
