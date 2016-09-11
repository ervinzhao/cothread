#ifndef COTHREAD_H
#define COTHREAD_H

#include <ucontext.h>

#ifdef __cplusplus
extern "C" { 
#endif

    typedef void (*cothread_fun_t)(void *);

    struct cothread_s;
    typedef struct {
        struct cothread_s *thread;
    } cothread_t;

    void cothread_init();
    cothread_t cothread_create(cothread_fun_t routine, void *arg);
    void cothread_resume(cothread_t thread);
    void cothread_yield();
    void cothread_release(cothread_t thread);

#ifdef __cplusplus
}
#endif

#endif // COTHREAD_H
