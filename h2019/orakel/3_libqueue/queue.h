#ifndef QUEUE_H
#define QUEUE_H

#include <sys/types.h>
#include "list.h"

#define MAX_QUEUE_SIZE 16

struct queue_entry {
    struct list_head next;
    void *           data;
};

struct queue {
    struct list_head head;
    size_t           length;
};

struct  queue* queue_create(void);
ssize_t queue_length(struct queue * q);
int     queue_is_empty(struct queue * q);
int     queue_is_full(struct queue * q);
struct  queue_entry * entry_create(void * data);
int     entry_destroy(struct queue_entry * entry);
int     queue_head_push(struct queue * q, void * data);
void *  queue_head_pop(struct queue * q);
void *  queue_head_peek(struct queue * q);
int     queue_tail_push(struct queue * p, void * data);
void *  queue_tail_pop(struct queue * q);
int     queue_flush(struct queue * q);

#endif
