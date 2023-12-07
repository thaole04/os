#include "queue.h"

#include <stdio.h>
#include <stdlib.h>

int empty(struct queue_t *q) {
  if (q == NULL) return 1;
  return (q->size == 0);
}

void enqueue(struct queue_t *q, struct pcb_t *proc) {
  // Check for NULL queue or process
  if (q == NULL || proc == NULL) return;
  // Check for full queue
  if (q->size >= MAX_QUEUE_SIZE) {
    printf("Full Queue!\n");
    return;
  }
  // Add process to queue
  q->proc[q->size] = proc;
  // Increment queue size
  q->size++;
}

struct pcb_t *dequeue(struct queue_t *q) {
  // Check for NULL queue
  if (q == NULL || q->size == 0) return NULL;
  struct pcb_t *first_proc = q->proc[0];
  // Shift all processes to the left
  for (int i = 0; i < q->size - 1; i++) {
    q->proc[i] = q->proc[i + 1];
  }
  // Decrement queue size
  q->size--;
  return first_proc;
}
