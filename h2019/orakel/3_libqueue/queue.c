#include <stdlib.h>

#include "list.h"
#include "queue.h"

struct queue_entry {
    struct list_head next;
    void *           data;
};

struct queue {
    struct list_head head;
    size_t           length;
};


struct queue* queue_create(void)
{
    struct queue *q = (struct queue *)malloc(sizeof(struct queue));
    if (!q)
        return NULL;

    INIT_LIST_HEAD(&q->head);
    q->length = 0;

    return q;
}

ssize_t queue_length(struct queue * q)
{
    if (!q)
        return -2;

    return q->length;
}

struct queue_entry * entry_create(void * data)
{
    struct queue_entry * entry = (struct queue_entry *)malloc(sizeof(struct queue_entry));
    if (!entry)
        return NULL;

    entry->data = data;
    INIT_LIST_HEAD(&entry->next);

    return entry;
}

int queue_is_empty(struct queue * q)
{
    if (!q) {
        perror("Can't chek the emptiness of a NULL queue");
        return -2;
    }

    return list_empty(&q->head) ? 1 : -1;
}

int queue_is_full(struct queue * q)
{
    if (!q) {
        perror("Can't check the fullness of a NULL queue");
        return -2;
    }

    if(queue_length(q) == MAX_QUEUE_SIZE)
        return 1;
    else
        return -1;
}

int entry_destroy(struct queue_entry * entry)
{
    if (!entry)
        return -1;

    free(entry);

    return 0;
}

int queue_head_push(struct queue * q, void * data)
{
    struct queue_entry * entry;

    if (!q) {
        perror("Cannot head-push on a NULL queue");
        return -1;
    }

    if (queue_is_full(q) == 1) {
        printf("[WARNING]: Cannot head-push on a FULL queue\n");
        return -1;
    }

    entry = entry_create(data);
    if (!entry)
        return -1;

    list_add(&entry->next, &q->head);
    q->length++;

    printf("[DEBUG]: Entry %p head-pushed into queue %p (length = %zd)\n", entry, q, q->length);

    return 0;
}

void * queue_head_pop(struct queue * q)
{
    struct queue_entry * entry;
    void *               data;

    if (!q) {
        perror("Cannot head-pop from a NULL queue");
        return NULL;
    }

    if (list_empty(&q->head)) {
        printf("[ERROR]: queue %p is empty, can't head-pop\n", q);
        return NULL;
    }

    entry = list_first_entry(&q->head, struct queue_entry, next);

    data = entry->data;

    list_del(&entry->next);
    q->length--;

    printf("[DEBUG]: Entry %p head-popped from queue %p (length = %zd)\n", entry, q, q->length);

    entry_destroy(entry);

    return data;
}


int queue_tail_push(struct queue * q, void * data)
{
    struct queue_entry * entry;

    if (!q) {
        perror("Cannot tail-push on a NULL queue");
        return -1;
    }

    if (queue_is_full(q) == 1) {
        printf("[WARNING]: Cannot head-push on a FULL queue\n");
        return -1;
    }

    entry = entry_create(data);
    if (!entry)
        return -1;

    list_add_tail(&entry->next, &q->head);
    q->length++;

    printf("[DEBUG]: Entry %p tail-pushed into queue %p (length = %zd)\n",
            entry, q, q->length);

    return 0;
}

void * queue_tail_pop(struct queue * q)
{
    struct queue_entry * entry;
    void *               data;

    if (!q) {
        perror("Cannot tail-pop from a NULL queue");
        return NULL;
    }

    if (list_empty(&q->head)) {
        printf("[ERROR]: queue %p is empty, can't tail-pop\n", q);
        return NULL;
    }

    entry = ((struct queue_entry *)(list_last_entry(&q->head, struct queue_entry, next)));

    data = entry->data;

    list_del(&entry->next);
    q->length--;

    printf("[DEBUG]: Entry %p tail-popped from queue %p (length = %zd)\n", entry, q, q->length);

    entry_destroy(entry);

    return data;
}


void * queue_head_peek(struct queue * q)
{
    struct queue_entry * entry;
    void *                data;

    if (!q) {
        perror("Cannot head-pop from a NULL queue");
        return NULL;
    }

    if (list_empty(&q->head)) {
        printf("[ERROR]: queue %p is empty, can't head-pop\n", q);
        return NULL;
    }

    entry = ((struct queue_entry *)(list_first_entry(&q->head, struct queue_entry, next)));

    data = entry->data;

    printf("[DEBUG]: Entry %p head-peeked from queue %p (length = %zd)\n", entry, q, q->length);

    return data;
}


int queue_flush(struct queue * q)
{
    struct queue_entry * pos, * nxt;

    list_for_each_entry_safe(pos, nxt, &q->head, next) {
        list_del(&pos->next);
        entry_destroy(pos);

        q->length--;
    }
    
    free(q);
    
    printf("[DEBUG]: Queue destroyed successfully!\n");

    return 0;
}
