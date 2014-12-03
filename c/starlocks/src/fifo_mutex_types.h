/* 
 * Type definitions for the FIFO Mutex. Contains only definitions 
 * relevant to external use of the FIFO Mutex.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 10095183
 */ 

#ifndef _FIFO_MUTEX_TYPES_H_
#define _FIFO_MUTEX_TYPES_H_

#include <pthread.h>
#include "check.h"
#include "queue.h"

typedef struct fifo_mutex {
    queue_t queue;                  /* Waiting tasks */
    pthread_mutex_t mutex;          /* The actual FIFO mutex */
} fifo_mutex_t;

/* Static Initializers */
#define FIFO_MUTEX_INITIALIZER(name) \
    { INIT_QUEUE_HEAD, PTHREAD_MUTEX_INITIALIZER }

#define INIT_FIFO_MUTEX(name) \
    name = FIFO_MUTEX_INITIALIZER(name)

/* Dynamic Initializer */
static inline int fifo_mutex_init(fifo_mutex_t *fm)
{
    int ret = 1;
    check(!fm, out);
    ret = 0;
    init_queue_head(&fm->queue);
    ret = pthread_mutex_init(&fm->mutex, NULL);
out:
    return ret;
}

#endif /* _FIFO_MUTEX_TYPES_H_ */

