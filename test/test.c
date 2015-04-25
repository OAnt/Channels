#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include "../src/buffer.h"
#include "../src/channel.h"

void test_init_buffer(void){
    printf("%s: \n", __func__);
    buffer_t r_buf;
    unsigned int n = 10;
    buffer_init(&r_buf, n, sizeof(int), RING_BUFFER);
    assert(r_buf.buffer);
    assert(r_buf.n = 10);
    assert(r_buf.size = sizeof(int));
    assert(r_buf.used == 0 && r_buf.start == 0 && r_buf.end == 0);
    buffer_free(&r_buf);
    printf("OK\n");
}

void test_rb_write_success(void){
    printf("%s: \n", __func__);
    buffer_t rb;
    unsigned int i, n = 3;
    buffer_init(&rb, n, sizeof(int), RING_BUFFER);
    int value = 12;
    (void)value;
    assert(*(int*)rb.buffer == 0);
    assert(rb_available(&rb) == n);
    assert(rb_write(&rb, &value, 0));
    assert(rb_available(&rb) == n - 1);
    for(i = 0; i < n; i++)
	if(i == 0)
	    assert(*(int*)rb.buffer == value);
	else
	    assert(*(int*)(rb.buffer + i) == 0);
    buffer_free(&rb);
    printf("OK\n");
}

void test_rb_take_success(void){
    printf("%s: \n", __func__);
    buffer_t rb;
    unsigned int n = 3;
    buffer_init(&rb, n, sizeof(int), RING_BUFFER);
    int value = 12;
    (void)value;
    ((int*)rb.buffer)[0] = value;
    rb.start = 1;
    rb.used = 1;
    assert(rb_available(&rb) == n - 1);
    int result;
    (void)result;
    assert(rb_take(&rb, &result));
    assert(result == value);
    assert(rb_available(&rb) == n);
    assert(rb.start == 1);
    buffer_free(&rb);
    printf("OK\n");
}

void test_rb_write_take_full_failure(void){
    printf("%s: \n", __func__);
    buffer_t rb;
    unsigned int i, n = 3;
    buffer_init(&rb, n, sizeof(int), RING_BUFFER);
    for(i = 0; i < n + 1; i++)
	if(i < n)
	    assert(rb_write(&rb, &i, 0));
	else
	    assert(!rb_write(&rb, &i, 0));
    for(i = 0; i < n; i++)
	assert(((int*)rb.buffer)[i] == i);
    int res;
    (void)res;
    for(i = 0; i < n + 1; i++)
	if(i < n){
	    assert(rb_take(&rb, &res));
	    assert(res == i);
	}else
	    assert(!rb_take(&rb, &res));
    buffer_free(&rb);
    printf("OK\n");
}

void test_new_channel(void){
    printf("%s: \n", __func__);
    unsigned int i, n = 3;
    queue_t * q = queue_new(n, sizeof(int));
    for(i = 0; i < n; i++){
	queue_put(q, &i);
    }
    for(i = 0; i < n; i++){
	int res;
	queue_take(q, &res);
	assert(res == i);
    }
    queue_free(q);
    printf("OK\n");
}

typedef struct dummy_t{
    int i;
    int j;
}dummy_t;

typedef queue_t dummy_queue_t;

static inline dummy_queue_t * dummy_queue_new(unsigned int n){
    return queue_new(n, sizeof(dummy_t));
}

static inline void dummy_queue_put(dummy_queue_t * q, dummy_t d){
    queue_put(q, &d);
}

static inline dummy_t dummy_queue_take(dummy_queue_t * q){
    dummy_t d;
    queue_take(q, &d);
    return d;
}

void test_new_dummy_channel(void){
    printf("%s: \n", __func__);
    unsigned int i, n = 3;
    dummy_queue_t * q = dummy_queue_new(n);
    for(i = 0; i < n; i++){
	dummy_t d = {i, i};
	dummy_queue_put(q, d);
    }
    for(i = 0; i < n; i++){
	dummy_t comp = {i, i};
	dummy_t d = dummy_queue_take(q);
	(void)comp;
	(void)d;
	assert(comp.i == d.i && comp.j == d.j);
    }
    queue_free(q);
    printf("OK\n");
}

typedef struct test_thread_t{
    queue_t * iq, *oq;
    int state;
}test_thread_t;

void * _test_thread(void * data){
    test_thread_t * tt = (test_thread_t*)data;
    int i;
    while(1){
	queue_take(tt->iq, &i);
	if(i == -1) break;
	i += tt->state;
	queue_put(tt->oq, &i);
    }
    return NULL;
}

void test_threaded_take_put(void){
    printf("%s: \n", __func__);
    int n = 3;
    test_thread_t tt = {
	queue_new(n, sizeof(int)),
	queue_new(n, sizeof(int)), 
	10
    };
    int i;
    pthread_t tid;
    pthread_create(&tid, NULL, &_test_thread, &tt);
    for(i = 0; i < 2*n - 1; i++)
	queue_put(tt.iq, &i);
    for(i = 0; i < 2*n - 1; i++){
	int j;
	queue_take(tt.oq, &j);
	assert(i+tt.state == j);
    }
    i = -1;
    queue_put(tt.iq, &i);
    pthread_join(tid, NULL);
    queue_free(tt.iq);
    queue_free(tt.oq);
    printf("OK\n");
}

void test_take_put_timeouts(void){
    printf("%s: \n", __func__);
    int n = 3;
    queue_t * q = queue_new(n, sizeof(int));
    (void)q;
    int i;
    for(i = 0; i < n+1; i++)
	if(i < n)
	    assert(queue_timed_put(q, &i, 1) == 0);
	else
	    assert(queue_timed_put(q, &i, 1) == ETIMEDOUT);
    int j;
    (void)j;
    for(i = 0; i < n+1; i++)
	if(i < n)
	    assert(queue_timed_take(q, &j, 1) == 0);
	else
	    assert(queue_timed_take(q, &j, 1) == ETIMEDOUT);
    queue_free(q);
    printf("OK\n");
}

typedef struct {
    int p;
    int v;
}dummy_heap_t;

#define N 7

int comp_int(const void * a, const void * b){
    dummy_heap_t * da = (dummy_heap_t*)a;
    dummy_heap_t * db = (dummy_heap_t*)b;
    return da->p - db->p;
}

void test_hb_write_take(void){
    printf("%s: \n", __func__);
    buffer_t hb;
    heap_init(&hb, N, sizeof(int));
    dummy_heap_t dh[N] = {{1, 3}, {2, 4}, 
        {0, 4}, {5, 5}, {3, 8}, {6, 7}, {8, 9}};
    int i = 0;
    for(i = 0; i < N; i++)
        assert(hb_write(&hb, &dh[i].v, dh[i].p));
    assert(hb_write(&hb, &i, 99) == 0);
    qsort(dh, N, sizeof(dummy_heap_t), &comp_int); 
    for(i = 0; i < N; i++){
        int v;
        (void)v;
        assert(hb_take(&hb, &v));
        assert(dh[N-i-1].v == v);
    }
    assert(hb_take(&hb, &i) == 0);
    buffer_free(&hb);
    printf("OK\n");
}

void test_priority_queue_write_take(void){
    printf("%s: \n", __func__);
    priority_queue_t * q = priority_queue_new(N, sizeof(int));
    (void)q;
    dummy_heap_t dh[N] = {{1, 3}, {2, 4}, 
        {2, 4}, {1, 5}, {3, 8}, {6, 7}, {8, 9}};
    int i = 0;
    for(i = 0; i < N; i++)
        assert(priority_queue_timed_put(q, &dh[i].v, dh[i].p, 2) == 0);
    assert(priority_queue_timed_put(q, &i, 99, 1) == ETIMEDOUT);
    qsort(dh, N, sizeof(dummy_heap_t), comp_int); 
    for(i = 0; i < N; i++){
        int v;
        (void)v;
        assert(priority_queue_timed_take(q, &v, 2) == 0);
        assert(dh[N-i-1].v == v);
    }
    assert(priority_queue_timed_take(q, &i, 1) == ETIMEDOUT);
    priority_queue_free(q);
    printf("OK\n");
}

void _queue_set_not_empty_callback(queue_t * q, 
        void(*callback)(queue_t * q, void * data), 
        void * data);
void _queue_destroy_not_empty_callback(queue_t * q);

void dummy_callback(queue_t *q, void * data){
    (void)q;
    int * i = (int *) data;
    *i += 1;
}

void test_callback(void){
    printf("%s: \n", __func__);
    queue_t * q = queue_new(5, sizeof(int));
    int d = 0;
    _queue_set_not_empty_callback(q, dummy_callback, &d);
    queue_put(q, &d);
    queue_put(q, &d);
    queue_put(q, &d);
    assert(d == 3);
    _queue_destroy_not_empty_callback(q);
    queue_put(q, &d);
    queue_put(q, &d);
    assert(d == 3);
    queue_free(q);
    printf("OK\n");
}

typedef struct {
    int n;
    int ns;
    queue_t ** q;
    queue_t * ctrlq;
    pthread_mutex_t * mutex;
}thread_ctrl_t;

void * waiter_thread(void * data){
    thread_ctrl_t * tctrl = (thread_ctrl_t *) data;
    queue_t ** q = tctrl->q;
    queue_t * ctrlq = tctrl->ctrlq;
    int n = tctrl->n;
    int ns = tctrl->ns;
    int i = 0;
    queue_t * sq;
    while(i < ns){
        queue_t * ptr;
        queue_take(ctrlq, &ptr);
        queue_select_not_empty(q, n, &sq); 
        assert(ptr == sq);
        int d;
        queue_take(sq, &d);
        i++;
    }
    return NULL;
}

void test_select(void){
    printf("%s: \n", __func__);
    int i, n = N;
    queue_t * qarray[n];
    for(i=0; i < n; i++)
        qarray[i] = queue_new(3, sizeof(int));
    queue_t * ctrlq = queue_new(1, sizeof(queue_t *));
    thread_ctrl_t tctrl = {n, 3, qarray, ctrlq, PTHREAD_MUTEX_NORMAL};
    pthread_t pthread;
    pthread_create(&pthread, NULL, waiter_thread, &tctrl);
    srand(12345);
    for(i=0; i<tctrl.ns; i++){
        int q = rand() % n;
        queue_put(qarray[q], &i);
        queue_put(ctrlq, &qarray[q]);
    }
    void * zzz;
    pthread_join(pthread, &zzz);
    for(i=0; i < n; i++)
        queue_free(qarray[i]);
    queue_free(ctrlq);
    printf("OK\n");
}

int main(int argc, char ** argv){
    (void)argc;
    (void)argv;
    test_init_buffer();
    test_rb_write_success();
    test_rb_take_success();
    test_rb_write_take_full_failure();
    test_new_channel();
    test_new_dummy_channel();
    test_threaded_take_put();
    test_take_put_timeouts();
    test_hb_write_take();
    test_priority_queue_write_take();
    test_callback();
    test_select();
    return 0;
}
