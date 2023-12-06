#include "queue.h"
#include <stdio.h>
#include <stdlib.h>

int empty(struct queue_t *q) {
  if (q == NULL)
    return 1;
  return (q->size == 0);
}

void enqueue(struct queue_t *q, struct pcb_t *proc) {
  // check for null pointers
  if (q == NULL || proc == NULL)
    return;
  // check for full queue
  if (q->size >= MAX_QUEUE_SIZE) {
    printf("Full Queue!\n");
    return;
  }
  // add process to end of array
  q->proc[q->size] = proc;
  // increment queue size
  q->size++;
}

struct pcb_t *dequeue(struct queue_t *q) {
  // check for empty queue
  if (q == NULL || q->size == 0)
    return NULL;
  struct pcb_t *first_proc = q->proc[0];
  // remove first process from queue
  for (int i = 0; i < q->size - 1; i++) {
    q->proc[i] = q->proc[i + 1];
  }
  // decrement queue size
  q->size--;
  return first_proc;
}
