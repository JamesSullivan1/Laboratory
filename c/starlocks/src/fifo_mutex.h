/* 
 * First-In, First-Out Mutex data type.
 *
 * Exactly one thread may hold the FIFO mutex at once. Contending
 * threads are given the mutex in a fair queue order, which is
 * enforced by each node waiting for its preceding node to signal
 * that progress can be made (which is done only when the queued
 * thread releases the mutex and thus the next thread in line is
 * the new front element). 
 *
 * Because dynamic allocation is used to maintain this queue, this
 * mutex is not suitable for use in signal handlers.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 10095183 
 */

#ifndef _FIFO_MUTEX_H_
#define _FIFO_MUTEX_H_

#include <pthread.h>
#include "fifo_mutex_types.h"
#include "check.h"
#include "queue.h"

typedef struct fifo_mutex_node {
    pthread_cond_t cond;            /* Cond for next thread to wait on*/
    pthread_mutex_t mutex;          /* Lock for the node cond var */
} fifo_mutex_node_t;

/* Dynamic Initializer */
static inline int fifo_mutex_node_init(fifo_mutex_node_t *fm_node)
{
    int ret = 1;
    check(!fm_node, out);
    ret = 0;
    ret |= pthread_cond_init(&fm_node->cond, NULL);
    ret |= pthread_mutex_init(&fm_node->mutex, NULL);
out:
    return ret;
}

/* 
 * If the queue is empty, immediately acquire its lock and return.
 *
 * Otherwise, enter the wait queue, and block until the preceding node
 *  unlocks and wakes us up. At this point, we acquire the lock. 
 *
 * Returns 0 on success and 1 on failure.
 */
static int fifo_mutex_lock(fifo_mutex_t *fm)
{
    int ret = 1;
    node_t *node;
    fifo_mutex_node_t *new;
    check(!fm, out);

    /* Instantiate a new node for this thread */
    node = node_alloc(fifo_mutex_node_t);
    check(!node, out);
    new = node_data(node, fifo_mutex_node_t *);
    fifo_mutex_node_init(new);

    /* Add it to the list */
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    pthread_mutex_lock(&fm->queue.mutex);
    queue_add_tail(node, &fm->queue);
    pthread_mutex_unlock(&fm->queue.mutex);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_testcancel();

    /* Wait until we're at the front of the queue */
    pthread_mutex_lock(&new->mutex);
    if(node->prev)
        pthread_cond_wait(&new->cond, &new->mutex);
    pthread_mutex_unlock(&new->mutex);

    /* Take the resource lock */
    pthread_mutex_lock(&fm->mutex);
out:
    return ret;
}

/* 
 * Exit the queue, deallocating the node and signalling the next-in-line
 *  task to wake up and acquire the lock.
 *
 * Returns 0 on success and 1 on failure.
 */
static int fifo_mutex_unlock(fifo_mutex_t *fm)
{
    int ret = 1;
    node_t *node;
    fifo_mutex_node_t *next;
    check(!fm, out);

    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
    pthread_mutex_lock(&fm->queue.mutex);
    node = fm->queue.front;
    check(!node, unlock);

    /* Release the resource lock */
    pthread_mutex_unlock(&fm->mutex);

    /* Take us off the list */
    queue_remove_head(&fm->queue);
    /* If there was a waiting element, wake them up */
    if(node->next) {
        next = node_data(node->next, fifo_mutex_node_t *);
        pthread_mutex_lock(&next->mutex);
        pthread_cond_broadcast(&next->cond);
        pthread_mutex_unlock(&next->mutex);
    }
    free(node);

unlock:
    pthread_mutex_unlock(&fm->queue.mutex);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_testcancel();
out:
    return ret;
}

#endif /* _FIFO_MUTEX_H_ */

