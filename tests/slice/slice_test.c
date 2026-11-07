/**
 * @file slice_test.c
 * @brief Self-hosted checks for the slice primitive.
 */

#include "ascend_forge/slice.h"

#include "slice_tests.h"

#include <stdint.h>
#include <stdio.h>

typedef struct SlicePair {
    Uint32 left;
    Uint32 right;
} SlicePair;

Slice_Define(Numbers, Uint32)
Slice_Define(Pairs, SlicePair)

static int g_failed = 0;

static void expect_true(bool condition, const char *name)
{
    if (condition) {
        return;
    }
    fprintf(stderr, "FAIL: %s\n", name);
    g_failed += 1;
}

static bool aligned(const void *data, size_t alignment)
{
    return ((uintptr_t)data % alignment) == 0u;
}

static void test_empty_slice_does_not_allocate(void)
{
    Arena *arena = NULL;
    Slice_Numbers numbers = {NULL, 0u, 0u};
    Slice_Numbers again = {NULL, 0u, 0u};
    Uint64 mark = 1u;

    numbers.count = 7u;
    numbers.capacity = 9u;
    expect_true(Arena_Create(64u, &arena), "Slice_Empty_Create");
    mark = Arena_Mark(arena);
    expect_true(Slice_Numbers_Init(&numbers), "Slice_Empty_Init");
    expect_true(numbers.data != NULL, "Slice_Empty_Pointer");
    expect_true(numbers.count == 0u, "Slice_Empty_Count");
    expect_true(numbers.capacity == 0u, "Slice_Empty_Capacity");
    expect_true(Slice_Numbers_Init(&again), "Slice_Empty_InitAgain");
    expect_true(again.data == numbers.data, "Slice_Empty_SameStorage");
    expect_true(Arena_Mark(arena) == mark, "Slice_Empty_Mark");
    expect_true(!Slice_Numbers_Remove(&numbers, 0u), "Slice_Empty_Remove");
    expect_true(numbers.count == 0u, "Slice_Empty_RemoveCount");
    expect_true(!Slice_Numbers_Init(NULL), "Slice_Empty_Null");
    Arena_Destroy(arena);
}

static void test_one_element(void)
{
    Arena *arena = NULL;
    Slice_Numbers numbers = {NULL, 0u, 0u};
    Uint32 *storage = NULL;

    expect_true(Arena_Create(256u, &arena), "Slice_One_Create");
    expect_true(Slice_Numbers_Init(&numbers), "Slice_One_Init");
    storage = numbers.data;
    expect_true(Slice_Numbers_Append(&numbers, arena, 7u), "Slice_One_Append");
    expect_true(numbers.count == 1u, "Slice_One_Count");
    expect_true(numbers.capacity == 1u, "Slice_One_Capacity");
    expect_true(numbers.data != storage, "Slice_One_Storage");
    expect_true(numbers.data[0] == 7u, "Slice_One_Value");
    expect_true(aligned(numbers.data, _Alignof(Uint32)), "Slice_One_Align");
    storage = numbers.data;
    expect_true(Slice_Numbers_Remove(&numbers, 0u), "Slice_One_Remove");
    expect_true(numbers.count == 0u, "Slice_One_RemoveCount");
    expect_true(numbers.capacity == 1u, "Slice_One_RemoveCapacity");
    expect_true(numbers.data == storage, "Slice_One_RemoveStorage");
    expect_true(!Slice_Numbers_Remove(&numbers, 0u), "Slice_One_RemoveAgain");
    expect_true(numbers.count == 0u, "Slice_One_RemoveAgainCount");
    Arena_Destroy(arena);
}

static void test_spare_capacity_does_not_allocate(void)
{
    Arena *arena = NULL;
    Slice_Numbers numbers = {NULL, 0u, 0u};
    Uint32 *storage = NULL;
    Uint64 mark = 0u;
    Uint64 count = 0u;
    Uint64 capacity = 0u;
    Uint32 guard = 0u;

    expect_true(Arena_Create(4096u, &arena), "Slice_Spare_Create");
    expect_true(Slice_Numbers_Init(&numbers), "Slice_Spare_Init");
    while (numbers.count == numbers.capacity && guard < 8u) {
        expect_true(Slice_Numbers_Append(&numbers, arena, guard), "Slice_Spare_Fill");
        guard += 1u;
    }
    expect_true(numbers.count < numbers.capacity, "Slice_Spare_Room");
    storage = numbers.data;
    capacity = numbers.capacity;
    count = numbers.count;
    mark = Arena_Mark(arena);
    expect_true(Slice_Numbers_Append(&numbers, arena, 40u), "Slice_Spare_Append");
    expect_true(numbers.data == storage, "Slice_Spare_Pointer");
    expect_true(numbers.capacity == capacity, "Slice_Spare_Capacity");
    expect_true(numbers.count == count + 1u, "Slice_Spare_Count");
    expect_true(Arena_Mark(arena) == mark, "Slice_Spare_Mark");
    expect_true(count < numbers.count && numbers.data[count] == 40u, "Slice_Spare_Value");
    Arena_Destroy(arena);
}

static void test_growth_stops_when_the_arena_is_full(void)
{
    Arena *arena = NULL;
    Slice_Numbers numbers = {NULL, 0u, 0u};
    Uint64 mark = 0u;
    Uint64 count = 0u;
    Uint64 index = 0u;
    Uint32 next = 0u;

    expect_true(Arena_Create(128u, &arena), "Slice_Growth_Create");
    expect_true(Slice_Numbers_Init(&numbers), "Slice_Growth_Init");
    mark = Arena_Mark(arena);
    while (Slice_Numbers_Append(&numbers, arena, next)) {
        mark = Arena_Mark(arena);
        next += 1u;
        if (next == 64u) {
            break;
        }
    }
    count = numbers.count;
    expect_true(count > 1u, "Slice_Growth_Grew");
    expect_true(count < 64u, "Slice_Growth_Stopped");
    expect_true(numbers.count == count, "Slice_Growth_Count");
    expect_true(Arena_Mark(arena) == mark, "Slice_Growth_Mark");
    for (index = 0u; index < count; index += 1u) {
        expect_true(numbers.data[index] == (Uint32)index, "Slice_Growth_Value");
    }
    Arena_Destroy(arena);
}

static void test_failed_append_leaves_elements(void)
{
    Arena *arena = NULL;
    Slice_Numbers numbers = {NULL, 0u, 0u};
    void *data = NULL;
    Uint64 count = 0u;
    Uint64 capacity = 0u;
    Uint64 mark = 0u;
    Uint32 kept = 4u;

    expect_true(Arena_Create(1u, &arena), "Slice_Failed_Create");
    expect_true(Slice_Numbers_Init(&numbers), "Slice_Failed_Init");
    data = numbers.data;
    count = numbers.count;
    capacity = numbers.capacity;
    mark = Arena_Mark(arena);
    expect_true(!Slice_Numbers_Append(&numbers, arena, kept), "Slice_Failed_First");
    expect_true(numbers.data == data, "Slice_Failed_FirstPointer");
    expect_true(numbers.count == count, "Slice_Failed_FirstCount");
    expect_true(numbers.capacity == capacity, "Slice_Failed_FirstCapacity");
    expect_true(Arena_Mark(arena) == mark, "Slice_Failed_FirstMark");
    Arena_Destroy(arena);
}

static void test_rejected_alignment_leaves_elements(void)
{
    Arena *arena = NULL;
    Slice_Numbers numbers = {NULL, 0u, 0u};
    void *data = NULL;
    Uint64 count = 0u;
    Uint64 capacity = 0u;
    Uint32 value = 9u;
    Uint32 kept = 4u;

    expect_true(Arena_Create(256u, &arena), "Slice_Align_Create");
    expect_true(Slice_Numbers_Init(&numbers), "Slice_Align_Init");
    expect_true(Slice_Numbers_Append(&numbers, arena, kept), "Slice_Align_Append");
    data = numbers.data;
    count = numbers.count;
    capacity = numbers.capacity;
    expect_true(!Slice_Append(&data, &count, &capacity, arena, &value, (Uint64)sizeof(Uint32), 3u),
                "Slice_Align_Reject");
    expect_true(data == numbers.data, "Slice_Align_Pointer");
    expect_true(count == numbers.count, "Slice_Align_Count");
    expect_true(capacity == numbers.capacity, "Slice_Align_Capacity");
    expect_true(numbers.data[0] == kept, "Slice_Align_Value");
    Arena_Destroy(arena);
}

static void test_second_type_remove_compacts(void)
{
    Arena *arena = NULL;
    Slice_Pairs pairs = {NULL, 0u, 0u};
    SlicePair first;
    SlicePair second;
    SlicePair third;
    SlicePair *storage = NULL;
    Uint64 mark = 0u;
    Uint64 capacity = 0u;

    first.left = 1u;
    first.right = 10u;
    second.left = 2u;
    second.right = 20u;
    third.left = 3u;
    third.right = 30u;
    expect_true(Arena_Create(4096u, &arena), "Slice_Pairs_Create");
    expect_true(Slice_Pairs_Init(&pairs), "Slice_Pairs_Init");
    expect_true(Slice_Pairs_Append(&pairs, arena, first), "Slice_Pairs_First");
    expect_true(Slice_Pairs_Append(&pairs, arena, second), "Slice_Pairs_Second");
    expect_true(Slice_Pairs_Append(&pairs, arena, third), "Slice_Pairs_Third");
    expect_true(pairs.count == 3u, "Slice_Pairs_Count");
    expect_true(aligned(pairs.data, _Alignof(SlicePair)), "Slice_Pairs_Align");
    expect_true(pairs.data[1].left == 2u && pairs.data[1].right == 20u, "Slice_Pairs_Middle");
    storage = pairs.data;
    capacity = pairs.capacity;
    mark = Arena_Mark(arena);
    expect_true(Slice_Pairs_Remove(&pairs, 1u), "Slice_Pairs_Remove");
    expect_true(pairs.count == 2u, "Slice_Pairs_RemovedCount");
    expect_true(pairs.capacity == capacity, "Slice_Pairs_RemovedCapacity");
    expect_true(pairs.data == storage, "Slice_Pairs_RemovedStorage");
    expect_true(Arena_Mark(arena) == mark, "Slice_Pairs_Mark");
    expect_true(pairs.data[0].left == 1u && pairs.data[0].right == 10u, "Slice_Pairs_KeptFirst");
    expect_true(pairs.data[1].left == 3u && pairs.data[1].right == 30u, "Slice_Pairs_KeptThird");
    expect_true(!Slice_Pairs_Remove(&pairs, 2u), "Slice_Pairs_RemovePastEnd");
    expect_true(pairs.count == 2u, "Slice_Pairs_PastEndCount");
    Arena_Destroy(arena);
}

int SliceTests_Run(void)
{
    g_failed = 0;
    test_empty_slice_does_not_allocate();
    test_one_element();
    test_spare_capacity_does_not_allocate();
    test_growth_stops_when_the_arena_is_full();
    test_failed_append_leaves_elements();
    test_rejected_alignment_leaves_elements();
    test_second_type_remove_compacts();
    return g_failed;
}

#ifndef ASCEND_FORGE_SLICE_TESTS_NO_MAIN
int main(void)
{
    const int failed = SliceTests_Run();

    if (failed != 0) {
        fprintf(stderr, "%d slice checks failed\n", failed);
        return 1;
    }
    printf("slice checks passed\n");
    return 0;
}
#endif
