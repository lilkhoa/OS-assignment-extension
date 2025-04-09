#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t *q)
{
        if (q == NULL)
                return 1;
        return (q->size == 0);
}

void enqueue(struct queue_t *q, struct pcb_t *proc)
{
        if (q == NULL || proc == NULL)
                return;

        if (q->size >= MAX_QUEUE_SIZE)
        {
                return;
        }

        // Add process to the endd of the queue
        q->proc[q->size] = proc;
        q->size++;
}

struct pcb_t *dequeue(struct queue_t *q)
{
        if (q == NULL || q->size == 0)
                return NULL;

        // Find the process with the highest priority
        int max_index = 0;
        for (int i = 1; i < q->size; i++)
        {
                if (q->proc[i]->priority > q->proc[max_index]->priority)
                        max_index = i;
        }

        // Retrieve thee process with the highest priority
        struct pcb_t *selected_proc = q->proc[max_index];

        // Shift elements after max_index to the left to fill the gap
        for (int i = max_index; i < q->size - 1; i++)
        {
                q->proc[i] = q->proc[i + 1];
        }
        q->size--;

        return selected_proc;
}