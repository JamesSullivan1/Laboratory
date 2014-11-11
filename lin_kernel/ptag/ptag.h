#ifndef _LINUX_PTAG_H_
#define _LINUX_PTAG_H_

#include <linux/list.h>
#include <linux/rwsem.h>

/*
 * Process Tags
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 10095183
 */

/* Operation Requests */
#define PTAG_ADD        0x0
#define PTAG_REMOVE     0x1

/* Tag Length Boundary (inclusive) */
#define PTAG_TAG_MAX    1023

/* Traversal macros for the global ptag task_list */
#define ptag_for_each_task(cur)                         \
        list_for_each_entry(cur, &ptag_tasklist, task_list)

#define ptag_for_each_task_safe(cur, next)              \
        list_for_each_entry_safe(cur, next, &ptag_tasklist, task_list)

extern struct list_head ptag_tasklist;
extern struct rw_semaphore ptag_list_rwsem;

/* 
 * Container for a single task in the ptag task list. Holds a list head
 *  into the task list and a list head for its own ptags.
 */
struct ptag_tasks_struct {
        struct task_struct *task;       /* Task that has this tag */
        struct list_head task_list;     /* List of other tasks */ 
        struct list_head ptags;         /* List of process tags */
};

/* 
 * Container for a single process tag.
 */
struct ptag_struct {
        char *tag;                      /* Tag string */
        unsigned int tag_len;           /* Length of tag */
        struct list_head ptags;         /* List of other ptags */
};

bool ptag_can_modify(struct task_struct *t);
int copy_ptags(struct task_struct *to, struct task_struct *from);
void destroy_ptags(struct task_struct *t);
int add_ptag(pid_t pid, char *tag, unsigned int tag_len);
int remove_ptag(pid_t pid, char *tag, unsigned int tag_len);

#endif /* _LINUX_PTAG_H_ */

