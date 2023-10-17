#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <ucontext.h>
#include <time.h>


#define STACK_SIZE SIGSTKSZ

ucontext_t nctx;
ucontext_t cctx,bctx;

static volatile sig_atomic_t switch_context = 0;

static void ring(int signum){
	switch_context = 1;
	printf("RING RING! The timer has gone off\n switch_context: %d\n", switch_context);
}

static void foo(){
	struct timespec delay = { 3, 0 };
	while (1 == 1){
		if (switch_context) {
			switch_context = 0;
			swapcontext(&cctx, &bctx);
		}
		puts("foo");
		nanosleep(&delay, 0);

	}
	
}

static void bar() {
	struct timespec delay = { 1, 0 };
	while (1 == 1){
		if (switch_context) { 
			switch_context = 0;
			swapcontext(&bctx, &cctx);
		}
		puts("bar");	
		// nanosleep(&delay, 0);
	}
}

int main(){
	if (getcontext(&cctx) < 0){
		perror("getcontext");
		exit(1);
	}

	// // Allocate space for stack	
	// void *stack=malloc(STACK_SIZE);
	
	// if (stack == NULL){
	// 	perror("Failed to allocate stack");
	// 	exit(1);
	// }

	// cctx.uc_link=NULL;
	// cctx.uc_stack.ss_sp=stack;
	// cctx.uc_stack.ss_size=STACK_SIZE;
	// cctx.uc_stack.ss_flags=0;

	// makecontext(&cctx,(void *)&foo,1,&nctx);

	if (getcontext(&bctx) < 0){
		perror("getcontext");
		exit(1);
	}

	// Allocate space for stack	
	void *stack2=malloc(STACK_SIZE);
	
	if (stack2 == NULL){
		perror("Failed to allocate stack");
		exit(1);
	}

	bctx.uc_link=NULL;
	bctx.uc_stack.ss_sp=stack2;
	bctx.uc_stack.ss_size=STACK_SIZE;
	bctx.uc_stack.ss_flags=0;

	makecontext(&bctx,(void *)&bar,1,&nctx);

	// Use sigaction to register signal handler
	struct sigaction sa;
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &ring;
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

	foo();

	// free(cctx.uc_stack.ss_sp);
	// free(bctx.uc_stack.ss_sp);
	
	// Kill some time
	// while(1);

}
