#include <assert.h>
#include "../src/buffer.h"

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

int main(int argc, char ** argv){
    (void)argc;
    (void)argv;
    test_init_ring_buffer();
    test_rb_write_success();
    test_rb_take_success();
    test_rb_write_take_full_failure();
    return 0;
}
