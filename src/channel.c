#include <pthread.h>
#include <sys/time.h>
#include <assert.h>

#include "channel.h"
#include "buffer.h"

typedef enum{
    FIFO_CHANNEL,
    PRIORITY_CHANNEL,
}channel_type_t;

typedef struct data_control_st dctrl_t;
struct data_control_st {
    pthread_mutex_t mutex;
    pthread_cond_t empty;
    pthread_cond_t full;
};

struct queue_st {
    channel_type_t type;
    dctrl_t ctrl;
    buffer_t rb;
};

int init_dctrl(dctrl_t * dctrl){
    int err_code;
    if((err_code = pthread_mutex_init(&(dctrl->mutex), NULL)))
        return err_code;
    if((err_code = pthread_cond_init(&(dctrl->empty), NULL))){
	pthread_mutex_destroy(&(dctrl->mutex));
        return err_code;
    }
    if((err_code = pthread_cond_init(&(dctrl->full), NULL))){
	pthread_mutex_destroy(&(dctrl->mutex));
	pthread_cond_destroy(&(dctrl->empty));
        return err_code;
    }
    return 0;
}

int dctrl_free(dctrl_t * dctrl){
    if(pthread_mutex_destroy(&(dctrl->mutex)))
        return 1;
    if(pthread_cond_destroy(&(dctrl->empty)))
        return 1;
    if(pthread_cond_destroy(&(dctrl->full)))
        return 1;
    return 0;
}

int notify_not_empty(dctrl_t * dctrl) {
    return pthread_cond_broadcast(&(dctrl->empty));
}

int wait_empty(dctrl_t * dctrl, struct timespec * abstime){
    if(abstime)
	return pthread_cond_timedwait(&(dctrl->empty),
				 &(dctrl->mutex),
				 abstime);
    else
	return pthread_cond_wait(&(dctrl->empty),
				 &(dctrl->mutex));
}

int notify_not_full(dctrl_t * dctrl) {
    return pthread_cond_broadcast(&(dctrl->full));
}

int wait_full(dctrl_t * dctrl, struct timespec * abstime){
    if(abstime)
	return pthread_cond_timedwait(&(dctrl->full),
				 &(dctrl->mutex), 
				 abstime);
    else
	return pthread_cond_wait(&(dctrl->full),
				 &(dctrl->mutex));
}

void _gettimer(struct timespec * ts, unsigned int sec){
    struct timeval tv;
    gettimeofday(&tv, NULL);    
    ts->tv_nsec = 0;
    ts->tv_sec = tv.tv_sec + sec;
}

int _queue_take(queue_t *queue, void * data, 
        struct timespec * abstime, buffer_take f)
{
    pthread_mutex_lock(&(queue->ctrl.mutex));
    int res;
    while((res = f(&(queue->rb), data)) == 0){
	int err;
        if((err = wait_empty(&(queue->ctrl), abstime)) != 0) {
	    pthread_mutex_unlock(&(queue->ctrl.mutex));
	    return err;
	}
    }
    pthread_mutex_unlock(&(queue->ctrl.mutex));
    notify_not_full(&(queue->ctrl));
    return 0;
}

int _queue_put(queue_t * queue, void * value, 
        struct timespec * abstime, buffer_write f, int priority)
{
    pthread_mutex_lock(&(queue->ctrl.mutex));
    int res;
    while((res = f(&(queue->rb), value, priority)) == 0){
	int err;
        if((err = wait_full(&(queue->ctrl), abstime)) != 0){
	    pthread_mutex_unlock(&(queue->ctrl.mutex));
	    return err;
	}
    }
    pthread_mutex_unlock(&(queue->ctrl.mutex));
    notify_not_empty(&(queue->ctrl));
    return 0;
}

int queue_init(queue_t * queue, unsigned int n, size_t size,
        channel_type_t type)
{
    queue->type = type;
    int err_code;
    if((err_code = init_dctrl(&(queue->ctrl))) != 0) 
	return err_code;
    if((type == FIFO_CHANNEL && 
                (err_code = buffer_init(&(queue->rb), n, size, RING_BUFFER)) != 0) ||
            (type == PRIORITY_CHANNEL &&
             (err_code = heap_init(&(queue->rb), n, size)) != 0)){ 
	dctrl_free(&queue->ctrl);
	return err_code;
    }
    return 0;
}

queue_t * _queue_new(unsigned int n, size_t size, channel_type_t type){
    queue_t * q = calloc(1, sizeof(queue_t));
    if(!q) return q;
    if(queue_init(q, n, size, type) != 0){
	free(q);
	return NULL;
    }
    return q;
}

void queue_free(queue_t * queue){
    buffer_free(&(queue->rb));
    dctrl_free(&(queue->ctrl));
    free(queue);
}

int queue_take(queue_t * q, void * data){
    assert(q->type == FIFO_CHANNEL);
    return _queue_take(q, data, NULL, rb_take);
}

int queue_timed_take(queue_t * q, void * data, unsigned int sec){
    assert(q->type == FIFO_CHANNEL);
    struct timespec ts;
    _gettimer(&ts, sec);
    return _queue_take(q, data, &ts, rb_take);
}

int queue_put(queue_t *q, void *data){
    assert(q->type == FIFO_CHANNEL);
    return _queue_put(q, data, NULL, rb_write, 0);
}

int queue_timed_put(queue_t * q, void *data, unsigned int sec){
    assert(q->type == FIFO_CHANNEL);
    struct timespec ts;
    _gettimer(&ts, sec);
    return _queue_put(q, data, &ts, rb_write, 0);
}

queue_t * queue_new(unsigned int n, size_t size){
    return _queue_new(n, size, FIFO_CHANNEL);
}

priority_queue_t * priority_queue_new(unsigned int n, size_t size){
    return _queue_new(n, size, PRIORITY_CHANNEL);
}

int priority_queue_take(priority_queue_t * q, void * data){
    assert(q->type == PRIORITY_CHANNEL);
    return _queue_take(q, data, NULL, hb_take);
}

int priority_queue_timed_take(priority_queue_t * q, 
        void * data, unsigned int sec){
    assert(q->type == PRIORITY_CHANNEL);
    struct timespec ts;
    _gettimer(&ts, sec);
    return _queue_take(q, data, &ts, hb_take);
}

int priority_queue_put(priority_queue_t *q, void *data, int priority){
    assert(q->type == PRIORITY_CHANNEL);
    return _queue_put(q, data, NULL, hb_write, priority);
}

int priority_queue_timed_put(priority_queue_t * q, 
        void *data, int priority, unsigned int sec){
    assert(q->type == PRIORITY_CHANNEL);
    struct timespec ts;
    _gettimer(&ts, sec);
    return _queue_put(q, data, &ts, hb_write, priority);
}

void priority_queue_free(priority_queue_t * q){
    queue_free(q);
}

