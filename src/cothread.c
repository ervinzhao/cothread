#include <stdlib.h>
#include <string.h>
#include "cothread.h"


#define container_of(ptr, type, member) ({ \
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)( (char *)__mptr - offsetof(type,member) );})    



struct cothread_s {
    int state;
    ucontext_t ctx;
};

struct pool_item {
    struct pool_item *last;
    struct pool_item *next;
    cothread_s thread;
};

struct cothread_pool_s {
    cothread_s main_thread;
    cothread_s *current_thread;

    struct pool_item *work_list;
    struct pool_item *free_list;
};

static __thread struct cothread_pool_s cothread_pool;

static void init_cothread(cothread_s *ct);
static void free_cothread(cothread_s *ct);
static void reinit_cothread(cothread_s *, cothread_fun_t, void *arg);
static void switch_to(ucontext_t *ctx);

struct pool_item *pool_list_pop(struct pool_item *list);
void pool_list_remove(struct pool_item *list, struct pool_item *item);
void pool_list_push(struct pool_item *list, struct pool_item *item);


void cothread_init()
{
    getcontext(&cothread_pool.main_thread);
    cothread_pool.current_thread = &cothread_pool.main_thread;

    cothread_pool.work_list = NULL;
    cothread_pool.free_list = NULL;
}

cothread_t cothread_create(cothread_fun_t routine, void *arg)
{
    struct pool_item *cothread = pool_list_pop(cothread_pool.free_list);
    if(cothread == NULL) {
        cothread = malloc(sizeof(struct pool_item));
        cothread->next = NULL;
        cothread->last = NULL;
        init_cothread(&cothread->thread);
    }
    reinit_cothread(&cothread->thread, routine, arg);

    // Now, switch to new cothread:
    cothread_s *current = cothread_pool.current_thread;
    cothread_pool.current_thread = cothread->thread;

    swapcontext(&current.ctx, &cothread->thread.ctx);

    cothread_t new_cothread;
    new_cothread.thread = &cothread->thread;
    return new_cothread;
}

void cothread_resume(cothread_t thread_)
{
    cothread_s *thread = thread_.thread;

    thread.ctx.uc_link = &cothread_pool.current_thread.ctx;

    cothread_s *current = cothread_pool.current_thread;
    cothread_pool.current_thread = thread->thread;
    swapcontext(&current.ctx, &thread->thread.ctx);

    return;
}

void cothread_yield()
{
    cothread_s *thread = cothread_pool.current_thread;

    cothread_s *current = cothread_pool.current_thread;
    cothread_pool.current_thread = thread->thread;
    swapcontext(&current.ctx, &thread->thread.ctx.uc_link);

    return;
}

void cothread_release(cothread_t thread)
{
    struct pool_item *item = container_of(thread.thread, struct pool_item, thread);
    pool_list_remove(cothread_pool.work_list, item);
    pool_list_push(cothread_pool.free_list, item);
}
