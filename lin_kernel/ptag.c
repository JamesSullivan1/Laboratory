/* 
 * Support for Process Tags
 *
 * ======================
 * What is a Process Tag?
 * ======================
 *  Process tags are strings that can be attached to a process on
 *   a per-process basis. A process can have any number of unique tags
 *   associated with it. These tags are useful for identifying and grouping
 *   processes at the user level.
 *
 * A global list of Process Tags is defined in this file, which contains
 *  a number of ptag_struct objects. Each of these in turn has a linked-list
 *  of processes that have their tag.
 *
 * When a process is forked, its ptags are copied to the child process. The
 *   init process does not have any process tags by default.
 *
 * ======================
 * Supported Operations
 * ======================
 *  The ptag(2) system call supports the following operation requests.
 *   PTAG_ADD           - Add a process tag
 *   PTAG_REMOVE        - Remove a process tag
 *
 * James Sullivan <sullivan.james.f@gmail.com>
 * 10095183
 */

#include <linux/ptag.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/syscalls.h>
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/errno.h>
#include <linux/cred.h>
#include <linux/capability.h>
#include <linux/uaccess.h>
#include <linux/rwsem.h>

/* The global process tag tasklist */
LIST_HEAD(ptag_tasklist);

/* Lock for the ptag list */
DECLARE_RWSEM(ptag_list_rwsem);

/* 
 * Initialize a new ptag object with the given tag. If a tag that is 
 *  too long is requested, then the ptag will not be instantiated.
 *
 *  The request might fail if there is not enough memory left.
 */
struct ptag_struct *init_ptag(char *tag, unsigned int tag_len)
{
        struct ptag_struct *ptag;

        if(tag_len > PTAG_TAG_MAX)
                return NULL;

        /* Allocate some memory for the new ptag */
        ptag = kmalloc(sizeof(struct ptag_struct), GFP_KERNEL);
        if(!ptag)
                return NULL;                    /* Out of memory - fail*/

        ptag->tag = tag;                        /* Set the tag reference */
        ptag->tag_len = tag_len;                /* Set tag length */

        return ptag;
}

/*
 * Initializes a new task wrapper for the linked list of tagged processes.
 *
 * The request may fail if we run out of memory. 
 */
struct ptag_tasks_struct *init_ptag_task(struct task_struct *t)
{
        struct ptag_tasks_struct *task;

        /* Allocate some memory for the task list entry */
        task = kmalloc(sizeof(struct ptag_tasks_struct), GFP_KERNEL);
        if(!task)
                return NULL;                    /* Out of memory */
        
        task->task = t;                         /* Set the task ptr */
        INIT_LIST_HEAD(&task->ptags);           /* Init the ptag list */

        return task;
}

/* 
 * Add an initialized task to the tasklist. The list is kept ordered by PID,
 *  so the new entry is added before the first PID greater than its own.
 *  
 * Returns 0 on success and 1 if the PID is already in the list (failure).
 */
int add_ptag_task(struct ptag_tasks_struct *task)
{
        struct ptag_tasks_struct *cur;
        int ret = 0;
        
        /* Initial entry just goes at the head of the list */
        down_write(&ptag_list_rwsem);
        if(list_empty(&ptag_tasklist)) {
                list_add(&task->task_list, &ptag_tasklist);
                goto unlock;
        }
        ptag_for_each_task(cur) {
                if(unlikely(cur->task->pid == task->task->pid)) {
                        ret = 1; /* Already in the list, abort */
                        goto unlock;
                }
                /* This is the first PID greater than the new one, so 
                 * add the new entry behind cur */
                if(cur->task->pid > task->task->pid) {
                        list_add_tail(&task->task_list, &cur->task_list);
                        goto unlock;
                }
        }
        /* If no such ptag was found then the PID to add is maximal,
         *  so put it at the end of the list. */
        list_add_tail(&task->task_list, &cur->task_list);
unlock:
        up_write(&ptag_list_rwsem);
        return ret;
}

/* Returns 0 if current can modify the given task, -EPERM otherwise */
int __ptag_can_modify(struct task_struct *t)
{
        const struct cred *cred = current_cred(), *tcred;

        tcred = __task_cred(t);
        /* 
         * Allow ptag access if any of the following are satisfied:
         *  current euid = target suid
         *  current euid = target uid
         *  current uid  = target suid
         *  current uid  = target uid
         *  current is priveleged (CAP_SYS_PACCT, process accounting)
         */
        if ((cred->euid ^ tcred->suid) &&
            (cred->euid ^ tcred->uid)  &&
            (cred->uid  ^ tcred->suid) &&
            (cred->uid  ^ tcred->uid)  &&
            !capable(CAP_SYS_PACCT))
                return -EPERM;

        return 0;
}

/* Returns True if and only if current can modify the given task's tags */
bool ptag_can_modify(struct task_struct *t)
{
        int err;
        task_lock(t);
        err = __ptag_can_modify(t);
        task_unlock(t);
        /* If there's no errors, then return 1 */
        return ((!err) ? 1 : 0);
}
EXPORT_SYMBOL(ptag_can_modify);

/* 
 * If the given task_struct has any tags, return the container in the tasklist
 *  with this task. If not, return NULL.
 */
struct ptag_tasks_struct *ptag_get_task(struct task_struct *t)
{
        struct ptag_tasks_struct *cur = NULL;
        int task_tagged = 0;

        /* Lock the task list */
        down_read(&ptag_list_rwsem);
        /* Check the task list for this task */
        ptag_for_each_task(cur) {
                if(unlikely(!cur->task)) {
                        panic("PTAG: Null task_struct in current");
                }
                /* No need to keep looking after we hit a greater PID */
                if(t->pid < cur->task->pid)
                        break;
                if(t->pid == cur->task->pid) {
                        task_tagged = 1;
                        break; /* Found it, exit */
                }
        }
        up_read(&ptag_list_rwsem);

        if(!task_tagged)
                cur = NULL;

        return cur;
}

/* 
 * If the task has the given tag, then return the ptag container
 *  that holds the tag. Otherwise, return NULL.
 */
struct ptag_struct *ptag_get_tag(struct ptag_tasks_struct *task, char *tag)
{
        struct ptag_struct *cur = NULL;
        int is_tagged = 0;

        down_read(&ptag_list_rwsem);
        /* Check if this tag string is in the list */
        list_for_each_entry(cur, &task->ptags, ptags) {
                if(strcmp(cur->tag, tag) == 0) {
                        is_tagged = 1;
                        break; /* Found it- break out */ 
                }
        }
        up_read(&ptag_list_rwsem);

        if(!is_tagged)
                cur = NULL;

        return cur;
}

/* 
 * Delete the given task from the ptag tasklist, also deleting all of its
 *  process tags.
 */
void destroy_ptags(struct task_struct *t)
{
        struct ptag_struct *p_cur, *p_next;
        struct ptag_tasks_struct *task;

        /* Get the tasklist container of the task, if it exists */
        task = ptag_get_task(t);
        if(!task)
                goto out;

        down_write(&ptag_list_rwsem);

        /* Delete and deallocate all of the process tags */
        list_for_each_entry_safe(p_cur, p_next, &task->ptags, ptags) {
                list_del(&p_cur->ptags);
                kfree(p_cur->tag);
                kfree(p_cur);
        }
        /* Now delete the task itself from the global tasklist */
        list_del(&task->task_list);
        kfree(task);

        up_write(&ptag_list_rwsem);

out:
        return;
}

/* 
 * Copies the ptag list of one process to the other, if any such tags
 *  exist. 
 *
 * Returns 0 on success and 1 if there's insufficient memory.
 */
int copy_ptags(struct task_struct *to, struct task_struct *from)
{
        struct ptag_struct *p_cur, *p_new;
        struct ptag_tasks_struct *t_cur, *t_new;
        char *new_tag;
        int ret = 0;

        /* Nothing to do if they're the same task */
        if(unlikely(to == from))
                goto out;

        /* Get the tasklist container for the 'from' task */
        t_cur = ptag_get_task(from);
        if(!t_cur)
                goto out; /* No tags to copy */

        /* Create a new entry in the tasklist */
        ret = 1;
        t_new = init_ptag_task(to);
        if(unlikely(!t_new))
                goto out; /* Out of memory */
        add_ptag_task(t_new);

        /* Now copy all of the ptags to the new entry's taglist */
        ret = 0;
        down_write(&ptag_list_rwsem);
        list_for_each_entry(p_cur, &t_cur->ptags, ptags) {
                /* Make a copy of the process tag */
                new_tag = kmalloc(p_cur->tag_len, GFP_KERNEL);
                if(unlikely(!new_tag)) {
                        ret = 1;
                        goto unlock; /* Out of memory */
                }
                memcpy(new_tag, p_cur->tag, p_cur->tag_len);
                new_tag[p_cur->tag_len] = '\0';

                p_new = init_ptag(new_tag, p_cur->tag_len);
                if(unlikely(!p_new)) {
                        ret = 1;
                        goto unlock; /* Out of memory */
                }

                /* Add the new entry */
                list_add(&p_new->ptags, &t_new->ptags);
        }
unlock:
        up_write(&ptag_list_rwsem);
out:
        return ret;
}

/*
 * Add a ptag to a process, unless it already has this tag.
 *  Allocates a new ptag_struct node in the tag list for the task.
 *  If the task has no tags, allocates a new entry in the global tasklist.
 *
 *  If the tag or the task_struct is NULL, return -EFAULT.
 *  If the tag length is greater than PTAG_TAG_MAX, return -EINVAL.
 *  If the process has no tag list, return -EFAULT.
 *  If the tag list is already full, return -EACCES.
 */
int _add_ptag(struct task_struct *t, char *tag, unsigned int tag_len)
{
        struct ptag_struct *p_new = NULL;
        struct ptag_tasks_struct *task;
        int ret;

        ret = -EINVAL;
        if(tag_len > PTAG_TAG_MAX)
                goto out;

        /* Initialize the new ptag object */
        ret = -ENOMEM;
        p_new = init_ptag(tag, tag_len);
        if(!p_new)
                goto out; /* Out of memory */

        /* 
         * If the task has a tasklist entry, get it. Otherwise,
         *  initialize a new one for it. 
         */
        task = ptag_get_task(t);
        if(!task) {
                ret = -ENOMEM;
                task = init_ptag_task(t);
                if(!task)
                        goto free_tag; /* Out of memory */
                add_ptag_task(task);
        } else {
                /* If the process already has this tag, exit. */
                ret = 0;
                if(ptag_get_tag(task, tag))
                        goto free_tag;
        }

        /* Add the new ptag entry to the tag list */
        down_write(&ptag_list_rwsem);
        list_add(&p_new->ptags, &task->ptags);
        up_write(&ptag_list_rwsem);

        ret = 0;
out:
        return ret;
free_tag:
        kfree(p_new->tag);
        kfree(p_new);
        return ret;
}

/*
 * Removes the given tag from the given process if it has this tag. 
 *  Otherwise, doesn't do much at all.
 * 
 *  If the tag or the task_struct is NULL, return -EFAULT.
 *  If the tag length is greater than PTAG_TAG_MAX, return -EINVAL.
 *  If the process has no tag list, return -EFAULT.
 */
int _remove_ptag(struct task_struct *t, char *tag, unsigned int tag_len)
{
        struct ptag_struct *cur;
        struct ptag_tasks_struct *task;
        int cull_task = 0;
        int ret;

        ret = -EINVAL;
        if(tag_len > PTAG_TAG_MAX)
                goto out;
        
        /* Get the entry containing the task in the tasklist. */
        ret = 0;
        task = ptag_get_task(t);
        if(!task)
                goto out; /* No tags associated with the task */

        /* Find the corresponding entry in the tag list */
        cur = ptag_get_tag(task, tag);
        if(!cur)
                goto out;

        down_write(&ptag_list_rwsem);

        /* Kill it with fire */
        list_del(&cur->ptags);
        /* Mark the task entry for culling if it has no tags */
        if(list_empty(&task->ptags))
                cull_task = 1;
        kfree(cur->tag);
        kfree(cur);

        /* If the process has no tags, remove it from the tasklist */
        if(cull_task) {
                list_del(&task->task_list);
                kfree(task);
        }

        up_write(&ptag_list_rwsem);
out:
        return ret;
}

/* 
 * Add a ptag to the process with the given PID. 
 *  If no such process exists, return -ESRCH. 
 *  If the current task can't modify the target task, return -EPERM.
 */
int add_ptag(pid_t pid, char *tag, unsigned int tag_len)
{
        struct task_struct *t;
        
        /* Check if the given process exists */
        t = find_task_by_vpid(pid); 
        if(!t)
                return -ESRCH;
        /* Check if current can modify it */
        if(!ptag_can_modify(t))
                return -EPERM;

        return _add_ptag(t, tag, tag_len);
}
EXPORT_SYMBOL(add_ptag);

/* 
 * Remove a ptag from the process with the given PID. 
 *  If no such process exists, return -ESRCH. 
 *  If the current task can't modify the target task, return -EPERM.
 */
int remove_ptag(pid_t pid, char *tag, unsigned int tag_len)
{
        struct task_struct *t;
        
        /* Check if the given process exists */
        t = find_task_by_vpid(pid); 
        if(!t)
                return -ESRCH;
        /* Check if current can modify it */
        if(!ptag_can_modify(t))
                return -EPERM;

        return _remove_ptag(t, tag, tag_len);
}
EXPORT_SYMBOL(remove_ptag);

/*
 * Defines the ptag system call.
 *      request - The operation request of the ptag system call
 *           PTAG_ADD | PTAG_REMOVE
 *      pid     - The Process ID to add or remove ptags from
 *      tag     - The process tag to add or remove
 *      tag_len - The length of the tag. Must be no more than PTAG_TAG_MAX
 *
 * Returns either 0 on success, or an error number on failure.
 */
SYSCALL_DEFINE4(ptag,
                long, request,
                pid_t, pid,
                char __user *, tag,
                unsigned long, tag_len)
{
        char *buf;
        int ret = 0;

        /* Check for upper bound on the length */
        ret = -EINVAL;
        if(tag_len > PTAG_TAG_MAX)
                goto out;

        /* Allocate space for the tag and copy from the user */
        ret = -ENOMEM;
        if(!(buf = kmalloc(tag_len + 1, GFP_KERNEL)))
                goto out;
        if(copy_from_user(buf, tag, tag_len))
                goto free_buffer;
        buf[tag_len] =  '\0'; /* Bad user, no overflows */

        ret = -EINVAL;
        switch(request){
                case(PTAG_ADD):
                        ret = add_ptag(pid, buf, tag_len);
                        goto out;
                case(PTAG_REMOVE):
                        ret = remove_ptag(pid, buf, tag_len);
                        goto out;
                default:
                        break;
        }

free_buffer:
        kfree(buf);
out: 
        return ret;
}

