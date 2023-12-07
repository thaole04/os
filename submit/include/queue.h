
#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

#define MAX_QUEUE_SIZE 10

/**
 * @struct queue_t
 * @brief Represents a queue data structure for storing process control blocks
 * (PCBs).
 *
 * The queue_t struct contains an array of pointers to PCBs, the size of the
 * queue, and an optional field for CPU remainder (used in MLQ scheduling).
 */
struct queue_t {
  struct pcb_t *proc[MAX_QUEUE_SIZE]; /**< Array of pointers to PCBs */
  int size;                           /**< Size of the queue */
#ifdef MLQ_SCHED
  int cpuRemainder; /**< CPU remainder for MLQ scheduling */
#endif
};

void enqueue(struct queue_t *q, struct pcb_t *proc);

struct pcb_t *dequeue(struct queue_t *q);

int empty(struct queue_t *q);

#endif
