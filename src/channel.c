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

static char * _notification_type_name[] = {
    "HEAD",
    "NODE",
    "DEAD"
};

typedef enum{
    HEAD = 0,
    NODE = 1,
    DEAD = 2,
}link_list_node_t;

struct notification_callback_st{
    void(*callback)(struct queue_st * q, void * data);
    void *data;
    link_list_node_t type;
    struct queue_st * q;
    struct notification_callback_st * n;
    struct notification_callback_st * p;
};

typedef struct data_control_st dctrl_t;
struct data_control_st {
    pthread_mutex_t mutex;
    pthread_cond_t empty;
    pthread_cond_t full;
    struct notification_callback_st * not_full_callback;
    struct notification_callback_st *  not_empty_callback;
};

struct queue_st {
    channel_type_t type;
    dctrl_t ctrl;
    buffer_t rb;
};

void queue_print(struct queue_st * q){
    printf("%p, %s\n", q, _channel_type_name[q->type]);
}

void notification_print(struct notification_callback_st * nc){
    printf("%p, %s\n", nc, _notification_type_name[nc->type]);
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
    struct notification_callback_st * nc = calloc(2, 
            sizeof(struct notification_callback_st));
    if(nc == NULL){
	pthread_mutex_destroy(&(dctrl->mutex));
	pthread_cond_destroy(&(dctrl->empty));
        pthread_cond_destroy(&(dctrl->full));
        return ENOMEM;
    }
    dctrl->not_full_callback = nc;
    dctrl->not_full_callback->type = HEAD;
    dctrl->not_empty_callback = nc + 1;
    dctrl->not_empty_callback->type = HEAD;
    return 0;
}

int dctrl_free(dctrl_t * dctrl){
    if(pthread_mutex_destroy(&(dctrl->mutex)))
        return 1;
    if(pthread_cond_destroy(&(dctrl->empty)))
        return 1;
    if(pthread_cond_destroy(&(dctrl->full)))
        return 1;
    free(dctrl->not_full_callback);
    return 0;
}

static inline int _queue_lock(queue_t * q){
    return pthread_mutex_lock(&q->ctrl.mutex);
}

// never supposed to be used just for test puposes
void __queue_lock(queue_t * q){
    _queue_lock(q);
}

static inline int _queue_unlock(queue_t * q){
    return pthread_mutex_unlock(&q->ctrl.mutex);
}

// never supposed to be used just for test puposes
void __queue_unlock(queue_t * q){
    _queue_unlock(q);
}

static inline void _append_callback(struct notification_callback_st ** d, 
        struct notification_callback_st * nc)
{
    if(*d){
        nc->type = NODE;
        nc->n = (*d)->n;
        if(nc->n) nc->n->p = nc;
        (*d)->n = nc;
    }else{
        nc->type = HEAD;
        nc->n = NULL;
    }   
    nc->p = *d;
}

void _remove_callback(struct notification_callback_st * nc){
    assert(nc->type != HEAD);
    struct notification_callback_st * n = nc->n;
    struct notification_callback_st * p = nc->p;
    if(p) p->n = n;
    if(n) n->p = p;
    nc->type = DEAD;
}

void _queue_append_not_empty_callback(struct queue_st * q, 
    struct notification_callback_st * nc)
{
    nc->q = q;
    _append_callback(&(q->ctrl.not_empty_callback), nc); 
}

void _queue_append_not_full_callback(struct queue_st * q,
        struct notification_callback_st * nc)
{
    nc->q = q;
    _append_callback(&(q->ctrl.not_full_callback), nc); 
}

static inline notification_callback_t * __queue_append_callback(
        struct queue_st * q,
        callback_t callback, void * data,
        void(*callback_setter)(struct queue_st * q,
            struct notification_callback_st * nc))
{
    struct notification_callback_st * nc = calloc(1, 
            sizeof(struct notification_callback_st));
    if(!nc) return nc;
    nc->callback = callback;
    nc->data = data;
    int err;
    if((err = _queue_lock(q)) != 0){
        free(nc);
        return NULL;
    }
    callback_setter(q, nc);
    _queue_unlock(q);
    return nc;
}

notification_callback_t * queue_append_not_empty_callback(
        struct queue_st * q, 
        callback_t callback, void * data)
{
    return __queue_append_callback(q, callback, data,
            _queue_append_not_empty_callback);
}

notification_callback_t * queue_append_not_full_callback(
        struct queue_st * q, 
        callback_t callback, void * data)
{
    return __queue_append_callback(q, callback, data,
            _queue_append_not_full_callback);
}

void queue_remove_callback(notification_callback_t * nc){
    _queue_lock(nc->q);
    _remove_callback(nc);
    _queue_unlock(nc->q);
    free(nc);
}

static inline void _queue_callback(queue_t * q, 
        struct notification_callback_st * nc)
{
    nc = nc->n;
    while(nc){
        if(nc->callback) nc->callback(q, nc->data);
        nc = nc->n;
    }
}

static inline int __notify_condition(pthread_cond_t * cond)
{
    if(pthread_cond_broadcast(cond) == 0){
        return 0;
    }else
        return EINVAL;
}

int notify_not_empty(queue_t * q) {
    return __notify_condition(&q->ctrl.empty);
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
    return __notify_condition(&q->ctrl.full);
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

typedef int (*mutex_lock_t)(pthread_mutex_t *);

int _queue_take(queue_t *queue, void * data, 
        struct timespec * abstime, buffer_take f)
{
    int err;
    if((err = pthread_mutex_lock(&(queue->ctrl.mutex))) != 0)
        return err;
    int res;
    while((res = f(&(queue->rb), data)) == 0){
        if((err = wait_empty(&(queue->ctrl), abstime)) != 0) {
	    pthread_mutex_unlock(&(queue->ctrl.mutex));
	    return err;
	}
    }
    notify_not_full(queue);
    _queue_callback(queue, queue->ctrl.not_full_callback);
    pthread_mutex_unlock(&(queue->ctrl.mutex));
    return 0;
}

int _queue_try_take(queue_t * q, void * data, 
        buffer_take f,
        mutex_lock_t mutex_lock)
{
    int err = 0;
    if((err = mutex_lock(&(q->ctrl.mutex))) != 0)
        return err;
    if(f(&(q->rb), data) == 0){
        err = EAGAIN;
        goto end_queue_try_take;
    }
    notify_not_full(q);
    _queue_callback(q, q->ctrl.not_full_callback);
end_queue_try_take:
    pthread_mutex_unlock(&(q->ctrl.mutex));
    return err;
}

int _queue_put(queue_t * queue, void * value, 
        struct timespec * abstime, buffer_write f, int priority)
{
    int err;
    if((err = pthread_mutex_lock(&(queue->ctrl.mutex))) != 0)
        return err;
    int res;
    while((res = f(&(queue->rb), value, priority)) == 0){
        if((err = wait_full(&(queue->ctrl), abstime)) != 0){
	    pthread_mutex_unlock(&(queue->ctrl.mutex));
	    return err;
	}
    }
    notify_not_empty(queue);
    _queue_callback(queue, queue->ctrl.not_empty_callback);
    pthread_mutex_unlock(&(queue->ctrl.mutex));
    return 0;
}

int _queue_try_put(queue_t * q, void * data, 
        buffer_write f, int priority, 
        mutex_lock_t mutex_lock){
    int err = 0;
    if((err = mutex_lock(&(q->ctrl.mutex))) != 0)
        return err;
    if(f(&(q->rb), data, priority) == 0){
        err = EAGAIN;
        goto end_queue_try_put;
    }
    notify_not_empty(q);
    _queue_callback(q, q->ctrl.not_empty_callback);
end_queue_try_put:
    pthread_mutex_unlock(&(q->ctrl.mutex));
    return err;
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

int queue_try_take(queue_t *q, void * data){
    assert(q->type == FIFO_CHANNEL);
    return _queue_try_take(q, data, rb_take, 
            pthread_mutex_trylock);
}

int queue_no_wait_take(queue_t * q, void * data){
    assert(q->type == FIFO_CHANNEL);
    return _queue_try_take(q, data, rb_take, 
            pthread_mutex_lock);
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

int queue_try_put(queue_t *q, void *data){
    assert(q->type == FIFO_CHANNEL);
    return _queue_try_put(q, data, rb_write, 0, 
            pthread_mutex_trylock);
}

int queue_no_wait_put(queue_t *q, void *data){
    assert(q->type == FIFO_CHANNEL);
    return _queue_try_put(q, data, rb_write, 0, 
            pthread_mutex_lock);
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

int priority_queue_try_take(priority_queue_t * q, void * data){
    assert(q->type == PRIORITY_CHANNEL);
    return _queue_try_take(q, data, hb_take, 
            pthread_mutex_trylock);
}

int priority_queue_no_wait_take(priority_queue_t * q, 
        void * data)
{
    assert(q->type == PRIORITY_CHANNEL);
    return _queue_try_take(q, data, hb_take, 
            pthread_mutex_lock);
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
    return _queue_put(q, data, NULL, 
            hb_write, priority);
}

int priority_queue_try_put(priority_queue_t *q, void *data, int priority){
    assert(q->type == PRIORITY_CHANNEL);
    return _queue_try_put(q, data, 
            hb_write, priority, 
            pthread_mutex_trylock);
}

int priority_queue_no_wait_put(priority_queue_t *q, 
        void *data, int priority)
{
    assert(q->type == PRIORITY_CHANNEL);
    return _queue_try_put(q, data, 
            hb_write, priority, 
            pthread_mutex_lock);
}

int priority_queue_timed_put(priority_queue_t * q, 
        void *data, int priority, unsigned int sec){
    assert(q->type == PRIORITY_CHANNEL);
    struct timespec ts;
    _gettimer(&ts, sec);
    return _queue_put(q, data, &ts, 
            hb_write, priority);
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

int _select(struct queue_st ** q, int n, 
        struct queue_st ** selected_queue, int * ns,
        void(*callback_setter)(struct queue_st*, struct notification_callback_st *),
        int peek_function(struct queue_st * q),
        struct timespec * ts){
    int i = 0;
    *ns = 0;
    int err = 0;
    for(i = 0; i < n; i++){
        err = _queue_lock(q[i]);
        if(err != 0){
            n = i;
            goto error_select;
        }
        if(peek_function(q[i]) > 0){
            //this queue already satisfy the condition
            //returning it
            selected_queue[*ns] = q[i];
            *ns += 1;
        }
    }
    if(*ns > 0) goto error_select;
    struct notification_callback_st * nc = calloc(n, 
            sizeof(struct notification_callback_st));
    if(nc == NULL){
        err = ENOMEM;
        goto error_select;
    }
    select_data_t sdata;
    if((err = select_data_init(&sdata)) != 0) return err;
    err = pthread_mutex_lock(&(sdata.mutex));
    if(err){
        select_data_destroy(&sdata);
        goto error_with_free_select;
    }
    for(i = 0; i < n; i++){
        struct notification_callback_st * n = nc + i;
        n->data = &sdata;
        n->callback = &__select_callback;
        callback_setter(q[i], n);
        _queue_unlock(q[i]);
    }
    if(ts) 
        err = pthread_cond_timedwait(&(sdata.cond), &(sdata.mutex), ts);
    else 
        err = pthread_cond_wait(&(sdata.cond), &(sdata.mutex));
    if(err != 0)
        goto end_select;
    *selected_queue = sdata.q;
    *ns = 1;
end_select:
    pthread_mutex_unlock(&(sdata.mutex));
    for(i = 0; i < n; i++){
        _queue_lock(q[i]);
        struct notification_callback_st * n = nc + i;
        _remove_callback(n);
        _queue_unlock(q[i]);
    }
    free(nc);
    select_data_destroy(&sdata);
    return err;
error_with_free_select:
    free(nc);
error_select:
    for(i = 0; i < n; i++)
        _queue_unlock(q[i]);
    return err;
}

int queue_select_not_full(struct queue_st ** q, int n, 
        struct queue_st ** selected_queue, int * ns)
{
    return _select(q, n, selected_queue, ns,
        _queue_append_not_full_callback, 
        _queue_peek_available,
        NULL);
}

int queue_timed_select_not_full(struct queue_st ** q, int n,
        struct queue_st ** selected_queue, int *ns,
        unsigned int s)
{
    struct timespec ts;
    _gettimer(&ts, s);
    return _select(q, n, selected_queue, ns,
        _queue_append_not_full_callback, 
        _queue_peek_available,
        &ts);
}

int queue_select_not_empty(struct queue_st ** q, int n,
        struct queue_st ** selected_queue, int * ns)
{
    return _select(q, n, selected_queue, ns,
            _queue_append_not_empty_callback,
            _queue_peek_used,
            NULL);
}

int queue_timed_select_not_empty(struct queue_st ** q, int n,
        struct queue_st ** selected_queue, int * ns,
        unsigned int s)
{
    struct timespec ts;
    _gettimer(&ts, s);
    return _select(q, n, selected_queue, ns,
            _queue_append_not_empty_callback,
            _queue_peek_used,
            &ts);
}

