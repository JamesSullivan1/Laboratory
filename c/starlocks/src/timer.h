/*
 * Static timer function definitions.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 10095183
 */

#ifndef _TIMER_H_
#define _TIMER_H_

#include <time.h>

/* Return the difference in time between start and end in seconds */
inline long timer_s(struct timeval *start, struct timeval *end)
{
    long delta = end->tv_sec - start->tv_sec;
    return delta;
}

/* Return the difference in time between start and end in millisecs */
inline long timer_ms(struct timeval *start, struct timeval *end)
{
    long delta = ((end->tv_sec - start->tv_sec) * 1000000) 
            + (end->tv_usec - start->tv_usec);
    return delta / 1000000;
}

/* Return the difference in time between start and end in microsecs */
inline long timer_us(struct timeval *start, struct timeval *end)
{
    long delta = ((end->tv_sec - start->tv_sec) * 1000000) 
            + (end->tv_usec - start->tv_usec);
    return delta / 1000;
}

/* Return the difference in time between start and end in nanosecs */
inline long timer_ns(struct timeval *start, struct timeval *end)
{
    long delta = ((end->tv_sec - start->tv_sec) * 1000000) 
            + (end->tv_usec - start->tv_usec);
    return delta;
}

#endif /* _TIMER_H_ */

