
#include "../include/queue.h"
#include "sched.h"
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>
static struct queue_t ready_queue;
static struct queue_t run_queue;
static pthread_mutex_t queue_lock;

static struct queue_t running_list;
#ifdef MLQ_SCHED
static struct queue_t mlq_ready_queue[MAX_PRIO];
static int slot[MAX_PRIO];
#endif

#ifdef CFS_SCHED
#include "../include/RBTree.h"

static RBNode *cfs_ready_tree; 

uint32_t total_weight = 0;
void accumulate_weight(RBNode *node) {
    if (node != NULL) {
        uint32_t weight = node->data->proc->weight;
        total_weight += weight;
    }
}

/* Calculate total weight of all processes in the tree */
uint32_t calculate_total_weight() {
    total_weight = 0; 
    if (cfs_ready_tree != NULL) {
        traverse(cfs_ready_tree, accumulate_weight, PREORDER);
    }
    return total_weight;
}

/* Calculate process weight based on its priority */
uint32_t calculate_process_weight(struct pcb_t *proc) {
    int niceness = proc->priority;
    uint32_t weight = 1024 * (1 << (-niceness / 10));
    return weight;
}

/* Calculate time slice based on process weight and system load */
uint32_t calculate_time_slice(struct pcb_t *proc) {
    const uint32_t target_latency = 20;    
    uint32_t weight = proc->weight;
    
    uint32_t total_weight = calculate_total_weight();
    if (total_weight == 0) total_weight = weight; 
    
    uint32_t time_slice = (weight * target_latency) / total_weight;
    if (time_slice == 0) time_slice = 1;
    
    return time_slice;
}

void re_calculate_time_slice(RBNode *node) {
	if (node != NULL) {
		struct pcb_t *proc = node->data->proc;
		proc->time_slice = calculate_time_slice(proc);
	}
}

/* Update vruntime of the process */
void update_vruntime(struct pcb_t *proc, uint32_t exec_time) {
	uint32_t weight = proc->weight;
	proc->vruntime += exec_time / weight;
}

struct pcb_t *get_cfs_proc(void) {
    pthread_mutex_lock(&queue_lock);
    
    if (cfs_ready_tree == NULL) {
        pthread_mutex_unlock(&queue_lock);
        return NULL; 
    }

    RBNode *minNode = getMinNode(cfs_ready_tree);
    struct pcb_t *proc = minNode->data->proc;
    deleteNode(&cfs_ready_tree, minNode->data);
    
    enqueue(&running_list, proc);
    
    pthread_mutex_unlock(&queue_lock);
    return proc;
}

void put_cfs_proc(struct pcb_t *proc) {
    pthread_mutex_lock(&queue_lock);
    
    proc->time_slice = calculate_time_slice(proc);
    
    proc->ready_queue = &ready_queue;
    proc->running_list = &running_list;
    
    Dtype *data = createDtype(proc);
    insertNode(&cfs_ready_tree, data);
    
    pthread_mutex_unlock(&queue_lock);
}

/* Initial insertion */
void add_cfs_proc(struct pcb_t *proc) {
    pthread_mutex_lock(&queue_lock);
    
    if (getMinNode(cfs_ready_tree) == NULL) {
		proc->vruntime = 0;
	} else {
		proc->vruntime = getMinNode(cfs_ready_tree)->data->proc->vruntime;
	}
	proc->weight = calculate_process_weight(proc);
    proc->time_slice = calculate_time_slice(proc);

	// Recalculate time slice for all processes in the tree
	traverse(cfs_ready_tree, re_calculate_time_slice, PREORDER);
    
    proc->ready_queue = &ready_queue;
    proc->running_list = &running_list;
    
    Dtype *data = createDtype(proc);
    insertNode(&cfs_ready_tree, data);
    
    pthread_mutex_unlock(&queue_lock);
}

struct pcb_t * get_proc(void) {
    return get_cfs_proc();
}

void put_proc(struct pcb_t * proc) {
    put_cfs_proc(proc);
}

void add_proc(struct pcb_t * proc) {
    add_cfs_proc(proc);
}

void init_scheduler(void) {
    ready_queue.size = 0;
    running_list.size = 0;
    pthread_mutex_init(&queue_lock, NULL);
    cfs_ready_tree = NULL; // Initialize the RB-tree to NULL
}
#endif

int queue_empty(void) {
#ifdef MLQ_SCHED
	unsigned long prio;
	for (prio = 0; prio < MAX_PRIO; prio++)
		if(!empty(&mlq_ready_queue[prio])) 
			return -1;
#endif
	return (empty(&ready_queue) && empty(&run_queue));
}

#ifndef CFS_SCHED
void init_scheduler(void) {
#ifdef MLQ_SCHED
    int i ;

	for (i = 0; i < MAX_PRIO; i ++) {
		mlq_ready_queue[i].size = 0;
		slot[i] = MAX_PRIO - i; 
	}
#endif
	ready_queue.size = 0;
	run_queue.size = 0;
	pthread_mutex_init(&queue_lock, NULL);
}
#endif

#ifdef MLQ_SCHED
/* 
 *  Stateful design for routine calling
 *  based on the priority and our MLQ policy
 *  We implement stateful here using transition technique
 *  State representation   prio = 0 .. MAX_PRIO, curr_slot = 0..(MAX_PRIO - prio)
 */
struct pcb_t * get_mlq_proc(void) {
	struct pcb_t * proc = NULL;
	/*TODO: get a process from PRIORITY [ready_queue].
	 * Remember to use lock to protect the queue.
	 * */
	return proc;	
}

void put_mlq_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_mlq_proc(struct pcb_t * proc) {
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);	
}

struct pcb_t * get_proc(void) {
	return get_mlq_proc();
}

void put_proc(struct pcb_t * proc) {
	proc->ready_queue = &ready_queue;
	proc->mlq_ready_queue = mlq_ready_queue;
	proc->running_list = & running_list;

	/* TODO: put running proc to running_list */


	return put_mlq_proc(proc);
}

void add_proc(struct pcb_t * proc) {
	proc->ready_queue = &ready_queue;
	proc->mlq_ready_queue = mlq_ready_queue;
	proc->running_list = & running_list;

	/* TODO: put running proc to running_list */

	return add_mlq_proc(proc);
}
#else
#ifndef CFS_SCHED
struct pcb_t * get_proc(void) {
	struct pcb_t * proc = NULL;
	/*TODO: get a process from [ready_queue].
	 * Remember to use lock to protect the queue.
	 * */
	return proc;
}

void put_proc(struct pcb_t * proc) {
	proc->ready_queue = &ready_queue;
	proc->running_list = & running_list;

	/* TODO: put running proc to running_list */

	pthread_mutex_lock(&queue_lock);
	enqueue(&run_queue, proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_proc(struct pcb_t * proc) {
	proc->ready_queue = &ready_queue;
	proc->running_list = & running_list;

	/* TODO: put running proc to running_list */

	pthread_mutex_lock(&queue_lock);
	enqueue(&ready_queue, proc);
	pthread_mutex_unlock(&queue_lock);	
}
#endif
#endif


