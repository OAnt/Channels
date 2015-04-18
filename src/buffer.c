#include <assert.h>
#include "buffer.h"

typedef struct {
    int priority;
    char value[];
}heap_node_t;

int buffer_init(buffer_t * r_buf, 
	unsigned int n, size_t size,
        buffer_type_t type){
    calloc_(r_buf->buffer, n, size);
    r_buf->type = type;
    r_buf->n = n;
    r_buf->size = size;
    r_buf->start = 0;
    r_buf->end = 0;
    r_buf->used = 0;
    return 0;
}

int rb_write(buffer_t * rb, void * data){
    assert(rb->type == RING_BUFFER);
    if(rb_available(rb) > 0) {
	memmove(rb->buffer + rb->start*rb->size,
		data, rb->size);
        rb->used += 1;
        rb->start = (rb->start + 1) % rb->n;
        return 1;
    }
    return 0;
}

int rb_take(buffer_t * rb, void * data){
    assert(rb->type == RING_BUFFER);
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

void buffer_free(buffer_t * rb){
    free(rb->buffer);
    rb->buffer = NULL;
    rb->type = FREED;
    rb->size = 0;
    rb->n = 0;
    rb->start = 0;
    rb->end = 0;
}

#define heap_get(hb, i) &(hb)->buffer[(i)*(hb)->size]

int heap_init(buffer_t * buf, unsigned int n,
        size_t size)
{
    return buffer_init(buf, n, 
            size + sizeof(heap_node_t), 
            HEAP_BUFFER);
}

void __hb_swap(buffer_t * hb, heap_node_t * n1,
        heap_node_t * n2)
{
    heap_node_t * tmp_n = alloca(hb->size);
    memmove(tmp_n, n2, hb->size);
    memmove(n2, n1, hb->size);
    memmove(n1, tmp_n, hb->size);
}

int hb_write(buffer_t * hb, void * data, int priority){
    assert(hb->type == HEAP_BUFFER);
    if(rb_available(hb) > 0){
        heap_node_t * n = (heap_node_t*)heap_get(hb, hb->used);
        n->priority = priority;
        memmove(n->value, data, hb->size - sizeof(int));
        int i = hb->used;
        hb->used++;
        while(i > 0){
            int j = (i - 1)/2;
            heap_node_t * pn = (heap_node_t*)heap_get(hb, j);
            if(n->priority > pn->priority){
                __hb_swap(hb, n, pn);
                i = j;
                n = pn;
            }else
                break;
        }
        return 1;
    }
    return 0;
}

int hb_take(buffer_t * hb, void * data){
    assert(hb->type == HEAP_BUFFER);
    if(rb_has_next(hb) > 0){
        heap_node_t * n = (heap_node_t*)heap_get(hb, 0);
        memmove(data, n->value, hb->size - sizeof(int));
        hb->used--;
        heap_node_t * ln = (heap_node_t*)heap_get(hb, hb->used);
        __hb_swap(hb, ln, n);
        unsigned int i = 0;
        while(1){
            unsigned int j = 2*i + 1;
            unsigned int k = 2*i + 2;
            heap_node_t * sn;
            if(j < hb->used && k < hb->used){
                int pj, pk;
                pj = *(int*)heap_get(hb, j);
                pk = *(int*)heap_get(hb, k);
                if(pj > pk){
                    sn = (heap_node_t*)heap_get(hb, j);
                    i = j;
                }else{
                    sn = (heap_node_t*)heap_get(hb, k);
                    i = k;
                }
            }else if(j < hb->used){
                sn = (heap_node_t*)heap_get(hb, j);
                i = j;
            }else if (k < hb->used){
                sn = (heap_node_t*)heap_get(hb, k);
                i = k;
            }else
                break;
            if(n->priority < sn->priority){
                __hb_swap(hb, n, sn); 
                n = sn;
            }else
                break;
        }
        return 1;
    }
    return 0;
}

