/*
 * Starlocks - Simulating the corporate machine's lubricant delivery
 *  system since 2014.
 *
 * Simulates the throughput of customers through the Starlocks system
 *  with the given parameters, writing to STDOUT the average wait time
 *  for each given customer type (simple or complex, selected from a
 *  normal distribution). The profit is also computed and written to 
 *  STDOUT.
 *
 * Usage: ./starlocks num_customers -b num_baristas [-c num_cashiers]
 *          [-s num_selfserves] [-q]
 *
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 10095183
 */


#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include "server.h"
#include "addict.h"
#include "starlocks.h"
#include "check.h"
#include "count.h"

/* Get rid of the insane default stack size for the customers */
#ifndef THREAD_STACK_SIZE
#define THREAD_STACK_SIZE 65536 
#endif

/* Static definitions for global data */
COUNT(running_threads);
COUNT(gl_profit);
COUNT(simple_count);
COUNT(complex_count);
long *simple_times = NULL;
long *complex_times = NULL;
int quiet = 0;

/* Returns the sum of every long in the list. */
static inline long sum_list(long *list, int count)
{
    int i;
    long total = 0;
    for(i = 0; i < count; i++) {
        total += list[i];
    }
    return total;
}

static inline long min_list(long *list, int count)
{
    int i;
    long min = 999999;
    for(i = 0; i < count; i++) {
        min = list[i] < min ? list[i] : min;
    }
    return min;
}

static inline long max_list(long *list, int count)
{
    int i;
    long max = 0;
    for(i = 0; i < count; i++) {
        max = list[i] > max ? list[i] : max;
    }
    return max;
}

/* Average the count elements of the list. */
static inline long average_list(long *list, int count)
{
    long total = sum_list(list, count);
    return total / count;
}

/* 
 * Computes a random number 0 <= x < n with a uniform distribution. 
 *
 * Returns the value of x or -1 on failure.
 */
static int rand_range(unsigned int n)
{
    long end;
    int ret = -1;

    /* If we want in the full range, just call rand() */
    if((n-1) == RAND_MAX) {
        ret = rand();
        goto out;
    }

    end = RAND_MAX / n;
    check(end <= 0, out);

    /* Go until we have a value in our range */
    while((ret = rand()) >= end); 

    ret %= n;
out:
    return ret;
}

/* 
 * Starts a day in the regular mode of operation- one queue, n 
 * baristas. 
 */
int start_day_classic(int n_customers, int n_barista)
{
    struct server *server = NULL;
    struct addict *cur;
    int i, rand, ret = 1;
    pthread_attr_t attr;
    pthread_t *threads = malloc(n_customers * sizeof(pthread_t));
    check_pr(!threads, "Out of memory", out);

    /* Instantiate the service line with n service points */
    server = init_server(n_barista);
    check_pr(!server, "Out of memory", free_threads);

    /* Initialize the detachable attributes */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    /* Spawn customers of random type (simple, complex). */
    srand(time(NULL));
    for(i = 0; i < n_customers; i++) {
        rand = rand_range(2); /* Two types to select */
        switch(rand) {
            case 0:
                cur = init_addict(ATIME_SIMPLE, ACOST_SIMPLE,
                        server, NULL);
                break;
            default:
                cur = init_addict(ATIME_COMPLEX, ACOST_COMPLEX,
                        server, NULL);
        }
        check_pr(!cur, "Out of memory", finish);
        /* Start the timer */
        gettimeofday(&cur->start, NULL);
again:
        /* Start the corresponding thread */
        ret = pthread_create(&threads[i], &attr, (void *)*get_coffee, 
                cur);
        if(ret) {
            if(!quiet)
                printf("Failed to start thread %d, trying again\n",
                        i);
            sched_yield();
            goto again;
        }
        count_inc(running_threads, 1);
    }
    ret = 0;
finish:
    /* Wait until the work for the day is done */
    pthread_mutex_lock(&running_threads.count_mutex);
    while(running_threads.val > 0) {
        pthread_cond_wait(&running_threads.cond, 
                &running_threads.count_mutex);
    }
    pthread_mutex_unlock(&running_threads.count_mutex);
free_threads:
    /* Free all of the threads */
    free(threads);
    if(server)
        free(server);
out:
    return ret;
}

/* 
 * Starts a day in the new mode of operation- two queues, one that
 * feeds into self-service, and one that feeds into the coffee bar.
 *
 * One queue is also kept for the cashier.
 */
int start_day_complex(int n_customers, int n_selfserve,
        int n_barista, int n_cashier)
{
    struct server *server = NULL, *selfserve = NULL, *cashier = NULL;
    struct addict *cur;
    int i, rand, ret = 1;
    pthread_attr_t attr;
    pthread_t *threads = malloc(n_customers * sizeof(pthread_t));
    check_pr(!threads, "Out of memory", out);

    /* Instantiate the service lines */
    server    = init_server(n_barista);
    selfserve = init_server(3 * n_selfserve);
    cashier   = init_server(n_cashier);
    check_pr((!cashier || !server || !selfserve), 
            "Out of memory", free_threads);

    /* Initialize the detachable attributes */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    /* Spawn customers of random type (simple, complex). */
    srand(time(NULL));
    for(i = 0; i < n_customers; i++) {
        rand = rand_range(2); /* Two types to select */
        switch(rand) {
            case 0:
                cur = init_addict(ATIME_SIMPLE, ACOST_SIMPLE,
                        selfserve, cashier);
                break;
            default:
                cur = init_addict(ATIME_COMPLEX, ACOST_COMPLEX,
                        server, cashier);
        }
        check_pr(!cur, "Out of memory", finish);
        /* Start the timer */
        gettimeofday(&cur->start, NULL);
again:
        /* Start the corresponding thread */
        ret = pthread_create(&threads[i], &attr, (void *)*get_coffee, 
                cur);
        if(ret) {
            if(!quiet)
                printf("Failed to start thread %d, trying again\n",
                        i);
            sched_yield();
            goto again;
        }
        count_inc(running_threads, 1);
    }
    ret = 0;
finish:
    /* Wait until the work for the day is done */
    pthread_mutex_lock(&running_threads.count_mutex);
    while(running_threads.val > 0) {
        pthread_cond_wait(&running_threads.cond, 
                &running_threads.count_mutex);
    }
    pthread_mutex_unlock(&running_threads.count_mutex);
free_threads:
    /* Free all of the threads, and all of the servers */
    free(threads);
    if(server)
        free(server);
    if(selfserve)
        free(selfserve);
    if(cashier)
        free(cashier);
out:
    return ret;
}

/* Start the day with the given parameters. 
 *
 *  Returns the total profit at the end of the day when all customers
 *  have been served, or -1 on failure. 
 */
static int start_day(int n_customers, int n_selfserve,
        int n_barista, int n_cashier)
{
    int ret;

    count_set(gl_profit, 0);

    /* 
     * If there are no self services, start in the classic mode 
     * and ignore the number of cashiers. Otherwise, start in complex
     * mode. 
     */
    if(!n_selfserve)
        ret = start_day_classic(n_customers, n_barista);
    else
        ret = start_day_complex(n_customers, n_barista, n_selfserve,
                n_cashier);

    if(ret)
        return -ret;

    /* Tally up the profits */
    pthread_mutex_lock(&gl_profit.count_mutex);
    ret = gl_profit.val;
    pthread_mutex_unlock(&gl_profit.count_mutex);

    return ret;
}

static inline void print_usage(char *name)
{
    printf("Usage: %s num_customers [-s num_selfserve] "
            "[-b num_barista] [-c num_cashier]\n",name);
}

static inline void print_profit(int profit)
{
    int cents, dollars;  
    cents = profit % 100;
    dollars = profit / 100;
    printf("Profit:\t$ %d.%02d\n",dollars,cents);
}

static inline void print_time(int time_microsecs)
{
    int seconds, milliseconds;
    seconds = time_microsecs / 1000000;
    milliseconds = time_microsecs % 1000000;
    printf("%d.%06d\n",seconds,milliseconds);
}

int main(int argc, char **argv)
{
    unsigned int num_customers;
    unsigned int num_selfserve = 0, num_barista = 0, num_cashier = 0; 
    int opt, ret = -1, profit;
    long avg_simple, avg_complex;

    if(argc < 2) {
        print_usage(argv[0]);
        goto out;
    }
    num_customers = atoi(argv[1]);
    check_pr(!num_customers, "Need at least one customer", out);


    while((opt = getopt(argc, argv, "s:b:c:q")) != -1)
    {
        switch(opt) {
            case 's': 
                num_selfserve = atoi(optarg);
                break;
            case 'b':
                num_barista = atoi(optarg);
                break;
            case 'c':
                num_cashier = atoi(optarg);
                break;
            case 'q':
                quiet = 1;
                break;
            default:
                print_usage(argv[0]);
                goto out;
        }
    }

    if(quiet) {
        check(!num_barista, out);
    } else {
        check_pr(!num_barista, "Need at least one barista", out);
    }

    if(num_selfserve) {
        if(quiet) {
            check(!num_cashier, out);
        } else {
            check_pr(!num_cashier, "Need at least one cashier", out);
        }
    }

    if(!quiet)
        printf( "Customers     :\t%d\n"
                "Self Services :\t%d\n"
                "Baristas      :\t%d\n"
                "Cashiers      :\t%d\n", 
                num_customers, num_selfserve, 
                num_barista, num_cashier);

    /* Allocate room for our list of times */
    simple_times = malloc(sizeof(long) * num_customers);
    check(!simple_times, free_times);
    complex_times = malloc(sizeof(long) * num_customers);
    check(!complex_times, free_times);

    profit = start_day(num_customers,
            num_selfserve, num_barista, num_cashier);
    check_pr(profit < 0, 
                "Simulation Aborted (Out of resources).",
                out);
    print_profit(profit);
    /* Compute the average turnaround time for each customer type */
    if(simple_count.val > 0)
        avg_simple = average_list(simple_times, simple_count.val);
    else
        avg_simple = 0l;
    if(complex_count.val > 0)
        avg_complex = average_list(complex_times, complex_count.val);
    else
        avg_complex = 0l;
    printf("Avg Simple :\t");
    print_time(avg_simple);
    printf("Avg Complex:\t");
    print_time(avg_complex);

    ret = 0;
free_times:
    if(complex_times)
        free(complex_times);
    if(simple_times)
        free(simple_times);
out:
    pthread_exit(&ret);
}

