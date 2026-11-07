/**
 * @file result_test.c
 * @brief Self-hosted checks for the shared result type.
 */

#include "ascend_forge/result.h"

#include "result_tests.h"

#include <stddef.h>
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

static bool borrow_text(const char *data, size_t size, Utf8 *out)
{
    return Utf8_Borrow(data, (Uint64)size, out);
}

static bool same_result(Result left, Result right)
{
    return left.ok == right.ok && left.code == right.code &&
           left.expected.data == right.expected.data && left.expected.size == right.expected.size &&
           left.actual.data == right.actual.data && left.actual.size == right.actual.size &&
           left.next.data == right.next.data && left.next.size == right.next.size;
}

static Result seeded_result(void)
{
    Result result;

    result.ok = true;
    result.code = 99u;
    result.expected = Utf8_Empty();
    result.actual = Utf8_Empty();
    result.next = Utf8_Empty();
    return result;
}

static bool store_text(Arena *arena, const char *literal, size_t size, Utf8 *out)
{
    Utf8Builder builder;
    Utf8 piece;

    if (!Utf8Builder_Init(&builder, arena)) {
        return false;
    }
    if (!borrow_text(literal, size, &piece)) {
        return false;
    }
    if (!Utf8Builder_Append(&builder, piece)) {
        return false;
    }
    *out = Utf8Builder_View(&builder);
    return out->data != NULL && out->size == (Uint64)size;
}

/**
 * A callee that documents the failure contract: an empty name is rejected
 * and the output name is left untouched.
 */
static bool accept_name(Utf8 text, Utf8 *name, Result *result)
{
    static const char expected_text[] = "plugin name";
    static const char next_text[] = "pass a plugin name";
    Utf8 expected;
    Utf8 next;

    if (text.size > 0u) {
        *name = text;
        *result = Result_Ok();
        return true;
    }
    if (!borrow_text(expected_text, sizeof(expected_text) - 1u, &expected) ||
        !borrow_text(next_text, sizeof(next_text) - 1u, &next)) {
        return false;
    }
    if (!Result_Fail(1u, expected, Utf8_Empty(), next, result)) {
        return false;
    }
    return false;
}

static void test_ok_is_success_with_empty_text(void)
{
    const Result result = Result_Ok();

    expect_true(result.ok, "Result_OkIsSuccess");
    expect_true(result.code == 0u, "Result_OkCodeIsZero");
    expect_true(result.expected.data != NULL, "Result_OkExpectedHasAPointer");
    expect_true(result.expected.size == 0u, "Result_OkExpectedIsEmpty");
    expect_true(result.actual.data != NULL, "Result_OkActualHasAPointer");
    expect_true(result.actual.size == 0u, "Result_OkActualIsEmpty");
    expect_true(result.next.data != NULL, "Result_OkNextHasAPointer");
    expect_true(result.next.size == 0u, "Result_OkNextIsEmpty");
}

static void test_fail_borrows_static_text(void)
{
    const char expected_text[] = "plugin name";
    const char actual_text[] = "missing";
    const char next_text[] = "pass a plugin name";
    Utf8 expected;
    Utf8 actual;
    Utf8 next;
    Result result = seeded_result();

    expect_true(borrow_text(expected_text, sizeof(expected_text) - 1u, &expected),
                "Result_FailBorrowsStaticText_Expected");
    expect_true(borrow_text(actual_text, sizeof(actual_text) - 1u, &actual),
                "Result_FailBorrowsStaticText_Actual");
    expect_true(borrow_text(next_text, sizeof(next_text) - 1u, &next),
                "Result_FailBorrowsStaticText_Next");
    expect_true(Result_Fail(2u, expected, actual, next, &result), "Result_FailBorrowsStaticText");
    expect_true(!result.ok, "Result_FailBorrowsStaticText_NotOk");
    expect_true(result.code == 2u, "Result_FailBorrowsStaticText_Code");
    expect_true(result.expected.data == expected_text, "Result_FailBorrowsStaticText_ExpectedPointer");
    expect_true(result.actual.data == actual_text, "Result_FailBorrowsStaticText_ActualPointer");
    expect_true(result.next.data == next_text, "Result_FailBorrowsStaticText_NextPointer");
}

static void test_fail_does_not_allocate(void)
{
    Arena *arena = NULL;
    const char expected_text[] = "plugin name";
    const char actual_text[] = "missing";
    const char next_text[] = "pass a plugin name";
    Utf8 expected;
    Utf8 actual;
    Utf8 next;
    Result result = seeded_result();
    const Result success = Result_Ok();

    expect_true(Arena_Create(32u, &arena), "Result_FailDoesNotAllocate_Create");
    expect_true(Arena_Used(arena) == 0u, "Result_FailDoesNotAllocate_StartsEmpty");
    expect_true(success.ok, "Result_FailDoesNotAllocate_Success");
    expect_true(borrow_text(expected_text, sizeof(expected_text) - 1u, &expected),
                "Result_FailDoesNotAllocate_Expected");
    expect_true(borrow_text(actual_text, sizeof(actual_text) - 1u, &actual),
                "Result_FailDoesNotAllocate_Actual");
    expect_true(borrow_text(next_text, sizeof(next_text) - 1u, &next),
                "Result_FailDoesNotAllocate_Next");
    expect_true(Result_Fail(2u, expected, actual, next, &result), "Result_FailDoesNotAllocate");
    expect_true(Arena_Used(arena) == 0u, "Result_FailDoesNotAllocate_UsedStaysZero");
    Arena_Destroy(arena);
}

static void test_fail_reports_missing_text(void)
{
    const char expected_text[] = "plugin name";
    const char next_text[] = "pass a plugin name";
    Utf8 expected;
    Utf8 next;
    Result result = seeded_result();

    expect_true(borrow_text(expected_text, sizeof(expected_text) - 1u, &expected),
                "Result_FailReportsMissingText_Expected");
    expect_true(borrow_text(next_text, sizeof(next_text) - 1u, &next),
                "Result_FailReportsMissingText_Next");
    expect_true(Result_Fail(1u, expected, Utf8_Empty(), next, &result), "Result_FailReportsMissingText");
    expect_true(!result.ok, "Result_FailReportsMissingText_NotOk");
    expect_true(result.code == 1u, "Result_FailReportsMissingText_Code");
    expect_true(Utf8_Equal(result.expected, expected), "Result_FailReportsMissingText_Expected");
    expect_true(result.actual.data != NULL, "Result_FailReportsMissingText_ActualPointer");
    expect_true(result.actual.size == 0u, "Result_FailReportsMissingText_ActualIsEmpty");
    expect_true(Utf8_Equal(result.next, next), "Result_FailReportsMissingText_Next");
}

static void test_fail_borrows_caller_memory(void)
{
    Arena *arena = NULL;
    const char expected_text[] = "plugin name";
    const char actual_text[] = "missing";
    const char next_text[] = "pass a plugin name";
    Utf8 expected;
    Utf8 actual;
    Utf8 next;
    Result result = seeded_result();
    Uint64 used = 0u;

    expect_true(Arena_Create(64u, &arena), "Result_FailBorrowsCallerMemory_Create");
    expect_true(store_text(arena, expected_text, sizeof(expected_text) - 1u, &expected),
                "Result_FailBorrowsCallerMemory_Expected");
    expect_true(store_text(arena, actual_text, sizeof(actual_text) - 1u, &actual),
                "Result_FailBorrowsCallerMemory_Actual");
    expect_true(store_text(arena, next_text, sizeof(next_text) - 1u, &next),
                "Result_FailBorrowsCallerMemory_Next");
    used = Arena_Used(arena);
    expect_true(used > 0u, "Result_FailBorrowsCallerMemory_ArenaHoldsText");
    expect_true(Result_Fail(3u, expected, actual, next, &result), "Result_FailBorrowsCallerMemory");
    expect_true(Arena_Used(arena) == used, "Result_FailBorrowsCallerMemory_DoesNotAllocate");
    expect_true(result.expected.data == expected.data, "Result_FailBorrowsCallerMemory_ExpectedPointer");
    expect_true(result.actual.data == actual.data, "Result_FailBorrowsCallerMemory_ActualPointer");
    expect_true(result.next.data == next.data, "Result_FailBorrowsCallerMemory_NextPointer");
    Arena_Destroy(arena);
}

static void test_failure_leaves_output_untouched(void)
{
    const char sentinel[] = "sentinel";
    const char expected_text[] = "plugin name";
    Utf8 expected;
    Utf8 name;
    Result result = seeded_result();

    name.data = sentinel;
    name.size = sizeof(sentinel) - 1u;
    expect_true(!accept_name(Utf8_Empty(), &name, &result), "Result_FailureLeavesOutputUntouched");
    expect_true(name.data == sentinel, "Result_FailureLeavesOutputUntouched_Pointer");
    expect_true(name.size == sizeof(sentinel) - 1u, "Result_FailureLeavesOutputUntouched_Size");
    expect_true(!result.ok, "Result_FailureLeavesOutputUntouched_NotOk");
    expect_true(result.actual.size == 0u, "Result_FailureLeavesOutputUntouched_ActualIsEmpty");
    expect_true(borrow_text(expected_text, sizeof(expected_text) - 1u, &expected),
                "Result_FailureLeavesOutputUntouched_Borrow");
    expect_true(Utf8_Equal(result.expected, expected), "Result_FailureLeavesOutputUntouched_NamesTheField");
}

static void test_success_writes_the_name(void)
{
    const char given[] = "forge";
    Utf8 text;
    Utf8 name;
    Result result = seeded_result();

    name.data = given;
    name.size = 0u;
    expect_true(borrow_text(given, sizeof(given) - 1u, &text), "Result_SuccessWritesTheName_Borrow");
    expect_true(accept_name(text, &name, &result), "Result_SuccessWritesTheName");
    expect_true(name.data == given, "Result_SuccessWritesTheName_Pointer");
    expect_true(name.size == sizeof(given) - 1u, "Result_SuccessWritesTheName_Size");
    expect_true(result.ok, "Result_SuccessWritesTheName_Ok");
    expect_true(result.code == 0u, "Result_SuccessWritesTheName_Code");
}

static void test_fail_rejects_success_code(void)
{
    const Result before = seeded_result();
    Result result = before;

    expect_true(!Result_Fail(0u, Utf8_Empty(), Utf8_Empty(), Utf8_Empty(), &result),
                "Result_FailRejectsSuccessCode");
    expect_true(same_result(result, before), "Result_FailRejectsSuccessCode_Untouched");
}

static void test_fail_rejects_null_text(void)
{
    const Result before = seeded_result();
    Result result = before;
    Utf8 missing;

    missing.data = NULL;
    missing.size = 1u;
    expect_true(!Result_Fail(1u, missing, Utf8_Empty(), Utf8_Empty(), &result),
                "Result_FailRejectsNullExpected");
    expect_true(same_result(result, before), "Result_FailRejectsNullExpected_Untouched");
    result = before;
    expect_true(!Result_Fail(1u, Utf8_Empty(), missing, Utf8_Empty(), &result),
                "Result_FailRejectsNullActual");
    expect_true(same_result(result, before), "Result_FailRejectsNullActual_Untouched");
    result = before;
    expect_true(!Result_Fail(1u, Utf8_Empty(), Utf8_Empty(), missing, &result),
                "Result_FailRejectsNullNext");
    expect_true(same_result(result, before), "Result_FailRejectsNullNext_Untouched");
}

static void test_fail_rejects_null_out(void)
{
    expect_true(!Result_Fail(1u, Utf8_Empty(), Utf8_Empty(), Utf8_Empty(), NULL),
                "Result_FailRejectsNullOut");
}

int ResultTests_Run(void)
{
    g_failed = 0;
    test_ok_is_success_with_empty_text();
    test_fail_borrows_static_text();
    test_fail_does_not_allocate();
    test_fail_reports_missing_text();
    test_fail_borrows_caller_memory();
    test_failure_leaves_output_untouched();
    test_success_writes_the_name();
    test_fail_rejects_success_code();
    test_fail_rejects_null_text();
    test_fail_rejects_null_out();
    return g_failed;
}

#ifndef ASCEND_FORGE_RESULT_TESTS_NO_MAIN
int main(void)
{
    const int failed = ResultTests_Run();

    if (failed != 0) {
        fprintf(stderr, "%d result checks failed\n", failed);
        return 1;
    }
    printf("result checks passed\n");
    return 0;
}
#endif
