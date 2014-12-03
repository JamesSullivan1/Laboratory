/* 
 * Count - thread-safe integer with an embedded lock and conditional.
 *
 * Useful for inter-thread shared state that can trigger thread action.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 10095183
 */

#ifndef _COUNT_H_
#define _COUNT_H_

#include <pthread.h>

typedef struct count {
    int val;                        /* Count */
    pthread_mutex_t count_mutex;    /* Lock for the count */
    pthread_cond_t cond;            /* Conditional */
} count_t;

#define COUNT_INIT(name) { 0, PTHREAD_MUTEX_INITIALIZER, \
    PTHREAD_COND_INITIALIZER } 

#define COUNT(name) count_t name = COUNT_INIT(name)

static inline void INIT_COUNT(count_t *count)
{
    count->val = 0;
    pthread_mutex_init(&count->count_mutex, NULL);
    pthread_cond_init(&count->cond, NULL);
}

#define count_dec(count,i)                      \
    do {                                        \
        pthread_mutex_lock(&count.count_mutex);    \
        count.val -= i;                         \
        pthread_mutex_unlock(&count.count_mutex);  \
    }while(0);                                  \

#define count_inc(count,i)                      \
    do {                                        \
        pthread_mutex_lock(&count.count_mutex);    \
        count.val += i;                         \
        pthread_mutex_unlock(&count.count_mutex);  \
    }while(0);                                  \

#define count_set(count,i)                      \
    do {                                        \
        pthread_mutex_lock(&count.count_mutex);    \
        count.val = i;                          \
        pthread_mutex_unlock(&count.count_mutex);  \
    }while(0);                                  \

#endif /* _COUNT_H_ */

