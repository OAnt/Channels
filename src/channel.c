#include <pthread.h>
#include <sys/time.h>
#include <assert.h>
#include <stdio.h>

#include "channel.h"
#include "buffer.h"

static char * _channel_type_name[] = {
    "FIFO_CHANNEL",
    "PRIORITY_CHANNEL"
};

typedef enum{
    FIFO_CHANNEL = 0,
    PRIORITY_CHANNEL = 1,
}channel_type_t;

typedef void(*callback_t)(queue_t * q, void * data);

typedef struct{
    callback_t callback;
    void *data;
    int set;
}notification_callback_t;

typedef struct data_control_st dctrl_t;
struct data_control_st {
    pthread_mutex_t mutex;
    pthread_cond_t empty;
    pthread_cond_t full;
    notification_callback_t not_full_callback;
    notification_callback_t not_empty_callback;
};

struct queue_st {
    channel_type_t type;
    dctrl_t ctrl;
    buffer_t rb;
};

void queue_print(queue_t * q){
    printf("%p, %s\n", q, _channel_type_name[q->type]);
}

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
    dctrl->not_empty_callback.set = 0;
    dctrl->not_full_callback.set = 0;
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

static inline void __set_callback(notification_callback_t * nc, 
        callback_t c,
        void * d)
{
    assert(nc->set == 0);
    nc->callback = c;
    nc->data = d;
    nc->set = 1;
}

void _queue_set_not_empty_callback(queue_t * q, callback_t callback,
        void * data)
{
    pthread_mutex_lock(&(q->ctrl.mutex));
    __set_callback(&(q->ctrl.not_empty_callback), callback, data); 
    pthread_mutex_unlock(&(q->ctrl.mutex));
}

void _queue_set_not_full_callback(queue_t * q, callback_t callback,
        void * data)
{
    pthread_mutex_lock(&(q->ctrl.mutex));
    __set_callback(&(q->ctrl.not_full_callback), callback, data); 
    pthread_mutex_unlock(&(q->ctrl.mutex));
}

void _queue_destroy_not_empty_callback(queue_t * q){
    pthread_mutex_lock(&(q->ctrl.mutex));
    q->ctrl.not_empty_callback.set = 0;
    pthread_mutex_unlock(&(q->ctrl.mutex));
}

void _queue_destroy_not_full_callback(queue_t * q){
    pthread_mutex_lock(&(q->ctrl.mutex));
    q->ctrl.not_full_callback.set = 0;
    pthread_mutex_unlock(&(q->ctrl.mutex));
}

static inline int __notify_condition(queue_t * q,
        pthread_cond_t * cond, 
        notification_callback_t * nc)
{
    if(pthread_cond_broadcast(cond) == 0){
        if(nc->set)
            nc->callback(q, nc->data);
        return 0;
    }else
        return EINVAL;
}

int notify_not_empty(queue_t * q) {
    return __notify_condition(q, &q->ctrl.empty, 
            &q->ctrl.not_empty_callback);
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

int notify_not_full(queue_t * q) {
    return __notify_condition(q, &q->ctrl.full, 
            &q->ctrl.not_full_callback);
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
    notify_not_full(queue);
    pthread_mutex_unlock(&(queue->ctrl.mutex));
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
    notify_not_empty(queue);
    pthread_mutex_unlock(&(queue->ctrl.mutex));
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

int _queue_peek_used(queue_t * q){
    return rb_has_next(&(q->rb));
}

int _queue_peek_available(queue_t * q){
    return rb_available(&(q->rb));
}

typedef struct{
    queue_t * q;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
}select_data_t;

int select_data_init(select_data_t * sdata){
    sdata->q = NULL;
    int err_code;
    if((err_code = pthread_mutex_init(&(sdata->mutex), NULL)))
        return err_code;
    if((err_code = pthread_cond_init(&(sdata->cond), NULL))){
	pthread_mutex_destroy(&(sdata->mutex));
        return err_code;
    }
    return 0;
}

int select_data_destroy(select_data_t * sdata){
    if(pthread_cond_destroy(&sdata->cond) ||
            pthread_mutex_destroy(&sdata->mutex))
        return 1;
    else
        return 0;
}

void __select_callback(queue_t * q, void * data){
    select_data_t * sdata = (select_data_t*)data;
    pthread_mutex_lock(&(sdata->mutex));
    sdata->q = q;
    pthread_cond_broadcast(&(sdata->cond));
    pthread_mutex_unlock(&(sdata->mutex));
}

int _select(queue_t ** q, int n, queue_t ** selected_queue, int * ns,
        void(*callback_setter)(queue_t*, callback_t, void *),
        void(*callback_destroyer)(queue_t *q),
        int peek_function(queue_t * q),
        struct timespec * ts){
    int i = 0;
    for(i = 0; i < n; i++){
        if(peek_function(q[i]) > 0){
            //this queue already satisfy the condition
            //returning it
            selected_queue[*ns] = q[i];
            *ns += 1;
        }
    }
    if(*ns > 0) return 0;
    int err;
    select_data_t sdata;
    if((err = select_data_init(&sdata)) != 0) return err;
    err = pthread_mutex_lock(&(sdata.mutex));
    if(err){
        select_data_destroy(&sdata);
        return err;
    }
    for(i = 0; i < n;i ++)
        callback_setter(q[i], &__select_callback, &sdata);
    if(ts) 
        err = pthread_cond_timedwait(&(sdata.cond), &(sdata.mutex), ts);
    else 
        err = pthread_cond_wait(&(sdata.cond), &(sdata.mutex));
    if(err != 0){
        pthread_mutex_unlock(&(sdata.mutex));
        select_data_destroy(&sdata);
        return err;
    }
    *selected_queue = sdata.q;
    *ns = 1;
    pthread_mutex_unlock(&(sdata.mutex));
    for(i = 0; i < n; i++)
        callback_destroyer(q[i]);
    select_data_destroy(&sdata);
    return 0;
}

int queue_select_not_full(queue_t ** q, int n, 
        queue_t ** selected_queue, int * ns)
{
    return _select(q, n, selected_queue, ns,
        _queue_set_not_full_callback, 
        _queue_destroy_not_full_callback, 
        _queue_peek_available,
        NULL);
}


int queue_select_not_empty(queue_t ** q, int n,
        queue_t ** selected_queue, int * ns)
{
    return _select(q, n, selected_queue, ns,
            _queue_set_not_empty_callback,
            _queue_destroy_not_empty_callback, 
            _queue_peek_used,
            NULL);
}
