/* 
 * Global variables for starlocks 
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 10095183
 */

#ifndef _STARLOCKS_H_
#define _STARLOCKS_H_

#include "count.h"

extern count_t gl_profit;
extern count_t running_threads;
extern count_t simple_count;
extern count_t complex_count;
extern long *simple_times;
extern long *complex_times;

#endif /* _STARLOCKS_H */

