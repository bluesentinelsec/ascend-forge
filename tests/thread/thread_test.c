/**
 * @file thread_test.c
 * @brief Self-hosted checks for the thread primitive.
 *
 * Spawn checks run where the platform can start a thread. Web stays
 * single-threaded, so those checks are compiled out there. Rejection checks
 * run everywhere.
 */

#include "ascend_forge/atomic.h"
#include "ascend_forge/thread.h"

#include "thread_tests.h"

#include <stdio.h>

static int g_failed = 0;

static void expect_true(bool condition, const char *name)
{
    if (condition) {
        return;
    }
    fprintf(stderr, "FAIL: %s\n", name);
    g_failed += 1;
}

static void test_init_clears_a_running_flag(void)
{
    Thread thread = {NULL, true};

    expect_true(Thread_Init(&thread), "Thread_Init");
    expect_true(thread.handle == NULL, "Thread_Init_Handle");
    expect_true(!thread.running, "Thread_Init_Running");
    expect_true(!Thread_Init(NULL), "Thread_Init_Null");
}

static void test_null_worker_leaves_the_thread_unstarted(void)
{
    Thread thread = {NULL, false};
    Sint32 status = 9;

    expect_true(Thread_Init(&thread), "Thread_NullFn_Init");
    expect_true(!Thread_Run(&thread, NULL, "worker", &status), "Thread_NullFn");
    expect_true(thread.handle == NULL, "Thread_NullFn_Handle");
    expect_true(!thread.running, "Thread_NullFn_Running");
    expect_true(!Thread_Join(&thread, &status), "Thread_NullFn_Join");
    expect_true(status == 9, "Thread_NullFn_Status");
    expect_true(!Thread_Run(NULL, NULL, "worker", &status), "Thread_NullFn_NullThread");
    expect_true(status == 9, "Thread_NullFn_NullThreadStatus");
}

static void test_join_before_start_leaves_status_untouched(void)
{
    Thread thread = {NULL, true};
    Sint32 status = 7;

    expect_true(Thread_Init(&thread), "Thread_EarlyJoin_Init");
    expect_true(!Thread_Join(&thread, &status), "Thread_EarlyJoin");
    expect_true(status == 7, "Thread_EarlyJoin_Status");
    expect_true(!Thread_Join(NULL, &status), "Thread_EarlyJoin_Null");
    expect_true(status == 7, "Thread_EarlyJoin_NullStatus");
}

#ifndef __EMSCRIPTEN__

typedef struct Stored {
    Sint32 value;
} Stored;

typedef struct Finished {
    Uint32 left;
    Uint32 right;
} Finished;

typedef struct PublishJob {
    Finished value;
    void *slot;
} PublishJob;

typedef struct Gate {
    AtomicU32 open;
    Sint32 value;
} Gate;

static int SDLCALL thread_storeValue(void *data)
{
    Stored *stored = NULL;

    if (data == NULL) {
        return 1;
    }
    stored = (Stored *)data;
    stored->value = 42;
    return 0;
}

static int SDLCALL thread_publishValue(void *data)
{
    PublishJob *job = NULL;

    if (data == NULL) {
        return 1;
    }
    job = (PublishJob *)data;
    job->value.left = 4u;
    job->value.right = 5u;
    if (!Atomic_Publish(&job->slot, &job->value)) {
        return 2;
    }
    return 0;
}

static int SDLCALL thread_waitForGate(void *data)
{
    Gate *gate = NULL;
    Uint32 open = 0u;

    if (data == NULL) {
        return 1;
    }
    gate = (Gate *)data;
    for (;;) {
        if (!AtomicU32_Load(&gate->open, &open)) {
            return 1;
        }
        if (open != 0u) {
            break;
        }
    }
    gate->value = 11;
    return 0;
}

static void test_worker_stores_a_value_the_join_can_see(void)
{
    Thread thread = {NULL, false};
    Stored stored = {0};
    Sint32 status = 7;

    expect_true(Thread_Init(&thread), "Thread_Store_Init");
    expect_true(Thread_Run(&thread, thread_storeValue, "store", &stored), "Thread_Store_Run");
    expect_true(Thread_Join(&thread, &status), "Thread_Store_Join");
    expect_true(status == 0, "Thread_Store_Status");
    expect_true(stored.value == 42, "Thread_Store_Value");
    expect_true(!thread.running, "Thread_Store_Stopped");
    expect_true(thread.handle == NULL, "Thread_Store_Handle");
}

static void test_join_after_the_worker_finishes_leaves_status(void)
{
    Thread thread = {NULL, false};
    Stored stored = {0};
    Sint32 status = 0;

    expect_true(Thread_Init(&thread), "Thread_Again_Init");
    expect_true(Thread_Run(&thread, thread_storeValue, "again", &stored), "Thread_Again_Run");
    expect_true(Thread_Join(&thread, &status), "Thread_Again_Join");
    status = 7;
    expect_true(!Thread_Join(&thread, &status), "Thread_Again");
    expect_true(status == 7, "Thread_Again_Untouched");
}

static void test_worker_publishes_a_value_the_joiner_acquires(void)
{
    Thread thread = {NULL, false};
    PublishJob job = {{0u, 0u}, NULL};
    void *acquired = NULL;
    Finished *value = NULL;
    Sint32 status = 7;

    expect_true(AtomicPointer_Init(&job.slot), "Thread_Publish_Slot");
    expect_true(Thread_Init(&thread), "Thread_Publish_Init");
    expect_true(Thread_Run(&thread, thread_publishValue, "publish", &job), "Thread_Publish_Run");
    expect_true(Thread_Join(&thread, &status), "Thread_Publish_Join");
    expect_true(status == 0, "Thread_Publish_Status");
    expect_true(Atomic_Acquire(&job.slot, &acquired), "Thread_Publish_Acquire");
    expect_true(acquired == &job.value, "Thread_Publish_Pointer");
    value = (Finished *)acquired;
    expect_true(value->left == 4u && value->right == 5u, "Thread_Publish_Fields");
}

static void test_second_run_leaves_the_worker_running(void)
{
    Thread thread = {NULL, false};
    Gate gate = {{{0u}}, 0};
    Uint32 previous = 0u;
    Sint32 status = 7;

    expect_true(AtomicU32_Init(&gate.open, 0u), "Thread_Busy_Gate");
    expect_true(Thread_Init(&thread), "Thread_Busy_Init");
    expect_true(Thread_Run(&thread, thread_waitForGate, "busy", &gate), "Thread_Busy_Run");
    expect_true(!Thread_Run(&thread, thread_waitForGate, "busy", &gate), "Thread_Busy_Again");
    expect_true(thread.running, "Thread_Busy_StillRunning");
    expect_true(AtomicU32_Store(&gate.open, 1u, &previous), "Thread_Busy_Open");
    expect_true(Thread_Join(&thread, &status), "Thread_Busy_Join");
    expect_true(status == 0, "Thread_Busy_Status");
    expect_true(gate.value == 11, "Thread_Busy_Value");
}

#endif /* !__EMSCRIPTEN__ */

int ThreadTests_Run(void)
{
    g_failed = 0;
    test_init_clears_a_running_flag();
    test_null_worker_leaves_the_thread_unstarted();
    test_join_before_start_leaves_status_untouched();
#ifndef __EMSCRIPTEN__
    test_worker_stores_a_value_the_join_can_see();
    test_join_after_the_worker_finishes_leaves_status();
    test_worker_publishes_a_value_the_joiner_acquires();
    test_second_run_leaves_the_worker_running();
#endif
    return g_failed;
}

#ifndef ASCEND_FORGE_THREAD_TESTS_NO_MAIN
int main(void)
{
    const int failed = ThreadTests_Run();

    if (failed != 0) {
        fprintf(stderr, "%d thread checks failed\n", failed);
        return 1;
    }
    printf("thread checks passed\n");
    return 0;
}
#endif
