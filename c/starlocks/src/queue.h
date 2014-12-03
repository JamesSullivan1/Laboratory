/* 
 * Doubly-linked queue implementation. 
 *
 * Each node can contain an arbitrary sized payload, the size
 * of which is specified by using node_alloc(node, data_type).
 * Generally you don't want to mix data types in lists.
 *
 * Best practice is to use the node_data(node, data_type) accessor
 * which will implicitly cast to the desired data type.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 10095183 
 */

#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdlib.h>
#include <pthread.h>

/* Definition of the node_t type. The char[0] trick is used to
 * allow for flexible data types, you malloc(sizeof(node) + 
 * sizeof(payload_t)) and then use data as the ref. */
typedef struct node_t{
    struct node_t *next;            /* Next element of the queue */
    struct node_t *prev;            /* Element before this one */
    char data[0];                   /* Reference to stored data */ 
} node_t;

/* Allocates num nodes with enough room for the data_t type */
#define node_alloc(data_t) \
    malloc(sizeof(node_t) + sizeof(data_t))

/* Casts the node's data to the desired type */
#define node_data(node, data_t) (data_t)((node)->data)

/* Head of a doubly linked queue. */
typedef struct queue_t {
    struct node_t *front;       /* Front of the queue */
    struct node_t *back;        /* End of the queue */
    pthread_mutex_t mutex;      /* Modification mutex */
} queue_t;

/* Static initializers */

#define QUEUE_HEAD_INIT(name)    \
    { NULL, NULL, PTHREAD_MUTEX_INITIALIZER }

#define QUEUE_HEAD(name)         \
    queue_t name = QUEUE_HEAD_INIT(name)

/* Dynamic initializer */
static inline void init_queue_head(queue_t *queue)
{
    queue->front = NULL;
    queue->back  = NULL;
    pthread_mutex_init(&queue->mutex, NULL);
}

/* Returns true if the queue is nonempty. */
static inline int queue_empty(queue_t *queue)
{
    return queue->front == NULL;
}

/* Add a new element to the end of the queue. */
static inline void queue_add_tail(node_t *new, queue_t *queue)
{
    /* Set up the new node's references */
    new->next = NULL;
    new->prev = queue->back; 
    /* Make new the queue's tail */
    if(queue->back)
        queue->back->next = new;
    queue->back = new;
    /* If there wasn't a queue, set new to be it */
    if(!queue->front)
        queue->front = new;
}

/* Remove an element from the front of the queue. 
 *
 * Assumes the list has at least one thing to remove; use with
 * list_empty() for safety.
 */
static inline void queue_remove_head(queue_t *queue)
{
    /* Move the reference back (possibly to NULL) */
    queue->front = queue->front->next;
    /* Remove the old reference */
    if(queue->front)
        queue->front->prev = NULL;
    else
        queue->back = NULL;
}

#endif /* _QUEUE_H_ */

