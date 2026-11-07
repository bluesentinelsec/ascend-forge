/**
 * @file thread.c
 * @brief One SDL worker and the join that waits for it.
 */

#include "ascend_forge/thread.h"

bool Thread_Init(Thread *thread)
{
    if (thread == NULL) {
        SDL_Log("Thread_Init expected a thread, received NULL");
        return false;
    }
    if (thread->handle != NULL) {
        SDL_Log("Thread_Init expected a thread that has not been started");
        return false;
    }
    thread->handle = NULL;
    thread->running = false;
    return true;
}

bool Thread_Run(Thread *thread, ThreadFn fn, const char *name, void *data)
{
    SDL_Thread *handle = NULL;

    if (thread == NULL) {
        SDL_Log("Thread_Run expected a thread, received NULL");
        return false;
    }
    if (fn == NULL) {
        SDL_Log("Thread_Run expected a worker function, received NULL");
        return false;
    }
    if (thread->running || thread->handle != NULL) {
        SDL_Log("Thread_Run expected a thread that is not already running");
        return false;
    }
    /* The macro injects the platform C runtime's thread start, including
     * Windows _beginthreadex. Call it. Do not call SDL_CreateThreadRuntime. */
    handle = SDL_CreateThread(fn, name, data);
    if (handle == NULL) {
        SDL_Log("Thread_Run failed to start a worker: %s", SDL_GetError());
        return false;
    }
    thread->handle = handle;
    thread->running = true;
    return true;
}

bool Thread_Join(Thread *thread, Sint32 *status)
{
    int code = 0;

    if (thread == NULL || status == NULL) {
        SDL_Log("Thread_Join expected a thread and a status");
        return false;
    }
    if (!thread->running || thread->handle == NULL) {
        SDL_Log("Thread_Join expected a running thread");
        return false;
    }
    SDL_WaitThread(thread->handle, &code);
    thread->handle = NULL;
    thread->running = false;
    *status = (Sint32)code;
    return true;
}
