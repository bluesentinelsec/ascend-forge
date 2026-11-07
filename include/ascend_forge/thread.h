/**
 * @file thread.h
 * @brief Start one worker and join it.
 *
 * One owner starts a worker and is the only caller that joins it. The caller
 * chooses how many workers to start. This primitive does not size a pool.
 *
 * The worker does not call SDL window, event, or renderer functions. This
 * primitive does not call them either. The first caller that opens a window,
 * pumps events, or renders does that work on the thread that initialized
 * video.
 *
 * The SDL thread handle stays inside this primitive. Callers do not read it
 * or pass it to SDL.
 */

#ifndef ASCEND_FORGE_THREAD_H
#define ASCEND_FORGE_THREAD_H

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Worker entry. The return value is the status Thread_Join reports. */
typedef SDL_ThreadFunction ThreadFn;

/**
 * One worker. Init before any other caller can reach it. Share the address.
 * Do not copy it.
 */
typedef struct Thread {
    SDL_Thread *handle;
    bool running;
} Thread;

/**
 * @brief Mark a thread as not started.
 *
 * A thread that already holds a worker is rejected and left unchanged so the
 * owner can still join it.
 *
 * @param thread Thread to initialize.
 * @return false when thread is NULL or already holds a worker.
 */
bool Thread_Init(Thread *thread);

/**
 * @brief Start one worker.
 *
 * Uses SDL_CreateThread. A failed create is logged. The thread is left as it
 * was. A thread that is already running is rejected and left running.
 *
 * @param thread Thread from Thread_Init.
 * @param fn Worker. NULL is rejected.
 * @param name Worker name. NULL lets SDL name it.
 * @param data Pointer handed to the worker. NULL is allowed.
 * @return false when the thread cannot start.
 */
bool Thread_Run(Thread *thread, ThreadFn fn, const char *name, void *data);

/**
 * @brief Wait for the worker to finish.
 *
 * Uses SDL_WaitThread. One owner calls this. The worker's return value is
 * written to status. A thread that is not running is rejected and status is
 * left untouched.
 *
 * @param status Receives the worker's return value. On failure this is left
 *        untouched.
 * @return false when the thread is not running or an argument is NULL.
 */
bool Thread_Join(Thread *thread, Sint32 *status);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_THREAD_H */
