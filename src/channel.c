#include <pthread.h>
#include <sys/time.h>

#include "channel.h"
#include "buffer.h"

typedef struct data_control_st dctrl_t;
struct data_control_st {
    pthread_mutex_t mutex;
    pthread_cond_t empty;
    pthread_cond_t full;
    int active;
};

struct queue_st {
    dctrl_t ctrl;
    ring_buffer_t rb;
};

int init_dctrl(dctrl_t * dctrl){
    int err_code;
    if((err_code = pthread_mutex_init(&(dctrl->mutex), NULL)))
        return err_code;
    if((err_code = pthread_cond_init(&(dctrl->empty), NULL)))
        return err_code;
    if((err_code = pthread_cond_init(&(dctrl->full), NULL)))
        return err_code;
    dctrl->active = 0;
    return 0;
}

int dctrl_free(dctrl_t * dctrl){
    if(pthread_mutex_destroy(&(dctrl->mutex)))
        return 1;
    if(pthread_cond_destroy(&(dctrl->empty)))
        return 1;
    if(pthread_cond_destroy(&(dctrl->full)))
        return 1;
    dctrl->active = 0;
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
    if(abstime )
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

int _queue_take(queue_t *queue, void * data, struct timespec * abstime){
    pthread_mutex_lock(&(queue->ctrl.mutex));
    int res;
    while((res = rb_take(&(queue->rb), data)) == 0){
	int err;
        if((err = wait_empty(&(queue->ctrl), abstime)) != 0) return err;
    }
    pthread_mutex_unlock(&(queue->ctrl.mutex));
    notify_not_full(&(queue->ctrl));
    return 0;
}

int queue_take(queue_t * q, void * data){
    return _queue_take(q, data, NULL);
}

int queue_timed_take(queue_t * q, void * data, unsigned int sec){
    struct timespec ts;
    _gettimer(&ts, sec);
    return _queue_take(q, data, &ts);
}

int _queue_put(queue_t * queue, void * value, struct timespec * abstime){
    pthread_mutex_lock(&(queue->ctrl.mutex));
    int res;
    while((res = rb_write(&(queue->rb), value)) == 0){
	int err;
        if((err = wait_full(&(queue->ctrl), abstime)) != 0) return err;
    }
    pthread_mutex_unlock(&(queue->ctrl.mutex));
    notify_not_empty(&(queue->ctrl));
    return 0;
}

int queue_put(queue_t *q, void *data){
    return _queue_put(q, data, NULL);
}

int queue_timed_put(queue_t * q, void *data, unsigned int sec){
    struct timespec ts;
    _gettimer(&ts, sec);
    return _queue_put(q, data, &ts);
}

int queue_init(queue_t * queue, unsigned int n, size_t size){
    int err_code;
    if((err_code = init_dctrl(&(queue->ctrl))) != 0) 
	return err_code;
    if((err_code = rb_init_ring_buffer(&(queue->rb), n, size)) != 0){ 
	dctrl_free(&queue->ctrl);
	return err_code;
    }
    return 0;
}

queue_t * queue_new(unsigned int n, size_t size){
    queue_t * q = calloc(1, sizeof(queue_t));
    if(!q) return q;
    if(queue_init(q, n, size) != 0){
	free(q);
	return NULL;
    }
    return q;
}

void queue_free(queue_t * queue){
    rb_free(&(queue->rb));
    dctrl_free(&(queue->ctrl));
    free(queue);
}

