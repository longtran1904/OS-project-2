// File:	thread-worker.c

// List all group member's name:
// username of iLab:
// iLab Server:

#include "thread-worker.h"
#include "run_queue.c"
#include "block_list.c"
#include <signal.h>
#include <time.h>
#include <string.h>
#include <sys/time.h>

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
static node **mlfq[4];
static int S;			 // to reset priorities for mlfq
static int mlfq_counter; // to compare to S ^
static block_node *blocklist = NULL;
static ucontext_t sched_ctx;
static ucontext_t caller_sched; // context of caller that switched to scheduler aka last thread running before ring
static ucontext_t main_ctx;
static volatile sig_atomic_t switch_context = 0;

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

/* Pre-emptive Shortest Job First (POLICY_PSJF) scheduling algorithm */
static void sched_psjf()
{
	// - your own implementation of PSJF
	// (feel free to modify arguments and return types)

	// YOUR CODE HERE
	while (1 == 1)
	{
		printf("Hello World from psjf schedule context!!!\n");
		if (!is_empty(&runqueue))
		{
			move_lowest_quantum_to_front(&runqueue);

			node *n = queue_front(&runqueue); // pop from queue - choose job to work
			// deciphering to get and set context)
			tcb *block = n->t_block;
			
			if (block->status == READY)
			{
				if (block->rant == false)
				{
					block->rant == true;
					struct timespec response_time, diff;
					// Record the start time
					clock_gettime(CLOCK_MONOTONIC, &response_time);
					block->time_response = response_time;
					diff.tv_sec = response_time.tv_sec - block->time_arrival.tv_sec;
                    diff.tv_nsec = response_time.tv_nsec - block->time_arrival.tv_nsec;
					double elapsed_microseconds = (diff.tv_sec * 1000000) + (diff.tv_nsec / 1000);
					total_resp_sum += elapsed_microseconds;

				}
				block->status = RUNNING;
				printf("Executing thread %d\n", *(block->id));
				setcontext(block->context);
			}
			else if (block->status == DONE)
			{
				puts("queue not empty!!\n");
				queue_add(&runqueue, block);
				queue_pop(&runqueue);
				printf("thread %d is done! pushing it back...\n", *(block->id));
			}
			else if (block->status == RUNNING)
			{
				printf("thread is running... continue...\n");
				setcontext(&caller_sched);
			}

			// printf("freeing heap from thread\n");
			// free(block->context->uc_stack.ss_sp);
		}
		else
		{
			puts("queue is empty\n");
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
		if (mlfq_counter == S)
		{ // resets priorities
			queue_moveNodes(mlfq[0], mlfq[1], 0);
			queue_moveNodes(mlfq[0], mlfq[2], 0);
			queue_moveNodes(mlfq[0], mlfq[3], 0);
			mlfq_counter = 0;
		}
		node *nextToRun = NULL;
		for (int i = 0; i < 4 && nextToRun == NULL; i++)
		{
			nextToRun = queue_front(mlfq[i]);
			if (nextToRun != NULL)
			{
				tcb *block = nextToRun->t_block;
				if (block->status != READY)
				{
					queue_add(mlfq[i], block);
					queue_pop(mlfq[i]);
					nextToRun == NULL;
				}
			}
		}
		if (nextToRun == NULL)
		{
			printf("MLFQ has nothing to run...\n");
		}
		else
		{
			tcb *block = nextToRun->t_block;
			if (block->rant == false)
				{
					block->rant == true;
					struct timespec response_time, diff;
					// Record the start time
					clock_gettime(CLOCK_MONOTONIC, &response_time);
					block->time_response = response_time;
					diff.tv_sec = response_time.tv_sec - block->time_arrival.tv_sec;
                    diff.tv_nsec = response_time.tv_nsec - block->time_arrival.tv_nsec;
					double elapsed_microseconds = (diff.tv_sec * 1000000) + (diff.tv_nsec / 1000);
					total_resp_sum += elapsed_microseconds;

				}
			add_front(&runqueue, nextToRun);
			queue_pop(mlfq[nextToRun->t_block->priority]);
			swapcontext(&sched_ctx, nextToRun->t_block->context);
		}
	}
}

static void ring(int signum)
{
	switch_context = 1;
	printf(YELLOW "RING RING! The timer has gone off\n" RESET);

	node *n = queue_front(&runqueue);
	if ((n != NULL) && (n->t_block->status == RUNNING))
	{
#ifndef MLFQ
		ucontext_t *current = malloc(sizeof(ucontext_t));

		node *n = queue_front(&runqueue);
		queue_pop(&runqueue);
		// free old context
		free(n->t_block->context);
		n->t_block->context = current;
		n->t_block->status = READY;
		n->t_block->quantum_counter++;
		queue_add(&runqueue, n->t_block);

		if (getcontext(current) < 0)
		{
			perror("context yield");
			exit(1);
		}
#else
		ucontext_t *current = malloc(sizeof(ucontext_t));

		node *n = queue_front(&runqueue);
		queue_pop(&runqueue);
		free(n->t_block->context);
		n->t_block->context = current;
		n->t_block->status = READY;
		n->t_block->quantum_counter = n->t_block->quantum_counter++;
		if (n->t_block->priority < 3)
		{
			n->t_block->priority = n->t_block->priority + 1;
			n->t_block->priority_quantum_counter = 0;
		}
		queue_add(&mlfq[n->t_block->priority], n->t_block);

		if (getcontext(current) < 0)
		{
			perror("context yield");
			exit(1);
		}
#endif
	}

	swapcontext(&caller_sched, &sched_ctx);
}

int worker_init()
{
	runqueue = NULL; // create scheduler queue

#ifdef MLFQ // sets up mlfq array with 4 queues
	// with highest priority
	S = 5;
	mlfq_counter = 0;
	for (int i = 0; i < 4; i++)
	{
		node *head = NULL;
		mlfq[i] = head; // Initialize to NULL or create the queues/nodes as needed
	}
#endif

	// create timer signal handler
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &ring; // call ring() whenever SIGPROF received
	sigaction(SIGPROF, &sa, NULL);

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

	total_worker_threads++;

// push thread to run queue
#ifndef MLFQ // then PSJF
	queue_add(&runqueue, block);
		// run schedule context
	callno = callno + 1;
#else // MLFQ
	queue_add(&mlfq[0], block);
		// run schedule context
	callno = callno + 1;
#endif


	return 0;
};

/* give CPU possession to other user-level worker threads voluntarily */
int worker_yield()
{

	// - change worker thread's state from Running to Ready
	// - save context of this thread to its thread control block
	// - switch from thread context to scheduler context

	// YOUR CODE HERE
	ucontext_t *current = malloc(sizeof(ucontext_t));
	if (getcontext(current) < 0)
	{
		perror("context yield");
		exit(1);
	}

	node *n = queue_front(&runqueue);
	queue_pop(&runqueue);
	// free old context
	free(n->t_block->context);
	n->t_block->context = current;
	n->t_block->status = READY;
	n->t_block->quantum_counter++;

#ifndef MLFQ
	queue_add(&runqueue, n->t_block);
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
	queue_add(&mlfq[n->t_block->priority], n->t_block);
#endif

	printf(GREEN "thread #%d yielding...\n" RESET, *(n->t_block->id));

	swapcontext(current, &sched_ctx);
	return 0;
};

/* terminate a thread */
void worker_exit(void *value_ptr)
{
	// - de-allocate any dynamic memory created when starting this thread

	// set tcb to DONE -> ready for join()
	node *n = queue_front(&runqueue);
	tcb *block = n->t_block;
	n->t_block->status = DONE;
	struct timespec finish_time, diff;
	clock_gettime(CLOCK_MONOTONIC, &finish_time);
    block->time_finish = finish_time;
    diff.tv_sec = finish_time.tv_sec - block->time_response.tv_sec;
	diff.tv_nsec = finish_time.tv_nsec - block->time_response.tv_nsec;
	double elapsed_microseconds = (diff.tv_sec * 1000000) + (diff.tv_nsec / 1000);
	total_turn_sum += elapsed_microseconds;



#ifdef MLFQ

#endif

	printf(GREEN "thread %d exited!\n" RESET, *(n->t_block->id));
};

/* Wait for thread termination */
int worker_join(worker_t thread, void **value_ptr)
{

	// - wait for a specific thread to terminate
	// - de-allocate any dynamic memory created by the joining thread
	// YOUR CODE HERE
	node *n = queue_front(&runqueue);
	printf("waiting for thread %d\n", thread);
	if (getcontext(&main_ctx) < 0)
	{
		perror("get main_ctx");
		exit(1);
	}
	while (*(n->t_block->id) != thread || (n->t_block->status != DONE))
	{
		// DO NOTHING
		n = queue_front(&runqueue);
	}

	// TODO free ucontext, block, stack
	printf(GREEN "popping thread %d\n" RESET, *(n->t_block->id));
	free(n->t_block->context->uc_stack.ss_sp);
	free(n->t_block->context);
	free(n->t_block);

	queue_pop(&runqueue);

	if (is_empty(&runqueue))
		puts("runqueue is emptied");

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
	node *n = queue_front(&runqueue);
	tcb *block = n->t_block;
	if (mutex->status == UNLOCKED)
	{
		mutex->status = LOCKED;
		// record which thread is holding lock
		mutex->thread_block = block;
	}
	else
	{
		queue_pop(&runqueue); // remove thread from runqueue

		block_node *new_block_node = malloc(sizeof(block_node));
		new_block_node->mutex = mutex;
		new_block_node->current_thread = n;
		new_block_node->current_thread->t_block->status = BLOCKED;
		new_block_node->next = NULL;
		list_add(&blocklist, new_block_node); // record whole thread node on queue
		setcontext(&sched_ctx);
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
		mutex->status = UNLOCKED;
		block_node *pop;
		while (!list_empty(&blocklist) && ((pop = list_find(&blocklist, mutex)) != NULL))
		{
			// release blocked threads to runqueue again
			pop->current_thread->t_block->status = READY;
			queue_add(&runqueue, pop->current_thread->t_block);
			free(pop);
		}
	}

	// YOUR CODE HERE
	return 0;
};

/* destroy the mutex */
int worker_mutex_destroy(worker_mutex_t *mutex)
{
	// - de-allocate dynamic memory created in worker_mutex_init

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
