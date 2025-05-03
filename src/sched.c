#include "../include/queue.h"
#include "sched.h"
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
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
#define VRUNTIME_SCALE 1000 // for better visualization of vruntime
// Use time_slot from os.c as TARGET_LATENCY
extern int time_slot; // This will be the TARGET_LATENCY for CFS

static RBNode *cfs_ready_tree; 
static int timestamp;

double total_weight = 0;
void accumulate_weight(RBNode *node) {
    if (node != NULL) {
        double weight = node->data->proc->weight;
        total_weight += weight;
    }
}

double calculate_total_weight() {
    total_weight = 0; 
    if (cfs_ready_tree != NULL) {
        traverse(cfs_ready_tree, accumulate_weight, PREORDER);
    }
    return total_weight;
}

double calculate_process_weight(struct pcb_t *proc) {
    int niceness = proc->niceness;
    double weight = 1024 * pow(2, (-niceness / 10.0));
    return weight;
}

uint32_t calculate_time_slice(struct pcb_t *proc) {
	const int TARGET_LATENCY = time_slot;
    double weight = proc->weight;
    
    double total_weight = calculate_total_weight();
    if (total_weight == 0) total_weight = weight; 
    
    uint32_t time_slice = round((weight * TARGET_LATENCY) / total_weight);
    if (time_slice == 0) time_slice = 1;
    
    return time_slice;
}

void re_calculate_time_slice(RBNode *node) {
	if (node != NULL) {
		struct pcb_t *proc = node->data->proc;
		proc->time_slice = calculate_time_slice(proc);
	}
}

void update_vruntime(struct pcb_t *proc, uint32_t exec_time) {
	double weight = proc->weight;
    
    uint64_t scaled_exec_time = (uint64_t)exec_time * VRUNTIME_SCALE;
    
    double vruntime_delta = (double)scaled_exec_time / weight;
    
    if (vruntime_delta == 0) {
        vruntime_delta = 1;
    }
    
    proc->vruntime += vruntime_delta;
}

uint32_t get_min_vruntime(void) {
	if (cfs_ready_tree == NULL) {
		return 0; 
	}
	
	RBNode *minNode = getMinNode(cfs_ready_tree);
	return minNode->data->key;
}

struct pcb_t *get_cfs_proc(void) {
    pthread_mutex_lock(&queue_lock);
    
    if (cfs_ready_tree == NULL) {
        pthread_mutex_unlock(&queue_lock);
        return NULL; 
    }

    RBNode *minNode = getMinNode(cfs_ready_tree);
	re_calculate_time_slice(minNode);
    struct pcb_t *proc = minNode->data->proc;
    deleteNode(&cfs_ready_tree, minNode->data);
    
    enqueue(&running_list, proc);
    
    pthread_mutex_unlock(&queue_lock);
    return proc;
}

void put_cfs_proc(struct pcb_t *proc) {
    pthread_mutex_lock(&queue_lock);
    
    // proc->time_slice = calculate_time_slice(proc);
    
    proc->ready_queue = &ready_queue;
    proc->running_list = &running_list;
    
    Dtype *data = createDtype(proc, timestamp++);
    insertNode(&cfs_ready_tree, data);

	// traverse(cfs_ready_tree, re_calculate_time_slice, PREORDER);
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

	// traverse(cfs_ready_tree, re_calculate_time_slice, PREORDER);
    
    proc->ready_queue = &ready_queue;
    proc->running_list = &running_list;
    
    Dtype *data = createDtype(proc, timestamp++);
    insertNode(&cfs_ready_tree, data);
    
    pthread_mutex_unlock(&queue_lock);
}

struct pcb_t * get_proc(void) {
    return get_cfs_proc();
}

void put_proc(struct pcb_t * proc) {
	proc->ready_queue = &ready_queue;
	proc->cfs_ready_tree = cfs_ready_tree;
	proc->running_list = &running_list;

	pthread_mutex_lock(&queue_lock);
	enqueue(&running_list, proc);
	pthread_mutex_unlock(&queue_lock);

    put_cfs_proc(proc);
}

void add_proc(struct pcb_t * proc) {
	proc->ready_queue = &ready_queue;
	proc->cfs_ready_tree = cfs_ready_tree;
	proc->running_list = &running_list;

	pthread_mutex_lock(&queue_lock);
	enqueue(&running_list, proc);
	pthread_mutex_unlock(&queue_lock);
	
    add_cfs_proc(proc);
}

void init_scheduler(void) {
    ready_queue.size = 0;
    running_list.size = 0;
    pthread_mutex_init(&queue_lock, NULL);
    cfs_ready_tree = NULL; 
	timestamp = 0;
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
void remove_from_queue(struct queue_t *q, struct pcb_t *proc){
	if (q == NULL || q->size == 0)
			return;
	int idx = -1;
	for(int i = 0;i<q->size;i++){
			if(q->proc[i]->pid == proc->pid){
					idx = i;
					break;
			}
	}
	if(idx == -1) return;
	for (int i = idx; i < q->size - 1; i++)
	{
			q->proc[i] = q->proc[i + 1];
	}
	q->size--;
}

struct pcb_t * dequeue_wrapper(void* queue){ //for syscall
	pthread_mutex_lock(&queue_lock);
	struct pcb_t * proc = dequeue((struct queue_t*)queue);
	pthread_mutex_unlock(&queue_lock);
	return proc;
}
void enqueue_wrapper(void* queue, struct pcb_t *proc){ //for syscall
	pthread_mutex_lock(&queue_lock);
	enqueue((struct queue_t*)queue, proc);
	pthread_mutex_unlock(&queue_lock);
}
/*
 *  Stateful design for routine calling
 *  based on the priority and our MLQ policy
 *  We implement stateful here using transition technique
 *  State representation   prio = 0 .. MAX_PRIO, curr_slot = 0..(MAX_PRIO - prio)
 */
struct pcb_t *get_mlq_proc(void)
{
	struct pcb_t *proc = NULL;
	pthread_mutex_lock(&queue_lock);

	//fix all slot equal 0
	if(slot[MAX_PRIO-1] == 0){
		for (int i = 0; i < MAX_PRIO; i++)
		{
			slot[i] = MAX_PRIO - i;
		}
	}
	for (int prio = 0; prio < MAX_PRIO; prio++)
	{
		if (!empty(&mlq_ready_queue[prio]) && slot[prio] > 0)
		{
			proc = dequeue(&mlq_ready_queue[prio]);
			slot[prio]--;
			break;
		}
	}
	if(proc!=NULL) enqueue(&running_list, proc); //add proc to running_list
	pthread_mutex_unlock(&queue_lock);
	return proc;
}

void put_mlq_proc(struct pcb_t *proc)
{
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_mlq_proc(struct pcb_t *proc)
{
	pthread_mutex_lock(&queue_lock);
	enqueue(&mlq_ready_queue[proc->prio], proc);
	pthread_mutex_unlock(&queue_lock);
}

struct pcb_t *get_proc(void)
{
	return get_mlq_proc();
}

void put_proc(struct pcb_t *proc)
{
	proc->ready_queue = &ready_queue;
	proc->mlq_ready_queue = mlq_ready_queue;
	proc->running_list = &running_list;

	/* TODO: put running proc to running_list */
	pthread_mutex_lock(&queue_lock);
	enqueue(&running_list, proc);
	pthread_mutex_unlock(&queue_lock);

	put_mlq_proc(proc);
}

void add_proc(struct pcb_t *proc)
{
	proc->ready_queue = &ready_queue;
	proc->mlq_ready_queue = mlq_ready_queue;
	proc->running_list = &running_list;

	/* TODO: put running proc to running_list */
	pthread_mutex_lock(&queue_lock);
	enqueue(&running_list, proc);
	pthread_mutex_unlock(&queue_lock);
	add_mlq_proc(proc);
}
#else
#ifndef CFS_SCHED
struct pcb_t *get_proc(void)
{
	struct pcb_t *proc = NULL;
	/*TODO: get a process from [ready_queue].
	 * Remember to use lock to protect the queue.
	 * */
	pthread_mutex_lock(&queue_lock);
	if (!empty(&ready_queue))
	{
		proc = dequeue(&ready_queue);
	}

	pthread_mutex_unlock(&queue_lock);
	return proc;
}

void put_proc(struct pcb_t *proc)
{
	proc->ready_queue = &ready_queue;
	proc->running_list = &running_list;

	/* TODO: put running proc to running_list */

	pthread_mutex_lock(&queue_lock);
	enqueue(&running_list, proc);
	pthread_mutex_unlock(&queue_lock);
}

void add_proc(struct pcb_t *proc)
{
	proc->ready_queue = &ready_queue;
	proc->running_list = &running_list;

	/* TODO: put running proc to running_list */

	pthread_mutex_lock(&queue_lock);
	enqueue(&running_list, proc);
	pthread_mutex_unlock(&queue_lock);
}
#endif
#endif


