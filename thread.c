//apt-get install gcc-multilib

#include "thread.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>


// thread metadata
struct thread {
	void *esp;
	struct thread *next;
	struct thread *prev;
	void *baseEsp;
};

struct thread *ready_list = NULL;     // ready list
struct thread *cur_thread = NULL;     // current thread

struct thread *last_thread = NULL;
static int waitingThreads = 0;
// defined in context.s
void context_switch(struct thread *prev, struct thread *next);

// insert the input thread to the end of the ready list.
static void push_back(struct thread *list, struct thread *t)
{
	t->next = NULL;
	t->prev = NULL;
	if (list==NULL){
		ready_list = t;
		return;
	}
	struct thread *ptr = list;
	while(ptr->next != NULL){
		ptr = ptr->next;
	}
	ptr->next = t;
	t->prev = ptr;
}

// remove the first thread from the ready list and return to caller.
static struct thread *pop_front()
{
	struct thread *ptr = ready_list;
	if (ready_list != NULL){
		ready_list = ready_list->next;
		ptr->next = NULL;
	}

	
	return ptr;
}

// the next thread to schedule is the first thread in the ready list.
// obtain the next thread from the ready list and call context_switch.
static void schedule()
{
	struct thread *prev = cur_thread;
	struct thread *next = pop_front();
	cur_thread = next;
	context_switch(prev, next);
}

// push the cur_thread to the end of the ready list and call schedule
// if cur_thread is null, allocate struct thread for cur_thread
static void schedule1()
{
	if (cur_thread==NULL){
		cur_thread = malloc(sizeof(struct thread));
	}
	push_back(ready_list, cur_thread);
	schedule();
}

// allocate stack and struct thread for new thread
// save the callee-saved registers and parameters on the stack
// set the return address to the target thread
// save the stack pointer in struct thread
// push the current thread to the end of the ready list
void create_thread(func_t func, void *param)
{
	struct thread *t =malloc(sizeof(struct thread));
	unsigned *stack = malloc(4096);
	t->baseEsp = stack;
	stack += 1024;

	*stack = (unsigned)param;
	stack --;
	*stack = 0;
	stack--;
	*stack = (unsigned)func;
	stack--;
	*stack = 0;
	stack--;
	*stack = 0;
	stack--;
	*stack = 0;
	stack--;
	*stack = 0;
	t->esp = stack;
	t->next = NULL;
	t->prev = NULL;
	push_back(ready_list,t);

	
}

// call schedule1
void thread_yield()
{
	schedule1();
}

// call schedule
void thread_exit()
{	
	if (last_thread!=NULL){
		free(last_thread->baseEsp);
		free(last_thread);
	}
	last_thread = NULL;

	last_thread = cur_thread;
	schedule();
}

// call schedule1 until ready_list is null
void wait_for_all()
{
	while(ready_list!=NULL || waitingThreads){
		schedule1();
	}
}

void sleep(struct lock *lock)
{
	void* wait_list = lock->wait_list;
	if (wait_list==NULL){
		lock->wait_list = (struct thread*)cur_thread;
	}
	else{
		while((*((struct thread*)wait_list)).next!= NULL){
			wait_list = (*((struct thread*)wait_list)).next;
			
		}
		(*((struct thread*)wait_list)).next = cur_thread;
	}
	waitingThreads++;

	schedule();
}

void wakeup(struct lock *lock)
{
	if (lock->wait_list != NULL){
		struct thread* thread = lock->wait_list;
		lock->wait_list = (*((struct thread*)(lock->wait_list))).next;
		waitingThreads--;

		push_back(ready_list, thread);
	}
}
