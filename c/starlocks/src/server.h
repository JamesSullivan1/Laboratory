/*
 * server - Corporate Caffeine Delivery System 
 *
 * Type definitions and macros for the server module.
 */
#ifndef _SERVER_H_
#define _SERVER_H_

#include "queue.h"
#include "addict.h"
#include <semaphore.h>
#ifndef CHAOS
#include "fifo_mutex_types.h"
#endif

/* Busy loop that runs for as long as the addict's order takes. */
#define _serve(addict)          \
    do {                        \
        volatile int _cnt;      \
        for(_cnt = 0; _cnt < addict->order_time; _cnt++) {};   \
    } while(0);

/* Busy loop that runs for as long as the payment takes. */
#define _pay(addict)            \
    do {                        \
        volatile int _cnt;      \
        for(_cnt = 0; _cnt < PAY_TIME; _cnt++) {};   \
    } while(0);

struct server { 
    int max_service;                    /* Number of service points */
    sem_t service_sem;                  /* Service point semaphore */
    #ifndef CHAOS
    fifo_mutex_t lock;                  /* Fair FIFO */
    #else
    pthread_mutex_t lock;               /* MACFO entry lock */
    #endif
};

struct server *init_server(unsigned int max_service);
inline void serve(struct addict *);
inline void pay(struct addict *);

#endif /* _SERVER_H_ */

