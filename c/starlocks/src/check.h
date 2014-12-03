/*
 * Access to the simple 'check' macro which checks a condition,
 * optionally prints a message and jumps to a label on the cond holding.
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 10095183
 */

#ifndef _CHECK_H_
#define _CHECK_H_

#include <stdio.h>
#include <stdlib.h>

#define check(cond,fail_label)                  \
        if(cond) {                              \
            goto fail_label;                    \
        }                                       \

#define check_pr(cond,message,fail_label)       \
        if(cond) {                              \
            printf("ERROR: %s\n",message);      \
            goto fail_label;                    \
        }

#endif /* _CHECK_H_ */

