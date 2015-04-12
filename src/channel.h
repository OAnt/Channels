#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdlib.h>

typedef struct queue_st queue_t;

queue_t * queue_new(unsigned int n, size_t size);
int queue_take(queue_t * queue, void * data);
int queue_timed_take(queue_t * queue, void * data, unsigned int sec);
int queue_put(queue_t * queue, void * value);
int queue_timed_put(queue_t * queue, void * data, unsigned int sec);
void queue_free(queue_t * queue);

#endif