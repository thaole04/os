#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"
#include "queue.h"

#ifndef MLQ_SCHED
#define MLQ_SCHED
#endif

#define MAX_PRIO 139

int queue_empty(void);

void init_scheduler(void);
void finish_scheduler(void);

//void incrNumberOfCpuCanUse(struct queue_t *q);

//void decrNumberOfCpuCanUse(struct queue_t *q);

/* Get the next process from ready queue */
struct pcb_t * get_proc(void);

/* Put a process back to run queue */
void put_proc(struct pcb_t * proc);

/* Add a new process to ready queue */
void add_proc(struct pcb_t * proc);

/* Handle when proc is done*/
void finish_proc(struct pcb_t ** proc);

#endif


