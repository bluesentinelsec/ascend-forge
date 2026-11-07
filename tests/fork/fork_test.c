/**
 * @file fork_test.c
 * @brief Self-hosted checks for the fork-join primitive.
 *
 * The same call runs on every platform. Desktop and Android start workers.
 * Web walks each part on the caller.
 */

#include "ascend_forge/fork.h"

#include "fork_tests.h"

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

typedef struct MarkedSlots {
    Uint32 *slots;
    SDL_ThreadID *ids;
    Uint32 marker;
} MarkedSlots;

static void write_marker(Uint32 index, void *data)
{
    MarkedSlots *marked = NULL;

    marked = (MarkedSlots *)data;
    marked->slots[index] = marked->marker;
    if (marked->ids != NULL) {
        marked->ids[index] = SDL_GetCurrentThreadID();
    }
}

static void count_call(Uint32 index, void *data)
{
    Uint32 *calls = NULL;

    (void)index;
    calls = (Uint32 *)data;
    *calls += 1u;
}

static void test_zero_workers_leaves_the_caller_untouched(void)
{
    Thread worker = {NULL, true};
    IndexRange range = {0u, 0u};
    Uint32 calls = 4u;

    expect_true(IndexRange_Init(&range, 0u, 2u), "ForkJoin_Zero_Init");
    expect_true(!ForkJoin_Run(&worker, 0u, &range, count_call, &calls), "ForkJoin_Zero");
    expect_true(calls == 4u, "ForkJoin_Zero_Calls");
    expect_true(worker.handle == NULL && worker.running, "ForkJoin_Zero_Thread");
    expect_true(!ForkJoin_Run(NULL, 1u, &range, count_call, &calls), "ForkJoin_NullWorkers");
    expect_true(calls == 4u, "ForkJoin_NullWorkers_Calls");
    expect_true(!ForkJoin_Run(&worker, 1u, NULL, count_call, &calls), "ForkJoin_NullRange");
    expect_true(!ForkJoin_Run(&worker, 1u, &range, NULL, &calls), "ForkJoin_NullFn");
    expect_true(calls == 4u, "ForkJoin_NullFn_Calls");
}

static void test_broken_range_leaves_slots_untouched(void)
{
    Thread worker = {NULL, false};
    IndexRange range = {4u, 1u};
    Uint32 slots[1] = {9u};
    MarkedSlots marked = {slots, NULL, 1u};

    expect_true(!ForkJoin_Run(&worker, 1u, &range, write_marker, &marked), "ForkJoin_Broken");
    expect_true(slots[0] == 9u, "ForkJoin_Broken_Slot");
    expect_true(worker.handle == NULL && !worker.running, "ForkJoin_Broken_Thread");
}

static void test_empty_range_does_nothing(void)
{
    Thread workers[2] = {{NULL, false}, {NULL, false}};
    IndexRange range = {0u, 0u};
    Uint32 calls = 6u;

    expect_true(IndexRange_Init(&range, 2u, 2u), "ForkJoin_Empty_Init");
    expect_true(ForkJoin_Run(workers, 2u, &range, count_call, &calls), "ForkJoin_Empty");
    expect_true(calls == 6u, "ForkJoin_Empty_Calls");
}

static void test_two_workers_do_not_overlap(void)
{
    Thread workers[2] = {{NULL, false}, {NULL, false}};
    IndexRange range = {0u, 0u};
    Uint32 slots[5] = {0u, 0u, 0u, 0u, 0u};
    SDL_ThreadID ids[5] = {0u, 0u, 0u, 0u, 0u};
    MarkedSlots marked = {slots, ids, 1u};
    const SDL_ThreadID main_id = SDL_GetCurrentThreadID();
    Uint32 index = 0u;

    expect_true(IndexRange_Init(&range, 0u, 5u), "ForkJoin_Two_Init");
    expect_true(ForkJoin_Run(workers, 2u, &range, write_marker, &marked), "ForkJoin_Two");
    for (index = 0u; index < 5u; index += 1u) {
        expect_true(slots[index] == 1u, "ForkJoin_Two_Slot");
    }
#ifndef __EMSCRIPTEN__
    expect_true(ids[0] != main_id && ids[3] != main_id, "ForkJoin_Two_OffMain");
    expect_true(ids[0] == ids[1] && ids[1] == ids[2], "ForkJoin_Two_FirstWorker");
    expect_true(ids[3] == ids[4], "ForkJoin_Two_SecondWorker");
    expect_true(ids[0] != ids[3], "ForkJoin_Two_DifferentWorkers");
#else
    expect_true(ids[0] == main_id && ids[3] == main_id, "ForkJoin_Two_OnCaller");
#endif
}

static void test_empty_parts_still_complete(void)
{
    Thread workers[3] = {{NULL, false}, {NULL, false}, {NULL, false}};
    IndexRange range = {0u, 0u};
    Uint32 slots[2] = {5u, 6u};
    MarkedSlots marked = {slots, NULL, 4u};

    expect_true(IndexRange_Init(&range, 0u, 1u), "ForkJoin_Tail_Init");
    expect_true(ForkJoin_Run(workers, 3u, &range, write_marker, &marked), "ForkJoin_Tail");
    expect_true(slots[0] == 4u, "ForkJoin_Tail_Slot");
    expect_true(slots[1] == 6u, "ForkJoin_Tail_Neighbor");
}

int ForkJoinTests_Run(void)
{
    g_failed = 0;
    test_zero_workers_leaves_the_caller_untouched();
    test_broken_range_leaves_slots_untouched();
    test_empty_range_does_nothing();
    test_two_workers_do_not_overlap();
    test_empty_parts_still_complete();
    return g_failed;
}

#ifndef ASCEND_FORGE_FORK_TESTS_NO_MAIN
int main(void)
{
    const int failed = ForkJoinTests_Run();

    if (failed != 0) {
        fprintf(stderr, "%d fork checks failed\n", failed);
        return 1;
    }
    printf("fork checks passed\n");
    return 0;
}
#endif
