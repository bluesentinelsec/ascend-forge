/**
 * @file io_test.c
 * @brief Self-hosted checks for SDL file and path wrappers.
 */

#include "ascend_forge/io.h"

#include "io_tests.h"

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

static bool ends_with_separator(Utf8 text)
{
    unsigned char mark = 0u;

    if (text.size == 0u || text.data == NULL) {
        return false;
    }
    mark = (unsigned char)text.data[text.size - 1u];
    return mark == (unsigned char)'/' || mark == (unsigned char)'\\';
}

static bool preference_dir(Arena *arena, Utf8 *out, Result *result)
{
    const char org_text[] = "ascend-forge";
    const char app_text[] = "io-test";
    Utf8 org;
    Utf8 app;

    if (!borrow_text(org_text, sizeof(org_text) - 1u, &org) ||
        !borrow_text(app_text, sizeof(app_text) - 1u, &app)) {
        return false;
    }
    return Io_PrefPath(arena, org, app, out, result);
}

static bool file_in_preferences(Arena *arena, const char *name, size_t name_size, Utf8 *out, Result *result)
{
    Utf8 dir;
    Utf8 leaf;

    if (!preference_dir(arena, &dir, result)) {
        return false;
    }
    if (!borrow_text(name, name_size, &leaf)) {
        return false;
    }
    return Io_JoinPath(arena, dir, leaf, out, result);
}

static void test_empty_file_round_trips(void)
{
    Arena *arena = NULL;
    const char name[] = "empty.bin";
    Utf8 path;
    Result result;
    Bytes loaded;

    loaded.data = NULL;
    loaded.size = 4u;
    result = Result_Ok();
    expect_true(Arena_Create(65536u, &arena), "Io_EmptyFile_Create");
    expect_true(file_in_preferences(arena, name, sizeof(name) - 1u, &path, &result), "Io_EmptyFile_Path");
    expect_true(Io_WriteFile(arena, path, Bytes_Empty(), &result), "Io_EmptyFile_Write");
    expect_true(result.ok, "Io_EmptyFile_WriteOk");
    expect_true(Io_ReadFile(arena, path, &loaded, &result), "Io_EmptyFile_Read");
    expect_true(result.ok, "Io_EmptyFile_ReadOk");
    expect_true(loaded.data != NULL, "Io_EmptyFile_Pointer");
    expect_true(loaded.size == 0u, "Io_EmptyFile_Size");
    Arena_Destroy(arena);
}

static void test_missing_file_leaves_output_untouched(void)
{
    Arena *arena = NULL;
    const char name[] = "missing.bin";
    const Uint8 sentinel = 0x5Au;
    Utf8 path;
    Utf8 actual;
    Result result;
    Bytes loaded;

    loaded.data = &sentinel;
    loaded.size = 7u;
    result = Result_Ok();
    expect_true(Arena_Create(65536u, &arena), "Io_MissingFile_Create");
    expect_true(file_in_preferences(arena, name, sizeof(name) - 1u, &path, &result), "Io_MissingFile_Path");
    expect_true(!Io_ReadFile(arena, path, &loaded, &result), "Io_MissingFile");
    expect_true(loaded.data == &sentinel, "Io_MissingFile_Pointer");
    expect_true(loaded.size == 7u, "Io_MissingFile_Size");
    expect_true(!result.ok, "Io_MissingFile_NotOk");
    expect_true(result.code == (Uint32)IoCode_Missing, "Io_MissingFile_Code");
    expect_true(Utf8_Equal(result.expected, path), "Io_MissingFile_Expected");
    expect_true(borrow_text("missing", sizeof("missing") - 1u, &actual), "Io_MissingFile_ActualBorrow");
    expect_true(Utf8_Equal(result.actual, actual), "Io_MissingFile_Actual");
    Arena_Destroy(arena);
}

static void test_bytes_round_trip(void)
{
    Arena *arena = NULL;
    const char name[] = "round.bin";
    const Uint8 raw[5] = {0x66u, 0x6Fu, 0x72u, 0x67u, 0x65u};
    Utf8 path;
    Result result;
    Bytes stored;
    Bytes loaded;

    stored = Bytes_Empty();
    loaded = Bytes_Empty();
    result = Result_Ok();
    expect_true(Arena_Create(65536u, &arena), "Io_RoundTrip_Create");
    expect_true(Bytes_Borrow(raw, 5u, &stored), "Io_RoundTrip_Borrow");
    expect_true(file_in_preferences(arena, name, sizeof(name) - 1u, &path, &result), "Io_RoundTrip_Path");
    expect_true(Io_WriteFile(arena, path, stored, &result), "Io_RoundTrip_Write");
    expect_true(Io_ReadFile(arena, path, &loaded, &result), "Io_RoundTrip_Read");
    expect_true(Bytes_Equal(stored, loaded), "Io_RoundTrip");
    Arena_Destroy(arena);
}

static void test_join_inserts_one_separator(void)
{
    Arena *arena = NULL;
    const char left_text[] = "plugin";
    const char right_text[] = "name";
    Utf8 left;
    Utf8 right;
    Utf8 joined;
    Result result;
    unsigned char mark = 0u;

    joined = Utf8_Empty();
    result = Result_Ok();
    expect_true(Arena_Create(256u, &arena), "Io_JoinInsertsSeparator_Create");
    expect_true(borrow_text(left_text, sizeof(left_text) - 1u, &left), "Io_JoinInsertsSeparator_Left");
    expect_true(borrow_text(right_text, sizeof(right_text) - 1u, &right), "Io_JoinInsertsSeparator_Right");
    expect_true(Io_JoinPath(arena, left, right, &joined, &result), "Io_JoinInsertsSeparator");
    expect_true(joined.size == 11u, "Io_JoinInsertsSeparator_Size");
    expect_true(Utf8_HasPrefix(joined, left), "Io_JoinInsertsSeparator_Prefix");
    expect_true(Utf8_HasSuffix(joined, right), "Io_JoinInsertsSeparator_Suffix");
    if (joined.size > 6u) {
        mark = (unsigned char)joined.data[6];
    }
    expect_true(mark == (unsigned char)'/' || mark == (unsigned char)'\\', "Io_JoinInsertsSeparator_Mark");
    Arena_Destroy(arena);
}

static void test_join_keeps_existing_separator(void)
{
    Arena *arena = NULL;
    const char left_text[] = "plugin/";
    const char right_text[] = "name";
    const char expected_text[] = "plugin/name";
    Utf8 left;
    Utf8 right;
    Utf8 expected;
    Utf8 joined;
    Result result;

    joined = Utf8_Empty();
    result = Result_Ok();
    expect_true(Arena_Create(256u, &arena), "Io_JoinKeepsSeparator_Create");
    expect_true(borrow_text(left_text, sizeof(left_text) - 1u, &left), "Io_JoinKeepsSeparator_Left");
    expect_true(borrow_text(right_text, sizeof(right_text) - 1u, &right), "Io_JoinKeepsSeparator_Right");
    expect_true(borrow_text(expected_text, sizeof(expected_text) - 1u, &expected),
                "Io_JoinKeepsSeparator_Expected");
    expect_true(Io_JoinPath(arena, left, right, &joined, &result), "Io_JoinKeepsSeparator");
    expect_true(Utf8_Equal(joined, expected), "Io_JoinKeepsSeparator_Text");
    Arena_Destroy(arena);
}

static void test_pref_path_ends_with_separator(void)
{
    Arena *arena = NULL;
    Utf8 dir;
    Result result;

    dir = Utf8_Empty();
    result = Result_Ok();
    expect_true(Arena_Create(65536u, &arena), "Io_PrefPath_Create");
    expect_true(preference_dir(arena, &dir, &result), "Io_PrefPath");
    expect_true(result.ok, "Io_PrefPath_Ok");
    expect_true(dir.size > 0u, "Io_PrefPath_Size");
    expect_true(ends_with_separator(dir), "Io_PrefPath_Separator");
    Arena_Destroy(arena);
}

static void test_base_path_ends_with_separator(void)
{
    Arena *arena = NULL;
    Utf8 path;
    Result result;

    path = Utf8_Empty();
    result = Result_Ok();
    expect_true(Arena_Create(65536u, &arena), "Io_BasePath_Create");
    expect_true(Io_BasePath(arena, &path, &result), "Io_BasePath");
    expect_true(result.ok, "Io_BasePath_Ok");
    expect_true(path.size > 0u, "Io_BasePath_Size");
    expect_true(ends_with_separator(path), "Io_BasePath_Separator");
    Arena_Destroy(arena);
}

static void test_path_info_reports_file_and_absence(void)
{
    Arena *arena = NULL;
    const char name[] = "info.bin";
    const char missing_name[] = "info-missing.bin";
    const Uint8 raw[1] = {0x41u};
    Utf8 path;
    Utf8 missing;
    Result result;
    Bytes stored;
    IoPathInfo info;
    IoPathInfo absent;

    stored = Bytes_Empty();
    info.exists = false;
    info.directory = true;
    info.size = 9u;
    absent.exists = true;
    absent.directory = true;
    absent.size = 9u;
    result = Result_Ok();
    expect_true(Arena_Create(65536u, &arena), "Io_PathInfo_Create");
    expect_true(Bytes_Borrow(raw, 1u, &stored), "Io_PathInfo_Borrow");
    expect_true(file_in_preferences(arena, name, sizeof(name) - 1u, &path, &result), "Io_PathInfo_Path");
    expect_true(Io_WriteFile(arena, path, stored, &result), "Io_PathInfo_Write");
    expect_true(Io_PathInfo(arena, path, &info, &result), "Io_PathInfo_File");
    expect_true(info.exists, "Io_PathInfo_Exists");
    expect_true(!info.directory, "Io_PathInfo_NotDirectory");
    expect_true(info.size == 1u, "Io_PathInfo_Size");
    expect_true(file_in_preferences(arena, missing_name, sizeof(missing_name) - 1u, &missing, &result),
                "Io_PathInfo_MissingPath");
    expect_true(Io_PathInfo(arena, missing, &absent, &result), "Io_PathInfo_Missing");
    expect_true(result.ok, "Io_PathInfo_MissingOk");
    expect_true(!absent.exists, "Io_PathInfo_MissingExists");
    expect_true(!absent.directory, "Io_PathInfo_MissingDirectory");
    expect_true(absent.size == 0u, "Io_PathInfo_MissingSize");
    Arena_Destroy(arena);
}

int IoTests_Run(void)
{
    g_failed = 0;
    test_empty_file_round_trips();
    test_missing_file_leaves_output_untouched();
    test_bytes_round_trip();
    test_join_inserts_one_separator();
    test_join_keeps_existing_separator();
    test_pref_path_ends_with_separator();
    test_base_path_ends_with_separator();
    test_path_info_reports_file_and_absence();
    return g_failed;
}

#ifndef ASCEND_FORGE_IO_TESTS_NO_MAIN
int main(void)
{
    const int failed = IoTests_Run();

    if (failed != 0) {
        fprintf(stderr, "%d io checks failed\n", failed);
        return 1;
    }
    printf("io checks passed\n");
    return 0;
}
#endif
