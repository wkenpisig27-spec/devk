// process.h - Linux compatibility shim for MSVC <process.h>
// Provides _beginthreadex, _endthreadex, _getpid mappings using pthreads
#pragma once
#ifndef _PROCESS_H_LINUX_COMPAT
#define _PROCESS_H_LINUX_COMPAT

#include <pthread.h>
#include <unistd.h>
#include <cstdlib>
#include <cstdint>

// _beginthreadex callback type: unsigned (__stdcall*)(void*)
// On Linux, __stdcall is defined as empty in platform_compat.h
typedef unsigned (*_beginthreadex_proc_type)(void*);

struct _ThreadStartInfo {
    _beginthreadex_proc_type proc;
    void* arg;
};

static inline void* _thread_start_routine(void* arg) {
    _ThreadStartInfo* info = (_ThreadStartInfo*)arg;
    _beginthreadex_proc_type proc = info->proc;
    void* userarg = info->arg;
    free(info);
    unsigned ret = proc(userarg);
    return (void*)(uintptr_t)ret;
}

static inline uintptr_t _beginthreadex(
    void* security, unsigned stack_size,
    _beginthreadex_proc_type start_address,
    void* arglist, unsigned initflag, unsigned* thrdaddr)
{
    (void)security; (void)initflag;

    pthread_t th;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (stack_size > 0) pthread_attr_setstacksize(&attr, stack_size);

    _ThreadStartInfo* info = (_ThreadStartInfo*)malloc(sizeof(_ThreadStartInfo));
    if (!info) { pthread_attr_destroy(&attr); return 0; }
    info->proc = start_address;
    info->arg = arglist;

    int ret = pthread_create(&th, &attr, _thread_start_routine, info);
    pthread_attr_destroy(&attr);

    if (ret != 0) {
        free(info);
        return 0;
    }

    if (thrdaddr) *thrdaddr = (unsigned)(uintptr_t)th;
    return (uintptr_t)th;
}

static inline void _endthreadex(unsigned retval) {
    pthread_exit((void*)(uintptr_t)retval);
}

#ifndef _getpid
#define _getpid getpid
#endif

#endif // _PROCESS_H_LINUX_COMPAT
