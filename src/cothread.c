#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "cothread.h"

#define container_of(ptr, type, member) ({ \
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)( (char *)__mptr - offsetof(type,member) );})

typedef void (*makecontext_func_t)();

struct cothread_s {
    int state;
    ucontext_t ctx;
};

struct pool_item {
    struct pool_item *last;
    struct pool_item *next;
    struct cothread_s thread;
};

typedef struct pool_item pool_item_t;

struct cothread_pool_s {
    struct cothread_s main_thread;
    struct cothread_s *current_thread;

    struct pool_item *work_list;
    struct pool_item *free_list;
};

static __thread struct cothread_pool_s cothread_pool;

static void init_cothread(struct cothread_s *ct);
static void free_cothread(struct cothread_s *ct);
static void reinit_cothread(struct cothread_s *, cothread_fun_t, void *arg);
static void switch_to(ucontext_t *ctx);

static struct pool_item *pool_list_pop(struct pool_item **list);
static void pool_list_remove(struct pool_item **list, struct pool_item *item);
static void pool_list_push(struct pool_item **list, struct pool_item *item);


void init_cothread(struct cothread_s *ct)
{
    memset(ct, 0, sizeof(struct cothread_s));
    getcontext(&ct->ctx);
    ssize_t stacksize = 1024 * 8;
    ct->ctx.uc_stack.ss_sp = malloc(stacksize);
    ct->ctx.uc_stack.ss_size = stacksize;
}

void free_cothread(struct cothread_s *ct)
{
    free(ct->ctx.uc_stack.ss_sp);
    memset(ct, 0, sizeof(struct cothread_s));
}


void reinit_cothread(struct cothread_s *ct, cothread_fun_t func, void *arg)
{
    ct->ctx.uc_link = &cothread_pool.current_thread->ctx;
    makecontext(&ct->ctx, (makecontext_func_t) func, 1, arg);
}

void cothread_init()
{
    getcontext(&cothread_pool.main_thread.ctx);
    cothread_pool.current_thread = &cothread_pool.main_thread;

    cothread_pool.work_list = NULL;
    cothread_pool.free_list = NULL;
}

cothread_t cothread_create(cothread_fun_t routine, void *arg)
{
    struct pool_item *cothread = pool_list_pop(&cothread_pool.free_list);
    if(cothread == NULL) {
        cothread = malloc(sizeof(struct pool_item));
        cothread->next = NULL;
        cothread->last = NULL;
        init_cothread(&cothread->thread);
    }
    reinit_cothread(&cothread->thread, routine, arg);

    // Now, switch to new cothread:
    struct cothread_s *current = cothread_pool.current_thread;
    cothread_pool.current_thread = &cothread->thread;

    swapcontext(&current->ctx, &cothread->thread.ctx);

    cothread_t new_cothread;
    new_cothread.thread = &cothread->thread;
    return new_cothread;
}

void cothread_resume(cothread_t thread_)
{
    struct cothread_s *thread = thread_.thread;

    thread->ctx.uc_link = &cothread_pool.current_thread->ctx;

    struct cothread_s *current = cothread_pool.current_thread;
    cothread_pool.current_thread = thread;
    swapcontext(&current->ctx, &thread->ctx);
    return;
}

void cothread_yield()
{
    struct cothread_s *thread = cothread_pool.current_thread;

    struct cothread_s *current = cothread_pool.current_thread;
    cothread_pool.current_thread = thread;
    swapcontext(&current->ctx, thread->ctx.uc_link);

    return;
}

void cothread_release(cothread_t thread)
{
    struct pool_item *item = container_of(
                thread.thread, struct pool_item, thread);
    pool_list_remove(&cothread_pool.work_list, item);
    pool_list_push(&cothread_pool.free_list, item);
}

struct pool_item *pool_list_pop(struct pool_item **list)
{
    struct pool_item *head = *list;
    if(head == NULL)
        return NULL;
    *list = (*list)->next;
    if(*list) {
        (*list)->last = NULL;
    }

    head->last = NULL;
    head->next = NULL;
    return head;
}
void pool_list_remove(struct pool_item **list, struct pool_item *item)
{
    if(*list == item) {
        *list = item->next;
        if(*list) {
            (*list)->last = NULL;
        }
    } else {
        item->last->next = item->next;
        if(item->next) {
            item->next->last = item->last;
        }
    }
    item->next = NULL;
    item->last = NULL;
}
void pool_list_push(struct pool_item **list, struct pool_item *item)
{
    item->next = (*list)->next;
    item->last = NULL;

    if((*list)->next) {
        (*list)->next->last = item;
    }
    *list = item;
}

