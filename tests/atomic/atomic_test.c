/**
 * @file atomic_test.c
 * @brief Self-hosted checks for the atomic primitive.
 */

#include "ascend_forge/atomic.h"

#include "atomic_tests.h"

#include <stdio.h>

typedef struct Finished {
    Uint32 left;
    Uint32 right;
} Finished;

static int g_failed = 0;

static void expect_true(bool condition, const char *name)
{
    if (condition) {
        return;
    }
    fprintf(stderr, "FAIL: %s\n", name);
    g_failed += 1;
}

static void test_zero_counter(void)
{
    AtomicU32 counter = {{0u}};
    Uint32 value = 7u;

    expect_true(AtomicU32_Init(&counter, 0u), "Atomic_Zero_Init");
    expect_true(AtomicU32_Load(&counter, &value), "Atomic_Zero_Load");
    expect_true(value == 0u, "Atomic_Zero_Value");
    value = 7u;
    expect_true(!AtomicU32_Load(NULL, &value), "Atomic_Zero_Null");
    expect_true(value == 7u, "Atomic_Zero_NullUntouched");
    expect_true(!AtomicU32_Init(NULL, 1u), "Atomic_Zero_NullInit");
}

static void test_store_replaces_the_value(void)
{
    AtomicU32 counter = {{0u}};
    Uint32 previous = 7u;
    Uint32 value = 0u;

    expect_true(AtomicU32_Init(&counter, 1u), "Atomic_Store_Init");
    expect_true(AtomicU32_Store(&counter, 9u, &previous), "Atomic_Store");
    expect_true(previous == 1u, "Atomic_Store_Previous");
    expect_true(AtomicU32_Load(&counter, &value) && value == 9u, "Atomic_Store_Value");
    previous = 7u;
    expect_true(!AtomicU32_Store(NULL, 3u, &previous), "Atomic_Store_Null");
    expect_true(previous == 7u, "Atomic_Store_NullUntouched");
}

static void test_add_returns_the_previous_value(void)
{
    AtomicU32 counter = {{0u}};
    Uint32 previous = 0u;
    Uint32 value = 0u;

    expect_true(AtomicU32_Init(&counter, 3u), "Atomic_Add_Init");
    expect_true(AtomicU32_Add(&counter, -1, &previous), "Atomic_Add_First");
    expect_true(previous == 3u, "Atomic_Add_FirstPrevious");
    expect_true(AtomicU32_Load(&counter, &value) && value == 2u, "Atomic_Add_FirstValue");
    expect_true(AtomicU32_Add(&counter, -1, &previous), "Atomic_Add_Second");
    expect_true(previous == 2u, "Atomic_Add_SecondPrevious");
    expect_true(AtomicU32_Add(&counter, -1, &previous), "Atomic_Add_Last");
    expect_true(previous == 1u, "Atomic_Add_LastPrevious");
    expect_true(AtomicU32_Load(&counter, &value) && value == 0u, "Atomic_Add_LastValue");
}

static void test_compare_swap_succeeds(void)
{
    AtomicU32 counter = {{0u}};
    Uint32 value = 0u;

    expect_true(AtomicU32_Init(&counter, 1u), "Atomic_Swap_Init");
    expect_true(AtomicU32_CompareSwap(&counter, 1u, 2u), "Atomic_Swap");
    expect_true(AtomicU32_Load(&counter, &value) && value == 2u, "Atomic_Swap_Value");
}

static void test_compare_swap_leaves_the_old_value(void)
{
    AtomicU32 counter = {{0u}};
    Uint32 value = 0u;

    expect_true(AtomicU32_Init(&counter, 2u), "Atomic_SwapMiss_Init");
    expect_true(!AtomicU32_CompareSwap(&counter, 1u, 3u), "Atomic_SwapMiss");
    expect_true(AtomicU32_Load(&counter, &value) && value == 2u, "Atomic_SwapMiss_Value");
    expect_true(!AtomicU32_CompareSwap(NULL, 2u, 4u), "Atomic_SwapMiss_Null");
}

static void test_publish_and_acquire_a_finished_struct(void)
{
    Finished finished = {0u, 0u};
    void *slot = NULL;
    void *acquired = NULL;
    Finished *value = NULL;

    expect_true(AtomicPointer_Init(&slot), "Atomic_Publish_Init");
    finished.left = 4u;
    finished.right = 5u;
    expect_true(Atomic_Publish(&slot, &finished), "Atomic_Publish");
    expect_true(Atomic_Acquire(&slot, &acquired), "Atomic_Acquire");
    expect_true(acquired == &finished, "Atomic_Acquire_Pointer");
    value = (Finished *)acquired;
    expect_true(value->left == 4u && value->right == 5u, "Atomic_Acquire_Fields");
}

static void test_acquire_of_an_unpublished_slot_leaves_output_untouched(void)
{
    static char seed = 'S';
    void *slot = NULL;
    void *acquired = &seed;

    expect_true(AtomicPointer_Init(&slot), "Atomic_Unpublished_Init");
    expect_true(!Atomic_Acquire(&slot, &acquired), "Atomic_Unpublished");
    expect_true(acquired == &seed, "Atomic_Unpublished_Untouched");
}

static void test_null_publish_leaves_the_slot(void)
{
    Finished finished = {1u, 2u};
    void *slot = NULL;
    void *acquired = NULL;

    expect_true(AtomicPointer_Init(&slot), "Atomic_NullPublish_Init");
    expect_true(Atomic_Publish(&slot, &finished), "Atomic_NullPublish_Publish");
    expect_true(!Atomic_Publish(&slot, NULL), "Atomic_NullPublish");
    expect_true(Atomic_Acquire(&slot, &acquired), "Atomic_NullPublish_Acquire");
    expect_true(acquired == &finished, "Atomic_NullPublish_Value");
    acquired = &finished;
    expect_true(!Atomic_Acquire(NULL, &acquired), "Atomic_NullPublish_NullSlot");
    expect_true(acquired == &finished, "Atomic_NullPublish_Untouched");
}

int AtomicTests_Run(void)
{
    g_failed = 0;
    test_zero_counter();
    test_store_replaces_the_value();
    test_add_returns_the_previous_value();
    test_compare_swap_succeeds();
    test_compare_swap_leaves_the_old_value();
    test_publish_and_acquire_a_finished_struct();
    test_acquire_of_an_unpublished_slot_leaves_output_untouched();
    test_null_publish_leaves_the_slot();
    return g_failed;
}

#ifndef ASCEND_FORGE_ATOMIC_TESTS_NO_MAIN
int main(void)
{
    const int failed = AtomicTests_Run();

    if (failed != 0) {
        fprintf(stderr, "%d atomic checks failed\n", failed);
        return 1;
    }
    printf("atomic checks passed\n");
    return 0;
}
#endif
