#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <ucontext.h>

#define STACK_SIZE SIGSTKSZ
ucontext_t fctx, bctx;
static volatile sig_atomic_t switch_context = 0;

void foo(){
    while(1){
        printf("foo\n");
		switch_context = 1;
    }
}

void bar(){
    while(1){
        printf("bar\n");
		switch_context = 0;
    }
}

void scheduler(){
    if (switch_context == 0) {
        swapcontext(&bctx, &fctx);
    } else if (switch_context == 1) {
        swapcontext(&fctx, &bctx);
    }
}


int main() {
    //initialise contexts;
    getcontext(&fctx);
    getcontext(&bctx);

    //allocate space for stack
    void *fstack = malloc(STACK_SIZE);

    if (fstack == NULL){
		perror("Failed to allocate stack");
		exit(1);
	}

    	/* Setup context that we are going to use */
	fctx.uc_link=NULL;
	fctx.uc_stack.ss_sp=fstack;
	fctx.uc_stack.ss_size=STACK_SIZE;
	fctx.uc_stack.ss_flags=0;

    makecontext(&fctx, (void *)&foo, 1, &bctx);


    void *bstack = malloc(STACK_SIZE);

	if (bstack == NULL){
		perror("Failed to allocate stack");
		exit(1);
	}

    	/* Setup context that we are going to use */
	bctx.uc_link=NULL;
	bctx.uc_stack.ss_sp=bstack;
	bctx.uc_stack.ss_size=STACK_SIZE;
	bctx.uc_stack.ss_flags=0;
	

    makecontext(&bctx, (void *)&bar, 0, NULL);

	// Use sigaction to register signal handler
    struct sigaction sa;
	memset (&sa, 0, sizeof (sa));
	sa.sa_handler = &scheduler;
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

	//setcontext(&fctx);

	while(1){

	}

	return 0;
}