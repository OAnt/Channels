#ifndef BUFFER_H
#define BUFFER_H
#include <string.h>
#include "common.h"

typedef struct ring_buffer_st {
    void * buffer;
    unsigned int n;
    size_t size;
    unsigned int used;
    unsigned int start;
    unsigned int end;
} ring_buffer_t;

int rb_init_ring_buffer(ring_buffer_t * r_buf, unsigned int n,
	size_t size);
int rb_write(ring_buffer_t * rb, void * data);
int rb_take(ring_buffer_t * rb, void * data);
void rb_free(ring_buffer_t * rb);

#define rb_has_next(B) ((B)->used)
#define rb_available(B) ((B)->n - rb_has_next((B)))

#endif

