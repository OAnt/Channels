#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdlib.h>

typedef struct queue_st queue_t;
typedef struct queue_st priority_queue_t;

/*
* allocates a new fifo queue able to hold n elements of size size.
* returns NULL if the initialization was unsuccesful at some point
*/
queue_t * queue_new(unsigned int n, size_t size);
/*
* retrieves the first element from the queue and copies it to
* data
* or blocks until an element is available
* data must point to block of memory of at least size
* where size is the value passed when allocating the queue
* returns 0 when the operation is succesful
* returns EINVAL if the queue is not properly initialized
*/
int queue_take(queue_t * queue, void * data);
/*
* retrieves the first element from the queue and copies it to
* data
* or blocks until an element is available or if sec seconds have passed
* data must point to block of memory of at least size
* where size is the value passed when allocating the queue
* returns 0 when the operation is succesful
* returns EINVAL if the queue is not properly initialized
* returns ETIMEDOUT if the when the timer has ellapsed
*/
int queue_timed_take(queue_t * queue, void * data, unsigned int sec);
/*
* tries to retrieve the first element from the queue and copies it to
* data
* this call is non blocking
* data must point to block of memory of at least size
* where size is the value passed when allocating the queue
* returns 0 when the operation is succesful
* returns EINVAL if the queue is not properly initialized
* returns EAGAIN if no data is available
* returns EBUSY if another thread is already trying to access
* the queue
*/
int queue_try_take(queue_t * queue, void * data);
/*
* retrieves the first element from the queue and copies it to
* data
* this call is non blocking
* data must point to block of memory of at least size
* where size is the value passed when allocating the queue
* returns 0 when the operation is succesful
* returns EINVAL if the queue is not properly initialized
* returns EAGAIN if no data is available
*/
int queue_no_wait_take(priority_queue_t * q, void * data);
/*
* copies the memory pointed by value into the queue
* or blocks until there is room in the queue.
* returns 0 when the operation is succesful
* returns EINVAL if the queue is not properly initialized
*/
int queue_put(queue_t * queue, void * value);
/*
* copies the memory pointed by value into the queue
* or blocks until there is room in the queue or if 
* sec second have passed.
* returns 0 when the operation is succesful
* returns EINVAL if the queue is not properly initialized
* returns ETIMEDOUT if the when the timer has ellapsed
*/
int queue_timed_put(queue_t * queue, void * data, unsigned int sec);
/*
* tries to copy the memory pointed by value into the queue
* this call is non blocking
* returns 0 when the operation is succesful
* returns EINVAL if the queue is not properly initialized
* returns EAGAIN if no room is available
* returns EBUSY if another thread is already trying to access
* the queue
*/
int queue_try_put(queue_t * queue, void * data);
/*
* copies the memory pointed by value into the queue
* this call is non blocking
* returns 0 when the operation is succesful
* returns EINVAL if the queue is not properly initialized
* returns EAGAIN if no room is available
*/
int queue_no_wait_put(queue_t *q, void *data);
/*
* free a previously allocated queue
*/
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
int priority_queue_no_wait_take(priority_queue_t * q, 
        void * data);
// blocking
int priority_queue_put(priority_queue_t *q, void *data, int priority);
// blocking, waits up to sec for data to be available
int priority_queue_timed_put(priority_queue_t * q, 
        void *data, int priority, unsigned int sec);
// non blocking
int priority_queue_try_put(priority_queue_t * q,
        void * data, int priority);
int priority_queue_no_wait_put(priority_queue_t *q, 
        void *data, int priority);
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

typedef struct notification_callback_st notification_callback_t;
typedef void(callback_t)(struct queue_st * q, void * data);

notification_callback_t * queue_append_not_empty_callback(
        struct queue_st *q,
        callback_t callback, void * data);
notification_callback_t * queue_append_not_full_callback(
        struct queue_st *q,
        callback_t callback, void * data);
void queue_remove_callback(notification_callback_t * nc);

#endif
