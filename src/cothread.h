#ifndef COTHREAD_H
#define COTHREAD_H

#include <ucontext.h>

#ifdef __cplusplus
extern "C" { 
#endif

    struct cothread_s;
    typedef struct {
        cothread_s *thread;
    } cothread_t;

    void cothread_init();
    cothread_t cothread_create();
    void cothread_resume(cothread_t thread);
    void cothread_yield();
    void cothread_release(cothread_t thread);

#ifdef __cplusplus
}
#endif

#endif // COTHREAD_H
