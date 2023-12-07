
#include "sched.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#include "../include/queue.h"
static struct queue_t ready_queue;
static struct queue_t run_queue;

// Synchronization variables
static pthread_mutex_t queue_lock;

#ifdef MLQ_SCHED
static struct queue_t mlq_ready_queue[MAX_PRIO];
#endif

int queue_empty(void) {
#ifdef MLQ_SCHED
  unsigned long prio;
  for (prio = 0; prio < MAX_PRIO; prio++)
    if (!empty(&mlq_ready_queue[prio])) return -1;
#endif
  return (empty(&ready_queue) && empty(&run_queue));
}

void init_scheduler(void) {
#ifdef MLQ_SCHED
  int i;

  for (i = 0; i < MAX_PRIO; i++) {
    mlq_ready_queue[i].size = 0;
    // init number of cpu each queue can use maximally
    mlq_ready_queue[i].cpuRemainder = MAX_PRIO - i;
  }

#endif
  ready_queue.size = 0;
  run_queue.size = 0;
  run_queue.cpuRemainder = MAX_PRIO;
  pthread_mutex_init(&queue_lock, NULL);
}

#ifdef MLQ_SCHED
/*
 *  Stateful design for routine calling
 *  based on the priority and our MLQ policy
 *  We implement stateful here using transition technique
 *  State representation   prio = 0 .. MAX_PRIO, curr_slot = 0..(MAX_PRIO -
 * prio)
 */

struct pcb_t *get_mlq_proc(void) {
  struct pcb_t *proc = NULL;
  unsigned long curr_prio = 0, max_prio = MAX_PRIO;
  // find the highest priority queue that has process
  while (curr_prio < max_prio) {
    // if there is a process in this queue and this queue still has slot
    if (!empty(&mlq_ready_queue[curr_prio]) &&
        mlq_ready_queue[curr_prio].cpuRemainder > 0) {
      proc = dequeue(&mlq_ready_queue[curr_prio]);
      if (proc != NULL) {
        // decrease cpuRemainder
        mlq_ready_queue[curr_prio].cpuRemainder--;
      }
      break;
    } else {
      // if there is no process in this queue or this queue has no slot
      curr_prio++;
    }
  }
  return proc;
}

void put_mlq_proc(struct pcb_t *proc) {
  pthread_mutex_lock(&queue_lock);
  enqueue(&mlq_ready_queue[proc->prio], proc);
  // increase cpuRemainder
  mlq_ready_queue[proc->prio].cpuRemainder++;
  pthread_mutex_unlock(&queue_lock);
}

void add_mlq_proc(struct pcb_t *proc) {
  pthread_mutex_lock(&queue_lock);
  enqueue(&mlq_ready_queue[proc->prio], proc);
  pthread_mutex_unlock(&queue_lock);
}

struct pcb_t *get_proc(int decrSlotCpu) { return get_mlq_proc(); }

void put_proc(struct pcb_t *proc) { return put_mlq_proc(proc); }

void add_proc(struct pcb_t *proc) { return add_mlq_proc(proc); }

void finish_proc(struct pcb_t **proc) {
  pthread_mutex_lock(&queue_lock);
  // increase cpuRemainder
  mlq_ready_queue[(*proc)->prio].cpuRemainder++;

  pthread_mutex_unlock(&queue_lock);
  free(*proc);
}
#else
struct pcb_t *get_proc(void) {
  struct pcb_t *proc = NULL;
  /*TODO: get a process from [ready_queue].
   * Remember to use lock to protect the queue.
   * */
  if (!empty(&ready_queue)) {
    proc = dequeue(&ready_queue);
  }
  return proc;
}

void put_proc(struct pcb_t *proc) { enqueue(&run_queue, proc); }

void add_proc(struct pcb_t *proc) { enqueue(&ready_queue, proc); }

void finish_proc(struct pcb_t **proc) { free(*proc); }
#endif
