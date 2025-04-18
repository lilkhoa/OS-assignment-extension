#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

// #ifndef MLQ_SCHED
// #define MLQ_SCHED
// #endif

#ifndef CFS_SCHED
#define CFS_SCHED
#endif

#define MAX_PRIO 140

int queue_empty(void);

void init_scheduler(void);
void finish_scheduler(void);

/* Get the next process from ready queue */
struct pcb_t * get_proc(void);

/* Put a process back to run queue */
void put_proc(struct pcb_t * proc);

/* Add a new process to ready queue */
void add_proc(struct pcb_t * proc);

#ifdef CFS_SCHED
void update_vruntime(struct pcb_t * proc, uint32_t exec_time);
#endif

#ifdef CFS_SCHED
uint32_t get_min_vruntime(void);
#endif

#endif


