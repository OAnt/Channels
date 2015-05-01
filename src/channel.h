#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdlib.h>

typedef struct queue_st queue_t;
typedef struct queue_st priority_queue_t;

queue_t * queue_new(unsigned int n, size_t size);
// blocking
int queue_take(queue_t * queue, void * data);
// blocking, waits up to sec for data to be available
int queue_timed_take(queue_t * queue, void * data, unsigned int sec);
// non blocking
int queue_try_take(queue_t * queue, void * data);
// blocking
int queue_put(queue_t * queue, void * value);
// blocking, waits up to sec for room to be available
int queue_timed_put(queue_t * queue, void * data, unsigned int sec);
// non blocking
int queue_try_put(queue_t * queue, void * data);
void queue_free(queue_t * queue);

priority_queue_t * priority_queue_new(unsigned int n, size_t size);
// blocking
int priority_queue_take(priority_queue_t * q, void * data);
// blocking, waits up to sec for data to be available
int priority_queue_timed_take(priority_queue_t * q, 
        void * data, unsigned int sec);
// non blocking
int priority_queue_try_take(priority_queue_t * q,
        void * data);
// blocking
int priority_queue_put(priority_queue_t *q, void *data, int priority);
// blocking, waits up to sec for data to be available
int priority_queue_timed_put(priority_queue_t * q, 
        void *data, int priority, unsigned int sec);
// non blocking
int priority_queue_try_put(priority_queue_t * q,
        void * data, int priority);
void priority_queue_free(priority_queue_t * q);

//beware a queue can only be selected once
//for not full and once for not empty
//at the time
int queue_select_not_full(struct queue_st ** q, int n,
        struct queue_st ** selected_queue, int * ns);
int queue_timed_select_not_full(struct queue_st ** q, int n,
        struct queue_st ** selected_queue, int *ns,
        unsigned int s);
int queue_select_not_empty(struct queue_st ** q, int n,
        struct queue_st ** selected_queue, int * ns);
int queue_timed_select_not_empty(struct queue_st ** q, int n,
        struct queue_st ** selected_queue, int * ns,
        unsigned int s);

void queue_print(struct queue_st * q);

#endif
