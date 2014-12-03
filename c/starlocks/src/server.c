/*
 * server - Corporate Caffeine Delivery System
 *
 * Contains functionality necessary for the servicing of addict
 * caffeine requests. A server is defined by a critical section
 * (in particular, a semaphore) that a fixed number of customers may
 * simultaneously enter.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 10095183
 */

#include <pthread.h>
#include "server.h"
#include "check.h"
#include "count.h"
#include "starlocks.h"
#include <semaphore.h>

/* 
 * Initialize a new server of the given type. 
 *
 * Returns 0 if the server type is invalid or there's not enough
 * memory.
 */
struct server *init_server(unsigned int max_service)
{
    struct server *server = NULL;
    check(max_service == 0, out);
    server = malloc(sizeof(struct server));
    check(!server, out);

    server->max_service = max_service;
    sem_init(&server->service_sem, 0, server->max_service);
    #ifndef CHAOS
    fifo_mutex_init(&server->lock);
    #else
    pthread_mutex_init(&server->lock);
    #endif
out:
    return server;
}

/*
 * Serve the given addict their glorious caffeine. The time to 
 *  service the addict's request depends on their order_time value.
 */
inline void serve(struct addict *addict)
{
    _serve(addict); /* Busy loop for order_time times */ 
    addict->caffeinated++;
}

/*
 * Force the addict to cough up the dough. This may take some 
 *  fixed amount of time.
 */
inline void pay(struct addict *addict)
{
    _pay(); /* Busy wait for a fixed amount of time */
    count_inc(gl_profit, addict->order_cost);
}

