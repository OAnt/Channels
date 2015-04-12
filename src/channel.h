#ifndef MY_THREAD_H
#define MY_THREAD_H

#include <pthread.h>
#include "buffer.h"

typedef struct queue_st queue_t;
typedef struct data_control_st dctrl_t;

int queue_init(queue_t * queue, unsigned int n, size_t size);
void * queue_take(queue_t * queue, void * data);
void queue_put(queue_t * queue, void * value);
void queue_free(queue_t * queue);

#endif
