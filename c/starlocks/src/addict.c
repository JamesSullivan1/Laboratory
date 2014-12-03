/* 
 * Addict - Hapless victim of the Corporate Caffeine Delivery System.
 *
 * Each customer thread has exactly one corresponding addict struct.
 * This struct shall be used for control flow of the thread, and is
 * passed as an argument to the thread when it starts.
 *
 * It is the thread's responsibility to deallocate its corresponding
 * addict struct before it exits- this is handled in get_coffee().
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 10095183
 */

#include "addict.h"
#include "count.h"
#include "server.h"
#include "queue.h"
#include "check.h"
#include "starlocks.h"
#include "timer.h"
#include <semaphore.h>
#include <sys/time.h>

#ifndef CHAOS
#include "fifo_mutex.h"
#endif

/*
 * Initialize an addict with the given parameters, and return a
 * reference to it. Returns NULL on failure.
 */
struct addict *init_addict(unsigned int time, unsigned int cost,
        struct server *server, struct server *next)
{
    struct addict *addict = malloc(sizeof(struct addict));
    check(!addict, out);

    addict->order_time  = time;
    addict->order_cost  = cost;
    addict->caffeinated = 0;
    addict->server      = server;
    addict->next        = next;
out:
    return addict;
}

/* 
 * Do the gruelling work of getting a coffee. 
 *
 * The thread will attempt to occupy both its first and next service 
 * points in order. While in each critical section, a busy loop is used
 * to simulate the order waiting time.
 *
 * After the thread is done, it is their job to deallocate their control
 * struct.
 */
void get_coffee(struct addict *addict)
{
    long time;
    check(!addict, exit);

    /* 
     * This is used for two reasons.
     * 1) With FIFO enabled, this lock maintains the order of the
     *  threads and atomicizes the operation of selecting a service
     *  point.
     * 2) Without FIFO enabled, this lock atomicizes the operation
     *  of selecting a service point (and also incurs a similar penalty
     *  of performance to the FIFO lock, keeping things fairish).
     */
    #ifndef CHAOS
    fifo_mutex_lock(&addict->server->lock);
    sem_wait(&addict->server->service_sem);
    fifo_mutex_unlock(&addict->server->lock);
    #else
    pthread_mutex_lock(&addict->server->lock);
    sem_wait(&addict->server->service_sem);
    pthread_mutex_unlock(&addict->server->lock);
    #endif

    serve(addict);
    /* If there's no next server, also pay */
    if(!addict->next)
        pay(addict);

    sem_post(&addict->server->service_sem);

    /* Optional second cashier */
    if(addict->next) {
        #ifndef CHAOS
        fifo_mutex_lock(&addict->next->lock);
        sem_wait(&addict->next->service_sem);
        fifo_mutex_unlock(&addict->next->lock);
        #else
        pthread_mutex_lock(&addict->next->lock);
        sem_wait(&addict->next->service_sem);
        pthread_mutex_unlock(&addict->next->lock);
        #endif

        pay(addict);

        sem_post(&addict->next->service_sem);
    }

exit:
    /* End the timer, and set our results in the relevant table */
    gettimeofday(&addict->end, NULL);
    time = timer_us(&addict->start, &addict->end);
    switch(addict->order_cost) {
        case ACOST_SIMPLE:
            pthread_mutex_lock(&simple_count.count_mutex);
            simple_times[simple_count.val++] = time;
            pthread_mutex_unlock(&simple_count.count_mutex);
            break;
        case ACOST_COMPLEX:
            pthread_mutex_lock(&complex_count.count_mutex);
            complex_times[complex_count.val++] = time;
            pthread_mutex_unlock(&complex_count.count_mutex);
        default:
            break;
    }
    free(addict);
    /* Signal that a thread is exiting */
    count_dec(running_threads, 1);
    /* The last one out wakes up main */
    pthread_mutex_lock(&running_threads.count_mutex);
    if(running_threads.val == 0)
        pthread_cond_broadcast(&running_threads.cond);
    pthread_mutex_unlock(&running_threads.count_mutex);
    pthread_exit(NULL);
}

