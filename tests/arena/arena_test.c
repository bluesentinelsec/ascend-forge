/**
 * @file arena_test.c
 * @brief Self-hosted checks for the bump arena.
 */

#include "ascend_forge/arena.h"

#include "arena_tests.h"

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

static void test_rejects_zero_capacity(void)
{
    Arena *arena = NULL;
    void *bytes = NULL;

    expect_true(!Arena_Create(0u, &arena), "Arena_RejectsZeroCapacity");
    expect_true(arena != NULL, "Arena_RejectsZeroCapacity_OutIsSet");
    expect_true(!Arena_Alloc(arena, 1u, 1u, &bytes), "Arena_RejectsZeroCapacity_AllocFails");
    expect_true(bytes != NULL, "Arena_RejectsZeroCapacity_AllocOutIsSet");
    Arena_Destroy(arena);
}

static void test_create_reports_capacity(void)
{
    Arena *arena = NULL;

    expect_true(Arena_Create(64u, &arena), "Arena_CreateReportsCapacity");
    expect_true(arena != NULL, "Arena_CreateReportsCapacity_OutIsSet");
    expect_true(Arena_Capacity(arena) == 64u, "Arena_CreateReportsCapacity_Capacity");
    expect_true(Arena_Used(arena) == 0u, "Arena_CreateReportsCapacity_Used");
    Arena_Destroy(arena);
}

static void test_allocates_and_preserves_bytes(void)
{
    Arena *arena = NULL;
    Uint8 *first = NULL;
    Uint8 *second = NULL;

    expect_true(Arena_Create(32u, &arena), "Arena_AllocatesBytes_Create");
    expect_true(Arena_Alloc(arena, 1u, 1u, (void **)&first), "Arena_AllocatesBytes_First");
    first[0] = 0xA1u;
    expect_true(Arena_Alloc(arena, 1u, 1u, (void **)&second), "Arena_AllocatesBytes_Second");
    second[0] = 0xB2u;
    expect_true(first[0] == 0xA1u, "Arena_SecondAllocDoesNotOverlapFirst");
    expect_true(first != second, "Arena_AllocatesDistinctAddresses");
    Arena_Destroy(arena);
}

static void test_zero_size_does_not_consume(void)
{
    Arena *arena = NULL;
    void *bytes = NULL;

    expect_true(Arena_Create(16u, &arena), "Arena_ZeroSize_Create");
    expect_true(Arena_Alloc(arena, 0u, 8u, &bytes), "Arena_ZeroSize_Succeeds");
    expect_true(bytes != NULL, "Arena_ZeroSize_OutIsSet");
    expect_true(Arena_Used(arena) == 0u, "Arena_ZeroSize_DoesNotConsumeCapacity");
    Arena_Destroy(arena);
}

static void test_rejects_bad_alignment(void)
{
    Arena *arena = NULL;
    void *bytes = NULL;

    expect_true(Arena_Create(32u, &arena), "Arena_RejectsBadAlignment_Create");
    expect_true(!Arena_Alloc(arena, 4u, 0u, &bytes), "Arena_RejectsZeroAlignment");
    expect_true(Arena_Used(arena) == 0u, "Arena_RejectsZeroAlignment_UsedUnchanged");
    expect_true(!Arena_Alloc(arena, 4u, 3u, &bytes), "Arena_RejectsNonPowerOfTwoAlignment");
    expect_true(Arena_Used(arena) == 0u, "Arena_RejectsNonPowerOfTwoAlignment_UsedUnchanged");
    Arena_Destroy(arena);
}

static void test_aligns_to_64(void)
{
    Arena *arena = NULL;
    void *bytes = NULL;
    uintptr_t address = 0u;

    expect_true(Arena_Create(256u, &arena), "Arena_AlignsTo64_Create");
    expect_true(Arena_Alloc(arena, 1u, 1u, &bytes), "Arena_AlignsTo64_Pad");
    expect_true(Arena_Alloc(arena, 8u, 64u, &bytes), "Arena_AlignsTo64");
    address = (uintptr_t)bytes;
    expect_true((address % 64u) == 0u, "Arena_AlignsTo64_Address");
    Arena_Destroy(arena);
}

static void test_rejects_request_past_capacity(void)
{
    Arena *arena = NULL;
    void *bytes = NULL;
    Uint64 used = 0u;

    expect_true(Arena_Create(8u, &arena), "Arena_RejectsShortArena_Create");
    expect_true(Arena_Alloc(arena, 8u, 1u, &bytes), "Arena_ExactCapacitySucceeds");
    used = Arena_Used(arena);
    expect_true(!Arena_Alloc(arena, 1u, 1u, &bytes), "Arena_OneMoreByteFails");
    expect_true(Arena_Used(arena) == used, "Arena_OneMoreByteFails_UsedUnchanged");
    expect_true(bytes != NULL, "Arena_OneMoreByteFails_OutIsSet");
    Arena_Destroy(arena);
}

static void test_reset_reuses_storage(void)
{
    Arena *arena = NULL;
    void *first = NULL;
    void *second = NULL;

    expect_true(Arena_Create(16u, &arena), "Arena_Reset_Create");
    expect_true(Arena_Alloc(arena, 4u, 1u, &first), "Arena_Reset_First");
    Arena_Reset(arena);
    expect_true(Arena_Used(arena) == 0u, "Arena_Reset_Used");
    expect_true(Arena_Alloc(arena, 4u, 1u, &second), "Arena_Reset_Second");
    expect_true(first == second, "Arena_Reset_ReusesStorage");
    Arena_Destroy(arena);
}

static void test_rewind_restores_mark(void)
{
    Arena *arena = NULL;
    void *first = NULL;
    void *second = NULL;
    void *third = NULL;
    Uint64 mark = 0u;
    Uint64 past = 0u;

    expect_true(Arena_Create(32u, &arena), "Arena_Rewind_Create");
    expect_true(Arena_Alloc(arena, 4u, 1u, &first), "Arena_Rewind_First");
    mark = Arena_Mark(arena);
    expect_true(Arena_Alloc(arena, 4u, 1u, &second), "Arena_Rewind_Second");
    past = Arena_Used(arena);
    expect_true(!Arena_Rewind(arena, past + 1u), "Arena_RejectsRewindPastUsed");
    expect_true(Arena_Used(arena) == past, "Arena_RejectsRewindPastUsed_Unchanged");
    expect_true(Arena_Rewind(arena, mark), "Arena_RewindRestoresMark");
    expect_true(Arena_Used(arena) == mark, "Arena_Rewind_Used");
    expect_true(Arena_Alloc(arena, 4u, 1u, &third), "Arena_Rewind_Third");
    expect_true(third == second, "Arena_Rewind_ReusesStorage");
    Arena_Destroy(arena);
}

static void test_null_arguments(void)
{
    expect_true(!Arena_Create(8u, NULL), "Arena_RejectsNullOutParameter");
    Arena_Destroy(NULL);
    expect_true(!Arena_Alloc(NULL, 1u, 1u, NULL), "Arena_RejectsNullAllocArguments");
}

int ArenaTests_Run(void)
{
    g_failed = 0;
    test_rejects_zero_capacity();
    test_create_reports_capacity();
    test_allocates_and_preserves_bytes();
    test_zero_size_does_not_consume();
    test_rejects_bad_alignment();
    test_aligns_to_64();
    test_rejects_request_past_capacity();
    test_reset_reuses_storage();
    test_rewind_restores_mark();
    test_null_arguments();
    return g_failed;
}

#ifndef ASCEND_FORGE_ARENA_TESTS_NO_MAIN
int main(void)
{
    const int failed = ArenaTests_Run();

    if (failed != 0) {
        fprintf(stderr, "%d arena checks failed\n", failed);
        return 1;
    }
    printf("arena checks passed\n");
    return 0;
}
#endif
