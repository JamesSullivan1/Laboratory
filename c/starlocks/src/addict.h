#ifndef _ADDICT_H_
#define _ADDICT_H_

#include <sys/time.h>

#define ATIME_SIMPLE    1<<18   /* Loop iterations */
#define ATIME_COMPLEX   1<<19
#define PAY_TIME        1<<18

#define ACOST_SIMPLE    200     /* In cents */
#define ACOST_COMPLEX   450 

enum 
{
    ATYPE_SIMPLE,
    ATYPE_COMPLEX
};

struct addict {
    unsigned int order_time;    /* Time for order completion */ 
    unsigned int order_cost;    /* Order cost */
    int caffeinated;            /* Is caffeinated */
    struct server *server;      /* First server to go to */ 
    struct server *next;        /* Optional next server */
    struct timeval start;       /* Used for timing measurement */
    struct timeval end;
};

struct addict *init_addict(unsigned int order_time, 
        unsigned int order_cost, 
        struct server *server, 
        struct server *next);

void get_coffee(struct addict *);

#endif /* _ADDICT_H_ */

