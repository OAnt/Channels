#include <pthread.h>

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

int wait_empty(dctrl_t * dctrl){
    return pthread_cond_wait(&(dctrl->empty),
                             &(dctrl->mutex));
}

int notify_not_full(dctrl_t * dctrl) {
    return pthread_cond_broadcast(&(dctrl->full));
}

int wait_full(dctrl_t * dctrl){
    return pthread_cond_wait(&(dctrl->full),
                             &(dctrl->mutex));
}

void * queue_take(queue_t *queue, void * data){
    pthread_mutex_lock(&(queue->ctrl.mutex));
    int res;
    while((res = rb_take(&(queue->rb), data)) == 0){
        wait_empty(&(queue->ctrl));
    }
    pthread_mutex_unlock(&(queue->ctrl.mutex));
    notify_not_full(&(queue->ctrl));
    return data;
}

void queue_put(queue_t * queue, void * value){
    pthread_mutex_lock(&(queue->ctrl.mutex));
    int res;
    while((res = rb_write(&(queue->rb), value)) == 0){
        wait_full(&(queue->ctrl));
    }
    pthread_mutex_unlock(&(queue->ctrl.mutex));
    notify_not_empty(&(queue->ctrl));
}

int queue_init(queue_t * queue, unsigned int n, size_t size){
    int err_code;
    if((err_code = rb_init_ring_buffer(&(queue->rb), n, size)) != 0) return err_code;
    return init_dctrl(&(queue->ctrl));
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

