#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdlib.h>

typedef struct queue_st queue_t;
typedef struct queue_st priority_queue_t;

queue_t * queue_new(unsigned int n, size_t size);
int queue_take(queue_t * queue, void * data);
int queue_timed_take(queue_t * queue, void * data, unsigned int sec);
int queue_put(queue_t * queue, void * value);
int queue_timed_put(queue_t * queue, void * data, unsigned int sec);
void queue_free(queue_t * queue);

priority_queue_t * priority_queue_new(unsigned int n, size_t size);
int priority_queue_take(priority_queue_t * q, void * data);
int priority_queue_timed_take(priority_queue_t * q, 
        void * data, unsigned int sec);
int priority_queue_put(priority_queue_t *q, void *data, int priority);
int priority_queue_timed_put(priority_queue_t * q, 
        void *data, int priority, unsigned int sec);
void priority_queue_free(priority_queue_t * q);

//beware a queue can only be selected once
//for not full and once for not empty
//at the time
int queue_select_not_full(queue_t ** q, int n,
        queue_t ** selected_queue, int * ns);
int queue_timed_select_not_full(queue_t ** q, int n,
        queue_t ** selected_queue, int *ns,
        unsigned int s);
int queue_select_not_empty(queue_t ** q, int n,
        queue_t ** selected_queue, int * ns);
int queue_timed_select_not_empty(queue_t ** q, int n,
        queue_t ** selected_queue, int * ns,
        unsigned int s);

void queue_print(queue_t * q);

#endif
