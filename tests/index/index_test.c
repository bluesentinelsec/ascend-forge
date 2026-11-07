/**
 * @file index_test.c
 * @brief Self-hosted checks for the index-range primitive.
 */

#include "ascend_forge/index.h"

#include "index_tests.h"

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

static void count_call(Uint32 index, void *data)
{
    Uint32 *calls = NULL;

    (void)index;
    calls = (Uint32 *)data;
    *calls += 1u;
}

typedef struct MarkedSlots {
    Uint32 *slots;
    Uint32 marker;
} MarkedSlots;

static void write_marker(Uint32 index, void *data)
{
    MarkedSlots *marked = NULL;

    marked = (MarkedSlots *)data;
    marked->slots[index] = marked->marker;
}

static void test_end_before_begin_leaves_the_range_untouched(void)
{
    IndexRange range = {9u, 8u};

    expect_true(!IndexRange_Init(&range, 3u, 1u), "IndexRange_EndBeforeBegin");
    expect_true(range.begin == 9u && range.end == 8u, "IndexRange_EndBeforeBegin_Untouched");
    expect_true(!IndexRange_Init(NULL, 0u, 1u), "IndexRange_Init_Null");
}

static void test_null_arguments_leave_outputs_untouched(void)
{
    IndexRange range = {0u, 0u};
    IndexRange parts[1] = {{4u, 5u}};
    Uint32 count = 11u;
    Uint32 calls = 0u;
    bool contains = true;

    expect_true(IndexRange_Init(&range, 0u, 1u), "IndexRange_Null_Init");
    expect_true(!IndexRange_Count(NULL, &count), "IndexRange_Count_NullRange");
    expect_true(count == 11u, "IndexRange_Count_NullRange_Untouched");
    expect_true(!IndexRange_Count(&range, NULL), "IndexRange_Count_NullOut");
    expect_true(!IndexRange_Contains(NULL, 0u, &contains), "IndexRange_Contains_NullRange");
    expect_true(contains, "IndexRange_Contains_NullRange_Untouched");
    expect_true(!IndexRange_Contains(&range, 0u, NULL), "IndexRange_Contains_NullOut");
    expect_true(!IndexRange_ForEach(NULL, count_call, &calls), "IndexRange_ForEach_NullRange");
    expect_true(calls == 0u, "IndexRange_ForEach_NullRange_Calls");
    expect_true(!IndexRange_ForEach(&range, NULL, &calls), "IndexRange_ForEach_NullFn");
    expect_true(calls == 0u, "IndexRange_ForEach_NullFn_Calls");
    expect_true(!IndexRange_Split(NULL, parts, 1u), "IndexRange_Split_NullRange");
    expect_true(parts[0].begin == 4u && parts[0].end == 5u, "IndexRange_Split_NullRange_Untouched");
    expect_true(!IndexRange_Split(&range, NULL, 1u), "IndexRange_Split_NullParts");
}

static void test_empty_range_does_nothing(void)
{
    IndexRange range = {0u, 0u};
    Uint32 count = 9u;
    Uint32 calls = 3u;
    bool contains = true;

    expect_true(IndexRange_Init(&range, 4u, 4u), "IndexRange_Empty_Init");
    expect_true(IndexRange_Count(&range, &count), "IndexRange_Empty_Count");
    expect_true(count == 0u, "IndexRange_Empty_Zero");
    expect_true(IndexRange_ForEach(&range, count_call, &calls), "IndexRange_Empty_Walk");
    expect_true(calls == 3u, "IndexRange_Empty_Calls");
    expect_true(IndexRange_Contains(&range, 4u, &contains), "IndexRange_Empty_Contains");
    expect_true(!contains, "IndexRange_Empty_Outside");
}

static void test_one_element_writes_its_slot(void)
{
    IndexRange range = {0u, 0u};
    Uint32 slots[2] = {7u, 8u};
    MarkedSlots marked = {slots, 3u};
    bool contains_begin = false;
    bool contains_end = true;

    expect_true(IndexRange_Init(&range, 0u, 1u), "IndexRange_One_Init");
    expect_true(IndexRange_ForEach(&range, write_marker, &marked), "IndexRange_One_Walk");
    expect_true(slots[0] == 3u, "IndexRange_One_Slot");
    expect_true(slots[1] == 8u, "IndexRange_One_Neighbor");
    expect_true(IndexRange_Contains(&range, 0u, &contains_begin), "IndexRange_One_Begin");
    expect_true(contains_begin, "IndexRange_One_BeginInside");
    expect_true(IndexRange_Contains(&range, 1u, &contains_end), "IndexRange_One_End");
    expect_true(!contains_end, "IndexRange_One_EndOutside");
}

static void test_two_parts_do_not_overlap(void)
{
    IndexRange range = {0u, 0u};
    IndexRange parts[2] = {{0u, 0u}, {0u, 0u}};
    Uint32 slots[5] = {0u, 0u, 0u, 0u, 0u};
    MarkedSlots first = {slots, 1u};
    MarkedSlots second = {slots, 2u};
    Uint32 first_count = 0u;
    Uint32 second_count = 0u;
    Uint32 index = 0u;

    expect_true(IndexRange_Init(&range, 0u, 5u), "IndexRange_Two_Init");
    expect_true(IndexRange_Split(&range, parts, 2u), "IndexRange_Two_Split");
    expect_true(parts[0].begin == 0u, "IndexRange_Two_FirstBegin");
    expect_true(parts[0].end == parts[1].begin, "IndexRange_Two_Meet");
    expect_true(parts[1].end == 5u, "IndexRange_Two_LastEnd");
    expect_true(IndexRange_Count(&parts[0], &first_count), "IndexRange_Two_FirstCount");
    expect_true(IndexRange_Count(&parts[1], &second_count), "IndexRange_Two_SecondCount");
    expect_true(first_count + second_count == 5u, "IndexRange_Two_Cover");
    expect_true(first_count == 3u && second_count == 2u, "IndexRange_Two_Remainder");
    expect_true(IndexRange_ForEach(&parts[0], write_marker, &first), "IndexRange_Two_WriteFirst");
    expect_true(IndexRange_ForEach(&parts[1], write_marker, &second), "IndexRange_Two_WriteSecond");
    for (index = 0u; index < 5u; index += 1u) {
        const Uint32 expected = (index < 3u) ? 1u : 2u;

        expect_true(slots[index] == expected, "IndexRange_Two_Slot");
    }
}

static void test_split_rejects_zero_parts(void)
{
    IndexRange range = {0u, 0u};
    IndexRange parts[1] = {{6u, 7u}};

    expect_true(IndexRange_Init(&range, 0u, 4u), "IndexRange_ZeroParts_Init");
    expect_true(!IndexRange_Split(&range, parts, 0u), "IndexRange_ZeroParts");
    expect_true(parts[0].begin == 6u && parts[0].end == 7u, "IndexRange_ZeroParts_Untouched");
}

int IndexRangeTests_Run(void)
{
    g_failed = 0;
    test_end_before_begin_leaves_the_range_untouched();
    test_null_arguments_leave_outputs_untouched();
    test_empty_range_does_nothing();
    test_one_element_writes_its_slot();
    test_two_parts_do_not_overlap();
    test_split_rejects_zero_parts();
    return g_failed;
}

#ifndef ASCEND_FORGE_INDEX_TESTS_NO_MAIN
int main(void)
{
    const int failed = IndexRangeTests_Run();

    if (failed != 0) {
        fprintf(stderr, "%d index checks failed\n", failed);
        return 1;
    }
    printf("index checks passed\n");
    return 0;
}
#endif
