#ifndef BUFFER_H
#define BUFFER_H
#include <string.h>
#include "common.h"

typedef enum {
    FREED,
    RING_BUFFER,
    HEAP_BUFFER
}buffer_type_t;

typedef struct ring_buffer_st {
    char * buffer;
    size_t size;
    unsigned int n;
    buffer_type_t type;
    unsigned int used;
    unsigned int start;
    unsigned int end;
} buffer_t;

int buffer_init(buffer_t * r_buf, unsigned int n,
	size_t size, buffer_type_t type);
void buffer_free(buffer_t * rb);
int rb_write(buffer_t * rb, void * data);
int rb_take(buffer_t * rb, void * data);
int hb_write(buffer_t * hb, void * data, int priority);
int hb_take(buffer_t * hb, void * data);
int heap_init(buffer_t * buf, unsigned int n, size_t size);

#define rb_has_next(B) ((B)->used)
#define rb_available(B) ((B)->n - rb_has_next((B)))

#endif

