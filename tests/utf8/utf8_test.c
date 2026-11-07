/**
 * @file utf8_test.c
 * @brief Self-hosted checks for borrowed bytes and UTF-8 text.
 */

#include "ascend_forge/utf8.h"

#include "utf8_tests.h"

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

static bool borrow_bytes(const Uint8 *data, Uint64 size, Bytes *out)
{
    return Bytes_Borrow(data, size, out);
}

static bool borrow_text(const char *data, Uint64 size, Utf8 *out)
{
    return Utf8_Borrow(data, size, out);
}

static void test_bytes_empty_has_a_pointer(void)
{
    const Bytes empty = Bytes_Empty();

    expect_true(empty.data != NULL, "Bytes_EmptyHasAPointer");
    expect_true(empty.size == 0u, "Bytes_EmptyHasAPointer_Size");
}

static void test_bytes_borrow_keeps_one_byte(void)
{
    const Uint8 byte = 0x41u;
    Bytes text;

    expect_true(borrow_bytes(&byte, 1u, &text), "Bytes_BorrowKeepsOneByte");
    expect_true(text.data == &byte, "Bytes_BorrowKeepsOneByte_Pointer");
    expect_true(text.size == 1u, "Bytes_BorrowKeepsOneByte_Size");
    expect_true(text.data[0] == 0x41u, "Bytes_BorrowKeepsOneByte_Value");
}

static void test_bytes_equal_matches_bytes(void)
{
    const Uint8 left_bytes[] = {0x00u, 0x01u};
    const Uint8 right_bytes[] = {0x00u, 0x01u};
    Bytes left;
    Bytes right;

    expect_true(borrow_bytes(left_bytes, 2u, &left), "Bytes_EqualMatchesBytes_Left");
    expect_true(borrow_bytes(right_bytes, 2u, &right), "Bytes_EqualMatchesBytes_Right");
    expect_true(Bytes_Equal(left, right), "Bytes_EqualMatchesBytes");
    expect_true(Bytes_Equal(Bytes_Empty(), Bytes_Empty()), "Bytes_EqualMatchesEmpty");
}

static void test_bytes_compare_sees_zero_byte(void)
{
    const Uint8 lower_bytes[] = {0x00u, 0x01u};
    const Uint8 higher_bytes[] = {0x00u, 0x02u};
    Bytes lower;
    Bytes higher;

    expect_true(borrow_bytes(lower_bytes, 2u, &lower), "Bytes_CompareSeesZeroByte_Lower");
    expect_true(borrow_bytes(higher_bytes, 2u, &higher), "Bytes_CompareSeesZeroByte_Higher");
    expect_true(Bytes_Compare(lower, higher) < 0, "Bytes_CompareSeesZeroByte");
    expect_true(!Bytes_Equal(lower, higher), "Bytes_CompareSeesZeroByte_NotEqual");
}

static void test_bytes_has_prefix(void)
{
    const Uint8 text_bytes[] = {0x66u, 0x6Fu, 0x72u, 0x67u, 0x65u};
    const Uint8 prefix_bytes[] = {0x66u, 0x6Fu, 0x72u};
    const Uint8 missing_bytes[] = {0x66u, 0x6Fu, 0x78u};
    Bytes text;
    Bytes prefix;
    Bytes missing;

    expect_true(borrow_bytes(text_bytes, 5u, &text), "Bytes_HasPrefix_Text");
    expect_true(borrow_bytes(prefix_bytes, 3u, &prefix), "Bytes_HasPrefix_Prefix");
    expect_true(borrow_bytes(missing_bytes, 3u, &missing), "Bytes_HasPrefix_Missing");
    expect_true(Bytes_HasPrefix(text, prefix), "Bytes_HasPrefix");
    expect_true(Bytes_HasPrefix(text, Bytes_Empty()), "Bytes_HasPrefix_Empty");
    expect_true(!Bytes_HasPrefix(text, missing), "Bytes_RejectsOtherPrefix");
}

static void test_bytes_has_suffix(void)
{
    const Uint8 text_bytes[] = {0x66u, 0x6Fu, 0x72u, 0x67u, 0x65u};
    const Uint8 suffix_bytes[] = {0x67u, 0x65u};
    Bytes text;
    Bytes suffix;

    expect_true(borrow_bytes(text_bytes, 5u, &text), "Bytes_HasSuffix_Text");
    expect_true(borrow_bytes(suffix_bytes, 2u, &suffix), "Bytes_HasSuffix_Suffix");
    expect_true(Bytes_HasSuffix(text, suffix), "Bytes_HasSuffix");
    expect_true(Bytes_HasSuffix(text, Bytes_Empty()), "Bytes_HasSuffix_Empty");
    expect_true(!Bytes_HasSuffix(suffix, text), "Bytes_RejectsLongerSuffix");
}

static void test_bytes_rejects_null(void)
{
    Bytes text = Bytes_Empty();

    expect_true(!Bytes_Borrow(NULL, 1u, &text), "Bytes_RejectsNull");
    expect_true(text.data != NULL, "Bytes_RejectsNull_OutIsSet");
    expect_true(text.size == 0u, "Bytes_RejectsNull_OutIsEmpty");
    expect_true(!Bytes_Borrow(NULL, 0u, NULL), "Bytes_RejectsNullOut");
}

static void test_utf8_empty_has_a_pointer(void)
{
    const Utf8 empty = Utf8_Empty();

    expect_true(empty.data != NULL, "Utf8_EmptyHasAPointer");
    expect_true(empty.size == 0u, "Utf8_EmptyHasAPointer_Size");
}

static void test_utf8_borrow_does_not_need_nul(void)
{
    const char raw[2] = {'a', 'b'};
    Utf8 text;

    expect_true(borrow_text(raw, 2u, &text), "Utf8_BorrowDoesNotNeedNul");
    expect_true(text.data == raw, "Utf8_BorrowDoesNotNeedNul_Pointer");
    expect_true(text.size == 2u, "Utf8_BorrowDoesNotNeedNul_Size");
    expect_true(Utf8_Equal(text, text), "Utf8_BorrowDoesNotNeedNul_Equal");
}

static void test_utf8_compare_orders_text(void)
{
    Utf8 lower;
    Utf8 higher;
    Utf8 longer;

    expect_true(borrow_text("a", 1u, &lower), "Utf8_CompareOrdersText_Lower");
    expect_true(borrow_text("b", 1u, &higher), "Utf8_CompareOrdersText_Higher");
    expect_true(borrow_text("aa", 2u, &longer), "Utf8_CompareOrdersText_Longer");
    expect_true(Utf8_Compare(lower, higher) < 0, "Utf8_CompareOrdersText");
    expect_true(Utf8_Compare(lower, longer) < 0, "Utf8_CompareOrdersShorterFirst");
    expect_true(Utf8_Equal(lower, lower), "Utf8_EqualMatchesText");
}

static void test_utf8_has_prefix(void)
{
    Utf8 text;
    Utf8 prefix;
    Utf8 missing;

    expect_true(borrow_text("forge", 5u, &text), "Utf8_HasPrefix_Text");
    expect_true(borrow_text("for", 3u, &prefix), "Utf8_HasPrefix_Prefix");
    expect_true(borrow_text("forx", 4u, &missing), "Utf8_HasPrefix_Missing");
    expect_true(Utf8_HasPrefix(text, prefix), "Utf8_HasPrefix");
    expect_true(Utf8_HasPrefix(text, Utf8_Empty()), "Utf8_HasPrefix_Empty");
    expect_true(!Utf8_HasPrefix(text, missing), "Utf8_RejectsOtherPrefix");
}

static void test_utf8_has_suffix(void)
{
    Utf8 text;
    Utf8 suffix;

    expect_true(borrow_text("forge", 5u, &text), "Utf8_HasSuffix_Text");
    expect_true(borrow_text("ge", 2u, &suffix), "Utf8_HasSuffix_Suffix");
    expect_true(Utf8_HasSuffix(text, suffix), "Utf8_HasSuffix");
    expect_true(Utf8_HasSuffix(text, Utf8_Empty()), "Utf8_HasSuffix_Empty");
    expect_true(!Utf8_HasSuffix(suffix, text), "Utf8_RejectsLongerSuffix");
}

static void test_utf8_next_reads_one_byte(void)
{
    Utf8 text;
    Uint32 codepoint = 0u;

    expect_true(borrow_text("A", 1u, &text), "Utf8_NextReadsOneByte_Borrow");
    expect_true(Utf8_Next(&text, &codepoint), "Utf8_NextReadsOneByte");
    expect_true(codepoint == 0x41u, "Utf8_NextReadsOneByte_CodePoint");
    expect_true(text.size == 0u, "Utf8_NextReadsOneByte_Consumes");
    expect_true(!Utf8_Next(&text, &codepoint), "Utf8_NextStopsWhenEmpty");
    expect_true(Utf8_CodePointCount(Utf8_Empty()) == 0u, "Utf8_EmptyHasNoCodePoints");
}

static void test_utf8_next_reports_non_utf8(void)
{
    const char raw[1] = {(char)0xFF};
    Utf8 text;
    Uint32 codepoint = 0u;

    expect_true(borrow_text(raw, 1u, &text), "Utf8_NextReportsNonUtf8_Borrow");
    expect_true(Utf8_Next(&text, &codepoint), "Utf8_NextReportsNonUtf8");
    expect_true(codepoint == SDL_INVALID_UNICODE_CODEPOINT, "Utf8_NextReportsNonUtf8_CodePoint");
    expect_true(text.size == 0u, "Utf8_NextReportsNonUtf8_ConsumesOneByte");
}

static void test_utf8_is_valid_accepts_ascii(void)
{
    Utf8 text;

    expect_true(borrow_text("A", 1u, &text), "Utf8_IsValidAcceptsAscii_Borrow");
    expect_true(Utf8_IsValid(text), "Utf8_IsValidAcceptsAscii");
    expect_true(Utf8_IsValid(Utf8_Empty()), "Utf8_IsValidAcceptsEmpty");
    expect_true(Utf8_CodePointCount(text) == 1u, "Utf8_OneByteIsOneCodePoint");
}

static void test_utf8_is_valid_rejects_non_utf8(void)
{
    const char raw[1] = {(char)0xFF};
    Utf8 text;

    expect_true(borrow_text(raw, 1u, &text), "Utf8_IsValidRejectsNonUtf8_Borrow");
    expect_true(!Utf8_IsValid(text), "Utf8_IsValidRejectsNonUtf8");
}

static void test_utf8_accepts_replacement_character(void)
{
    const char raw[3] = {(char)0xEF, (char)0xBF, (char)0xBD};
    Utf8 text;
    Uint32 codepoint = 0u;

    expect_true(borrow_text(raw, 3u, &text), "Utf8_AcceptsReplacementCharacter_Borrow");
    expect_true(Utf8_IsValid(text), "Utf8_AcceptsReplacementCharacter");
    expect_true(Utf8_Next(&text, &codepoint), "Utf8_AcceptsReplacementCharacter_Next");
    expect_true(codepoint == SDL_INVALID_UNICODE_CODEPOINT, "Utf8_AcceptsReplacementCharacter_CodePoint");
    expect_true(text.size == 0u, "Utf8_AcceptsReplacementCharacter_ConsumesThree");
}

static void test_builder_append_empty_consumes_nothing(void)
{
    Arena *arena = NULL;
    Utf8Builder builder;
    Utf8 view;

    expect_true(Arena_Create(16u, &arena), "Utf8Builder_AppendEmpty_Create");
    expect_true(Utf8Builder_Init(&builder, arena), "Utf8Builder_AppendEmpty_Init");
    expect_true(Utf8Builder_Append(&builder, Utf8_Empty()), "Utf8Builder_AppendEmpty");
    expect_true(Arena_Used(arena) == 0u, "Utf8Builder_AppendEmpty_DoesNotConsume");
    view = Utf8Builder_View(&builder);
    expect_true(view.data != NULL, "Utf8Builder_AppendEmpty_ViewPointer");
    expect_true(view.size == 0u, "Utf8Builder_AppendEmpty_ViewSize");
    Arena_Destroy(arena);
}

static void test_builder_stops_at_arena_exhaustion(void)
{
    Arena *arena = NULL;
    Utf8Builder builder;
    Utf8 first;
    Utf8 second;
    Utf8 view;

    expect_true(Arena_Create(1u, &arena), "Utf8Builder_StopsAtExhaustion_Create");
    expect_true(Utf8Builder_Init(&builder, arena), "Utf8Builder_StopsAtExhaustion_Init");
    expect_true(borrow_text("A", 1u, &first), "Utf8Builder_StopsAtExhaustion_FirstText");
    expect_true(borrow_text("B", 1u, &second), "Utf8Builder_StopsAtExhaustion_SecondText");
    expect_true(Utf8Builder_Append(&builder, first), "Utf8Builder_AppendGrowsToOneByte");
    expect_true(Arena_Used(arena) == 1u, "Utf8Builder_AppendGrowsToOneByte_Used");
    expect_true(!Utf8Builder_Append(&builder, second), "Utf8Builder_AppendStopsAtArenaExhaustion");
    expect_true(Arena_Used(arena) == 1u, "Utf8Builder_AppendStopsAtArenaExhaustion_Used");
    view = Utf8Builder_View(&builder);
    expect_true(view.size == 1u, "Utf8Builder_FailedAppendLeavesSize");
    expect_true(view.data[0] == 'A', "Utf8Builder_FailedAppendLeavesText");
    Arena_Destroy(arena);
}

static void test_builder_cstring_adds_nul(void)
{
    Arena *arena = NULL;
    Utf8Builder builder;
    Utf8 source;
    Utf8 view;
    const char *terminated = NULL;

    expect_true(Arena_Create(16u, &arena), "Utf8Builder_CString_Create");
    expect_true(Utf8Builder_Init(&builder, arena), "Utf8Builder_CString_Init");
    expect_true(borrow_text("hi", 2u, &source), "Utf8Builder_CString_Source");
    expect_true(Utf8Builder_Append(&builder, source), "Utf8Builder_CString_Append");
    view = Utf8Builder_View(&builder);
    expect_true(Utf8Builder_CString(&builder, &terminated), "Utf8Builder_CStringAddsNul");
    expect_true(terminated != NULL, "Utf8Builder_CStringAddsNul_Pointer");
    expect_true(terminated != view.data, "Utf8Builder_CStringDoesNotTerminateTheView");
    expect_true(terminated[0] == 'h', "Utf8Builder_CStringAddsNul_First");
    expect_true(terminated[1] == 'i', "Utf8Builder_CStringAddsNul_Second");
    expect_true(terminated[2] == '\0', "Utf8Builder_CStringAddsNul_Terminator");
    expect_true(view.size == 2u, "Utf8Builder_CStringLeavesViewSize");
    Arena_Destroy(arena);
}

static void test_builder_rejects_null_arena(void)
{
    Utf8Builder builder;
    const char *terminated = NULL;

    expect_true(!Utf8Builder_Init(NULL, NULL), "Utf8Builder_RejectsNullBuilder");
    expect_true(!Utf8Builder_Init(&builder, NULL), "Utf8Builder_RejectsNullArena");
    expect_true(!Utf8Builder_Append(&builder, Utf8_Empty()), "Utf8Builder_RejectsAppendWithoutArena");
    expect_true(!Utf8Builder_CString(&builder, &terminated), "Utf8Builder_RejectsCStringWithoutArena");
    expect_true(terminated != NULL, "Utf8Builder_RejectsCStringWithoutArena_OutIsSet");
}

int Utf8Tests_Run(void)
{
    g_failed = 0;
    test_bytes_empty_has_a_pointer();
    test_bytes_borrow_keeps_one_byte();
    test_bytes_equal_matches_bytes();
    test_bytes_compare_sees_zero_byte();
    test_bytes_has_prefix();
    test_bytes_has_suffix();
    test_bytes_rejects_null();
    test_utf8_empty_has_a_pointer();
    test_utf8_borrow_does_not_need_nul();
    test_utf8_compare_orders_text();
    test_utf8_has_prefix();
    test_utf8_has_suffix();
    test_utf8_next_reads_one_byte();
    test_utf8_next_reports_non_utf8();
    test_utf8_is_valid_accepts_ascii();
    test_utf8_is_valid_rejects_non_utf8();
    test_utf8_accepts_replacement_character();
    test_builder_append_empty_consumes_nothing();
    test_builder_stops_at_arena_exhaustion();
    test_builder_cstring_adds_nul();
    test_builder_rejects_null_arena();
    return g_failed;
}

#ifndef ASCEND_FORGE_UTF8_TESTS_NO_MAIN
int main(void)
{
    const int failed = Utf8Tests_Run();

    if (failed != 0) {
        fprintf(stderr, "%d utf8 checks failed\n", failed);
        return 1;
    }
    printf("utf8 checks passed\n");
    return 0;
}
#endif
