#include "buffer.h"

int rb_init_ring_buffer(ring_buffer_t * r_buf, 
	unsigned int n, size_t size){
    calloc_(r_buf->buffer, n, size);
    r_buf->n = n;
    r_buf->size = size;
    r_buf->start = 0;
    r_buf->end = 0;
    r_buf->used = 0;
    return 0;
}

int rb_write(ring_buffer_t * rb, void * data){
    if(rb_available(rb) > 0) {
	memmove(rb->buffer + rb->start*rb->size,
		data, rb->size);
        rb->used += 1;
        rb->start = (rb->start + 1) % rb->n;
        return 1;
    }
    return 0;
}

int rb_take(ring_buffer_t * rb, void * data){
    if(rb_has_next(rb) > 0){
        int end = rb->end;
        rb->end = (rb->end + 1) % rb->n;
        rb->used -= 1;
	memmove(data, rb->buffer + end*rb->size,
		rb->size);
        return 1;
    }
    return 0;
}

void rb_free(ring_buffer_t * rb){
    free(rb->buffer);
    rb->buffer = NULL;
    rb->size = 0;
    rb->n = 0;
    rb->start = 0;
    rb->end = 0;
}

