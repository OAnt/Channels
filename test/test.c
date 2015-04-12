#include <assert.h>
#include <pthread.h>
#include <errno.h>
#include "../src/buffer.h"
#include "../src/channel.h"

void test_init_ring_buffer(void){
    ring_buffer_t r_buf;
    unsigned int n = 10;
    rb_init_ring_buffer(&r_buf, n, sizeof(int));
    assert(r_buf.buffer);
    assert(r_buf.n = 10);
    assert(r_buf.size = sizeof(int));
    assert(r_buf.used == 0 && r_buf.start == 0 && r_buf.end == 0);
    rb_free(&r_buf);
}

void test_rb_write_success(void){
    ring_buffer_t rb;
    unsigned int n = 3;
    rb_init_ring_buffer(&rb, n, sizeof(int));
    int value = 12;
    (void)value;
    assert(*(int*)rb.buffer == 0);
    assert(rb_available(&rb) == n);
    assert(rb_write(&rb, &value));
    assert(rb_available(&rb) == n - 1);
    for(unsigned int i = 0; i < n; i++)
	if(i == 0)
	    assert(*(int*)rb.buffer == value);
	else
	    assert(*(int*)(rb.buffer + i) == 0);
    rb_free(&rb);
}

void test_rb_take_success(void){
    ring_buffer_t rb;
    unsigned int n = 3;
    rb_init_ring_buffer(&rb, n, sizeof(int));
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
    rb_free(&rb);
}

void test_rb_write_take_full_failure(void){
    ring_buffer_t rb;
    unsigned int n = 3;
    rb_init_ring_buffer(&rb, n, sizeof(int));
    for(unsigned int i = 0; i < n + 1; i++)
	if(i < n)
	    assert(rb_write(&rb, &i));
	else
	    assert(!rb_write(&rb, &i));
    for(unsigned int i = 0; i < n; i++)
	assert(((int*)rb.buffer)[i] == i);
    int res;
    (void)res;
    for(unsigned int i = 0; i < n + 1; i++)
	if(i < n){
	    assert(rb_take(&rb, &res));
	    assert(res == i);
	}else
	    assert(!rb_take(&rb, &res));
    rb_free(&rb);
}

void test_new_channel(void){
    unsigned int n = 3;
    queue_t * q = queue_new(n, sizeof(int));
    for(unsigned int i = 0; i < n; i++){
	queue_put(q, &i);
    }
    for(unsigned int i = 0; i < n; i++){
	int res;
	queue_take(q, &res);
	assert(res == i);
    }
    queue_free(q);
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
    unsigned int n = 3;
    dummy_queue_t * q = dummy_queue_new(n);
    for(unsigned int i = 0; i < n; i++){
	dummy_t d = {i, i};
	dummy_queue_put(q, d);
    }
    for(unsigned int i = 0; i < n; i++){
	dummy_t comp = {i, i};
	dummy_t d = dummy_queue_take(q);
	(void)comp;
	(void)d;
	assert(comp.i == d.i && comp.j == d.j);
    }
    queue_free(q);
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
    int n = 3;
    test_thread_t tt = {
	queue_new(n, sizeof(int)),
	queue_new(n, sizeof(int)), 
	10
    };
    pthread_t tid;
    pthread_create(&tid, NULL, &_test_thread, &tt);
    for(int i = 0; i < 2*n - 1; i++)
	queue_put(tt.iq, &i);
    for(int i = 0; i < 2*n - 1; i++){
	int j;
	queue_take(tt.oq, &j);
	assert(i+tt.state == j);
    }
    int i = -1;
    queue_put(tt.iq, &i);
    pthread_join(tid, NULL);
    queue_free(tt.iq);
    queue_free(tt.oq);
}

void test_take_put_timeouts(void){
    int n = 3;
    queue_t * q = queue_new(n, sizeof(int));
    (void)q;
    for(int i = 0; i < n+1; i++)
	if(i < n)
	    assert(queue_timed_put(q, &i, 1) == 0);
	else
	    assert(queue_timed_put(q, &i, 1) == ETIMEDOUT);
    int j;
    (void)j;
    for(int i = 0; i < n+1; i++)
	if(i < n)
	    assert(queue_timed_take(q, &j, 1) == 0);
	else
	    assert(queue_timed_take(q, &j, 1) == ETIMEDOUT);
    queue_free(q);
}

int main(int argc, char ** argv){
    (void)argc;
    (void)argv;
    test_init_ring_buffer();
    test_rb_write_success();
    test_rb_take_success();
    test_rb_write_take_full_failure();
    test_new_channel();
    test_new_dummy_channel();
    test_threaded_take_put();
    test_take_put_timeouts();
    return 0;
}
