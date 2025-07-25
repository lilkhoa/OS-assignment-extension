#include "cpu.h"
#include "timer.h"
#include "sched.h"
#include "loader.h"
#include "mm.h"

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef CFS_SCHED
#include <time.h>
#endif

int time_slot; // Make time_slot globally accessible for CFS scheduling
static int num_cpus;
static int done = 0;

static uint32_t total_waiting_time = 0;
static uint32_t total_turnaround_time = 0;
static uint32_t completed_processes = 0;

#ifdef MM_PAGING
static int memramsz;
static int memswpsz[PAGING_MAX_MMSWP];

struct mmpaging_ld_args {
	/* A dispatched argument struct to compact many-fields passing to loader */
	int vmemsz;
	struct memphy_struct *mram;
	struct memphy_struct **mswp;
	struct memphy_struct *active_mswp;
	int active_mswp_id;
	struct timer_id_t  *timer_id;
};
#endif

static struct ld_args{
	char ** path;
	unsigned long * start_time;
	unsigned long * prio;    // Always include priority for MLQ
	int * niceness;          // Always include niceness for CFS
} ld_processes;
int num_processes;

struct cpu_args {
	struct timer_id_t * timer_id;
	int id;
};

#ifdef CFS_SCHED
static clock_t start_time;


/* Modified CPU routine for CFS */
static void * cfs_cpu_routine(void * args) {
    struct timer_id_t * timer_id = ((struct cpu_args*)args)->timer_id;
    int id = ((struct cpu_args*)args)->id;
    
    struct pcb_t * proc = NULL;
    uint32_t time_left = 0;
    uint32_t executed_time = 0;
    
    while (1) {
        /* Check the status of current process */
        if (proc == NULL) {
            /* No process is running, then we load new process from ready queue */
            proc = get_proc();
            if (proc == NULL) {
                next_slot(timer_id);
                continue; /* First load failed. skip dummy load */
            }
        } else if (proc->pc == proc->code->size) {
            /* The process has finished its job */
            proc->finish_time = current_time();
            proc->cpu_burst_time += executed_time;
            proc->turnaround_time = proc->finish_time - proc->arrival_time;
            proc->waiting_time = proc->turnaround_time - proc->cpu_burst_time;
            
            total_waiting_time += proc->waiting_time;
            total_turnaround_time += proc->turnaround_time;
            completed_processes++;
            
            printf("\tCPU %d: Processed %2d has finished (niceness: %d, vruntime: %f)\n",
                id, proc->pid, proc->niceness, proc->vruntime);
            printf("\t      Waiting time: %u, Turnaround time: %u, CPU burst time: %u\n",
                proc->waiting_time, proc->turnaround_time, proc->cpu_burst_time);
            
            /* Update vruntime based on actual execution time */
            if (executed_time > 0) {
                update_vruntime(proc, executed_time);
            }
            
            free(proc);
            proc = get_proc();
            time_left = 0;
            executed_time = 0;
        } else if (time_left == 0) {
            /* Update vruntime based on actual execution time */
            if (executed_time > 0) {
                update_vruntime(proc, executed_time);
                proc->cpu_burst_time += executed_time;
            }

            /* The process has done its job in current time slice or has been preempted */
            printf("\tCPU %d: Put process %2d to run queue (niceness: %d, vruntime: %f)\n",
                id, proc->pid, proc->niceness, proc->vruntime);
            
            put_proc(proc);
            proc = get_proc();
            executed_time = 0;
        }
        
        /* Recheck process status after loading new process */
        if (proc == NULL && done) {
            /* No process to run, exit */
            printf("\tCPU %d stopped\n", id);
            break;
        } else if (proc == NULL) {
            /* There may be new processes to run in next time slots, just skip current slot */
            next_slot(timer_id);
            continue;
        } else if (time_left == 0) {
            printf("\tCPU %d: Dispatched process %2d (niceness: %d, weight: %f, vruntime: %f, time_slice: %u)\n",
                id, proc->pid, proc->niceness, proc->weight, proc->vruntime, proc->time_slice);
            
            /* Set time_left to the dynamically calculated time slice for this process */
            time_left = proc->time_slice;
            executed_time = 0;
        }
        
        /* Run current process */
        run(proc);
        time_left--;
        executed_time++;
        next_slot(timer_id);
    }
    
    detach_event(timer_id);
    pthread_exit(NULL);
}
#endif

static void * cpu_routine(void * args) {
	struct timer_id_t * timer_id = ((struct cpu_args*)args)->timer_id;
	int id = ((struct cpu_args*)args)->id;
	/* Check for new process in ready queue */
	int time_left = 0;
	struct pcb_t * proc = NULL;
	uint32_t executed_time = 0;
	
	while (1) {
		/* Check the status of current process */
		if (proc == NULL) {
			/* No process is running, the we load new process from
		 	* ready queue */
			proc = get_proc();
			if (proc == NULL) {
                next_slot(timer_id);
                continue; /* First load failed. skip dummy load */
            }
            executed_time = 0;
		} else if (proc->pc == proc->code->size) {
			/* The process has finish it job */
			proc->finish_time = current_time();
			proc->cpu_burst_time += executed_time;
			proc->turnaround_time = proc->finish_time - proc->arrival_time;
			proc->waiting_time = proc->turnaround_time - proc->cpu_burst_time;
			
			// Update global statistics
			total_waiting_time += proc->waiting_time;
			total_turnaround_time += proc->turnaround_time;
			completed_processes++;
			
			printf("\tCPU %d: Processed %2d has finished\n", id, proc->pid);
#ifdef MLQ_SCHED
			printf("\t      Priority: %u, ", proc->prio);
#endif
			printf("Waiting time: %u, Turnaround time: %u, CPU burst time: %u\n",
				proc->waiting_time, proc->turnaround_time, proc->cpu_burst_time);
			
			free(proc);
			proc = get_proc();
			time_left = 0;
			executed_time = 0;
		} else if (time_left == 0) {
			/* The process has done its job in current time slot */
			proc->cpu_burst_time += executed_time;
			
			printf("\tCPU %d: Put process %2d to run queue\n",
				id, proc->pid);
			put_proc(proc);
			proc = get_proc();
			executed_time = 0;
		}
		
		/* Recheck process status after loading new process */
		if (proc == NULL && done) {
			/* No process to run, exit */
			printf("\tCPU %d stopped\n", id);
			break;
		} else if (proc == NULL) {
			/* There may be new processes to run in
			 * next time slots, just skip current slot */
			next_slot(timer_id);
			continue;
		} else if (time_left == 0) {
			printf("\tCPU %d: Dispatched process %2d\n",
				id, proc->pid);
			time_left = time_slot;
			executed_time = 0;
		}
		
		/* Run current process */
		run(proc);
		time_left--;
		executed_time++;
		next_slot(timer_id);
	}
	detach_event(timer_id);
	pthread_exit(NULL);
}

static void * ld_routine(void * args) {
#ifdef MM_PAGING
    struct memphy_struct* mram = ((struct mmpaging_ld_args *)args)->mram;
    struct memphy_struct** mswp = ((struct mmpaging_ld_args *)args)->mswp;
    struct memphy_struct* active_mswp = ((struct mmpaging_ld_args *)args)->active_mswp;
    struct timer_id_t * timer_id = ((struct mmpaging_ld_args *)args)->timer_id;
#else
    struct timer_id_t * timer_id = (struct timer_id_t*)args;
#endif
    int i = 0;
    printf("ld_routine\n");
    while (i < num_processes) {
        struct pcb_t * proc = load(ld_processes.path[i]);
#ifdef MLQ_SCHED
        proc->prio = ld_processes.prio[i];
#endif
#ifdef CFS_SCHED
        proc->niceness = ld_processes.niceness[i]; 
#endif
        while (current_time() < ld_processes.start_time[i]) {
            next_slot(timer_id);
        }
#ifdef MM_PAGING
        proc->mm = malloc(sizeof(struct mm_struct));
        init_mm(proc->mm, proc);
        proc->mram = mram;
        proc->mswp = mswp;
        proc->active_mswp = active_mswp;
#endif
        proc->arrival_time = current_time();
        proc->cpu_burst_time = 0;
        proc->finish_time = 0;
        proc->waiting_time = 0;
        proc->turnaround_time = 0;
        
        printf("\tLoaded a process at %s, PID: %d", ld_processes.path[i], proc->pid);
#ifdef MLQ_SCHED
        printf(", PRIO: %ld", ld_processes.prio[i]);
#endif
#ifdef CFS_SCHED
        printf(", NICENESS: %d", ld_processes.niceness[i]);
#endif
        printf("\n");
        add_proc(proc);
        free(ld_processes.path[i]);
        i++;
        next_slot(timer_id);
    }
    free(ld_processes.path);
    free(ld_processes.start_time);
#ifdef MLQ_SCHED
    free(ld_processes.prio);
#endif
#ifdef CFS_SCHED
	free(ld_processes.niceness);
#endif
    done = 1;
    detach_event(timer_id);
    pthread_exit(NULL);
}

static void read_config(const char * path) {
	FILE * file;
	if ((file = fopen(path, "r")) == NULL) {
		printf("Cannot find configure file at %s\n", path);
		exit(1);
	}
	fscanf(file, "%d %d %d\n", &time_slot, &num_cpus, &num_processes);
	ld_processes.path = (char**)malloc(sizeof(char*) * num_processes);
	ld_processes.start_time = (unsigned long*)
		malloc(sizeof(unsigned long) * num_processes);
#ifdef MM_PAGING
	int sit;
#ifdef MM_FIXED_MEMSZ
	/* We provide here a back compatible with legacy OS simulatiom config file
         * In which, it have no addition config line for Mema, keep only one line
	 * for legacy info 
         *  [time slice] [N = Number of CPU] [M = Number of Processes to be run]
         */
        memramsz    =  0x100000;
        memswpsz[0] = 0x1000000;
	for(sit = 1; sit < PAGING_MAX_MMSWP; sit++)
		memswpsz[sit] = 0;
#else
	/* Read input config of memory size: MEMRAM and upto 4 MEMSWP (mem swap)
	 * Format: (size=0 result non-used memswap, must have RAM and at least 1 SWAP)
	 *        MEM_RAM_SZ MEM_SWP0_SZ MEM_SWP1_SZ MEM_SWP2_SZ MEM_SWP3_SZ
	*/
	fscanf(file, "%d\n", &memramsz);
	for(sit = 0; sit < PAGING_MAX_MMSWP; sit++)
		fscanf(file, "%d", &(memswpsz[sit])); 

       fscanf(file, "\n"); /* Final character */
#endif
#endif

	// Allocate memory for both prio and niceness arrays
	ld_processes.prio = (unsigned long*)malloc(sizeof(unsigned long) * num_processes);
	ld_processes.niceness = (int*)malloc(sizeof(int) * num_processes);
	
	int i;
	for (i = 0; i < num_processes; i++) {
		ld_processes.path[i] = (char*)malloc(sizeof(char) * 100);
		ld_processes.path[i][0] = '\0';
		strcat(ld_processes.path[i], "input/proc/");
		char proc[100];
		
		// Try to read both priority and niceness values
		int result = fscanf(file, "%lu %s %lu %d\n", 
			&ld_processes.start_time[i], 
			proc, 
			&ld_processes.prio[i], 
			&ld_processes.niceness[i]);
		
		// If we only read 3 values (no niceness), set a default niceness
		if (result == 3) {
			ld_processes.niceness[i] = 0; // Default niceness is 0
		}
		
		strcat(ld_processes.path[i], proc);
	}
}

int main(int argc, char * argv[]) {
	/* Read config */
	if (argc != 2) {
		printf("Usage: os [path to configure file]\n");
		return 1;
	}
	char path[100];
	path[0] = '\0';
	strcat(path, "input/");
	strcat(path, argv[1]);
	read_config(path);

	pthread_t * cpu = (pthread_t*)malloc(num_cpus * sizeof(pthread_t));
	struct cpu_args * args =
		(struct cpu_args*)malloc(sizeof(struct cpu_args) * num_cpus);
	pthread_t ld;
	
	/* Init timer */
	int i;
	for (i = 0; i < num_cpus; i++) {
		args[i].timer_id = attach_event();
		args[i].id = i;
	}
	struct timer_id_t * ld_event = attach_event();
	start_timer();

#ifdef MM_PAGING
	/* Init all MEMPHY include 1 MEMRAM and n of MEMSWP */
	int rdmflag = 1; /* By default memphy is RANDOM ACCESS MEMORY */

	struct memphy_struct mram;
	struct memphy_struct mswp[PAGING_MAX_MMSWP];

	/* Create MEM RAM */
	init_memphy(&mram, memramsz, rdmflag);

        /* Create all MEM SWAP */ 
	int sit;
	for(sit = 0; sit < PAGING_MAX_MMSWP; sit++)
	       init_memphy(&mswp[sit], memswpsz[sit], rdmflag);

	/* In Paging mode, it needs passing the system mem to each PCB through loader*/
	struct mmpaging_ld_args *mm_ld_args = malloc(sizeof(struct mmpaging_ld_args));

	mm_ld_args->timer_id = ld_event;
	mm_ld_args->mram = (struct memphy_struct *) &mram;
	mm_ld_args->mswp = (struct memphy_struct**) &mswp;
	mm_ld_args->active_mswp = (struct memphy_struct *) &mswp[0];
        mm_ld_args->active_mswp_id = 0;
#endif

	/* Init scheduler */
	init_scheduler();

	/* Run CPU and loader */
#ifdef MM_PAGING
	pthread_create(&ld, NULL, ld_routine, (void*)mm_ld_args);
#else
	pthread_create(&ld, NULL, ld_routine, (void*)ld_event);
#endif
#ifdef CFS_SCHED
	for (i = 0; i < num_cpus; i++) {
		pthread_create(&cpu[i], NULL,
			cfs_cpu_routine, (void*)&args[i]);
	}
#else
	for (i = 0; i < num_cpus; i++) {
		pthread_create(&cpu[i], NULL,
			cpu_routine, (void*)&args[i]);
	}
#endif

	/* Wait for CPU and loader finishing */
	for (i = 0; i < num_cpus; i++) {
		pthread_join(cpu[i], NULL);
	}
	pthread_join(ld, NULL);

	/* Stop timer */
	stop_timer();
	
	if (completed_processes > 0) {
		float avg_waiting_time = (float)total_waiting_time / completed_processes;
		float avg_turnaround_time = (float)total_turnaround_time / completed_processes;
		
		printf("\n=== Scheduling Statistics ===\n");
#ifdef CFS_SCHED
		printf("Scheduler: CFS (Completely Fair Scheduler)\n");
#endif
#ifdef MLQ_SCHED
		printf("Scheduler: MLQ (Multi-Level Queue)\n");
#endif
		printf("Number of processes completed: %u\n", completed_processes);
		printf("Average waiting time: %.2f time units\n", avg_waiting_time);
		printf("Average turnaround time: %.2f time units\n", avg_turnaround_time);
		printf("============================\n");
	}

	return 0;
}