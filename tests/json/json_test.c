/**
 * @file json_test.c
 * @brief Self-hosted checks for the JSON primitive.
 */

#include "ascend_forge/json.h"

#include "json_tests.h"

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

static Json seeded_json(void)
{
    static char sentinel = 'S';
    Json value;

    value.node = &sentinel;
    return value;
}

static bool parse_text(Arena *arena, const char *data, size_t size, Json *out, Result *result)
{
    Utf8 text;

    if (!borrow_text(data, size, &text)) {
        return false;
    }
    return Json_Parse(arena, text, out, result);
}

static bool text_equals(Json value, const char *data, size_t size)
{
    Utf8 expected;
    Utf8 actual;

    actual = Utf8_Empty();
    if (!borrow_text(data, size, &expected) || !Json_Text(value, &actual)) {
        return false;
    }
    return Utf8_Equal(expected, actual);
}

static void test_empty_input_is_rejected(void)
{
    Arena *arena = NULL;
    Json root = seeded_json();
    Json seeded = root;
    Result result = Result_Ok();
    Utf8 text = Utf8_Empty();

    expect_true(Arena_Create(4096u, &arena), "Json_Empty_Create");
    expect_true(!Json_Parse(arena, text, &root, &result), "Json_Empty");
    expect_true(root.node == seeded.node, "Json_Empty_Untouched");
    expect_true(!result.ok, "Json_Empty_NotOk");
    expect_true(result.code == (Uint32)JsonCode_Invalid, "Json_Empty_Code");
    Arena_Destroy(arena);
}

static void test_empty_containers(void)
{
    Arena *arena = NULL;
    const char raw[] = "{\"a\":[],\"o\":{}}";
    Json root = seeded_json();
    Json array = seeded_json();
    Json object = seeded_json();
    Json missing = seeded_json();
    Utf8 key;
    Result result = Result_Ok();
    JsonKind kind = JsonKind_Null;
    Uint64 count = 7u;

    expect_true(Arena_Create(65536u, &arena), "Json_EmptyContainers_Create");
    expect_true(parse_text(arena, raw, sizeof(raw) - 1u, &root, &result), "Json_EmptyContainers_Parse");
    expect_true(borrow_text("a", 1u, &key), "Json_EmptyContainers_ArrayKey");
    expect_true(Json_Field(root, key, &array), "Json_EmptyContainers_Array");
    expect_true(Json_Kind(array, &kind) && kind == JsonKind_Array, "Json_EmptyContainers_ArrayKind");
    expect_true(Json_Count(array, &count) && count == 0u, "Json_EmptyContainers_ArrayCount");
    expect_true(!Json_Element(array, 0u, &missing), "Json_EmptyContainers_ArrayElement");
    expect_true(missing.node == seeded_json().node, "Json_EmptyContainers_ArrayUntouched");
    expect_true(borrow_text("o", 1u, &key), "Json_EmptyContainers_ObjectKey");
    expect_true(Json_Field(root, key, &object), "Json_EmptyContainers_Object");
    expect_true(Json_Kind(object, &kind) && kind == JsonKind_Object, "Json_EmptyContainers_ObjectKind");
    count = 7u;
    expect_true(Json_Count(object, &count) && count == 0u, "Json_EmptyContainers_ObjectCount");
    Arena_Destroy(arena);
}

static void test_nested_object(void)
{
    Arena *arena = NULL;
    const char raw[] = "{\"plugin\":{\"name\":\"forge\",\"tags\":[\"c\",\"sdl\"],\"ok\":true,\"extra\":null}}";
    Json root = seeded_json();
    Json plugin = seeded_json();
    Json tags = seeded_json();
    Json name = seeded_json();
    Json first = seeded_json();
    Json second = seeded_json();
    Json flag = seeded_json();
    Json extra = seeded_json();
    Json absent = seeded_json();
    Json pair_key = seeded_json();
    Json pair_value = seeded_json();
    Json seeded = absent;
    Utf8 key;
    Result result = Result_Ok();
    JsonKind kind = JsonKind_String;
    Uint64 count = 0u;
    bool ok = false;

    expect_true(Arena_Create(65536u, &arena), "Json_Nested_Create");
    expect_true(parse_text(arena, raw, sizeof(raw) - 1u, &root, &result), "Json_Nested_Parse");
    expect_true(result.ok, "Json_Nested_Ok");
    expect_true(Json_Kind(root, &kind) && kind == JsonKind_Object, "Json_Nested_Root");
    expect_true(borrow_text("plugin", 6u, &key), "Json_Nested_PluginKey");
    expect_true(Json_Field(root, key, &plugin), "Json_Nested_Plugin");
    expect_true(Json_Kind(plugin, &kind) && kind == JsonKind_Object, "Json_Nested_PluginKind");
    expect_true(Json_Count(plugin, &count) && count == 4u, "Json_Nested_PluginCount");
    expect_true(Json_Pair(plugin, 0u, &pair_key, &pair_value), "Json_Nested_Pair");
    expect_true(text_equals(pair_key, "name", 4u), "Json_Nested_PairKey");
    expect_true(borrow_text("name", 4u, &key), "Json_Nested_NameKey");
    expect_true(Json_Field(plugin, key, &name), "Json_Nested_Name");
    expect_true(text_equals(name, "forge", 5u), "Json_Nested_NameText");
    expect_true(borrow_text("tags", 4u, &key), "Json_Nested_TagsKey");
    expect_true(Json_Field(plugin, key, &tags), "Json_Nested_Tags");
    expect_true(Json_Count(tags, &count) && count == 2u, "Json_Nested_TagsCount");
    expect_true(Json_Element(tags, 0u, &first) && text_equals(first, "c", 1u), "Json_Nested_FirstTag");
    expect_true(Json_Element(tags, 1u, &second) && text_equals(second, "sdl", 3u), "Json_Nested_SecondTag");
    expect_true(!Json_Element(tags, 2u, &absent), "Json_Nested_TagRange");
    expect_true(absent.node == seeded.node, "Json_Nested_TagUntouched");
    expect_true(borrow_text("ok", 2u, &key), "Json_Nested_OkKey");
    expect_true(Json_Field(plugin, key, &flag), "Json_Nested_Flag");
    expect_true(Json_Bool(flag, &ok) && ok, "Json_Nested_True");
    expect_true(borrow_text("extra", 5u, &key), "Json_Nested_ExtraKey");
    expect_true(Json_Field(plugin, key, &extra), "Json_Nested_Extra");
    expect_true(Json_Kind(extra, &kind) && kind == JsonKind_Null, "Json_Nested_Null");
    expect_true(borrow_text("missing", 7u, &key), "Json_Nested_MissingKey");
    absent = seeded;
    expect_true(!Json_Field(plugin, key, &absent), "Json_Nested_Missing");
    expect_true(absent.node == seeded.node, "Json_Nested_MissingUntouched");
    Arena_Destroy(arena);
}

static void test_escapes(void)
{
    Arena *arena = NULL;
    const char raw[] = "\"a\\nb\\t\\\"c\\\\d\\u0041\"";
    const char expected[] = "a\nb\t\"c\\dA";
    Json root = seeded_json();
    Result result = Result_Ok();
    JsonKind kind = JsonKind_Null;

    expect_true(Arena_Create(65536u, &arena), "Json_Escapes_Create");
    expect_true(parse_text(arena, raw, sizeof(raw) - 1u, &root, &result), "Json_Escapes_Parse");
    expect_true(Json_Kind(root, &kind) && kind == JsonKind_String, "Json_Escapes_Kind");
    expect_true(text_equals(root, expected, sizeof(expected) - 1u), "Json_Escapes");
    Arena_Destroy(arena);
}

static void test_rejected_document_leaves_output_untouched(void)
{
    Arena *arena = NULL;
    const char raw[] = "{\"a\":";
    const char trailing[] = "[]true";
    Json root = seeded_json();
    Json seeded = root;
    Result result = Result_Ok();
    Utf8 expected;

    expect_true(Arena_Create(65536u, &arena), "Json_Rejected_Create");
    expect_true(!parse_text(arena, raw, sizeof(raw) - 1u, &root, &result), "Json_Rejected");
    expect_true(root.node == seeded.node, "Json_Rejected_Untouched");
    expect_true(!result.ok, "Json_Rejected_NotOk");
    expect_true(result.code == (Uint32)JsonCode_Invalid, "Json_Rejected_Code");
    expect_true(borrow_text("one JSON value", sizeof("one JSON value") - 1u, &expected), "Json_Rejected_ExpectedBorrow");
    expect_true(Utf8_Equal(result.expected, expected), "Json_Rejected_Expected");
    root = seeded;
    result = Result_Ok();
    expect_true(!parse_text(arena, trailing, sizeof(trailing) - 1u, &root, &result), "Json_Trailing");
    expect_true(root.node == seeded.node, "Json_Trailing_Untouched");
    expect_true(!result.ok, "Json_Trailing_NotOk");
    Arena_Destroy(arena);
}

static void test_number_keeps_source_digits(void)
{
    Arena *arena = NULL;
    const char raw[] = "{\"n\":9007199254740993,\"f\":1.2300}";
    Json root = seeded_json();
    Json number = seeded_json();
    Json fraction = seeded_json();
    Utf8 key;
    Result result = Result_Ok();
    JsonKind kind = JsonKind_Null;

    expect_true(Arena_Create(65536u, &arena), "Json_Number_Create");
    expect_true(parse_text(arena, raw, sizeof(raw) - 1u, &root, &result), "Json_Number_Parse");
    expect_true(borrow_text("n", 1u, &key), "Json_Number_Key");
    expect_true(Json_Field(root, key, &number), "Json_Number_Field");
    expect_true(Json_Kind(number, &kind) && kind == JsonKind_Number, "Json_Number_Kind");
    expect_true(text_equals(number, "9007199254740993", 16u), "Json_Number_Digits");
    expect_true(borrow_text("f", 1u, &key), "Json_Number_FractionKey");
    expect_true(Json_Field(root, key, &fraction), "Json_Number_Fraction");
    expect_true(text_equals(fraction, "1.2300", 6u), "Json_Number_FractionDigits");
    Arena_Destroy(arena);
}

static void test_stringify_round_trip(void)
{
    Arena *arena = NULL;
    const char raw[] = "{\"n\":9007199254740993,\"f\":1.2300}";
    Json root = seeded_json();
    Json again = seeded_json();
    Json number = seeded_json();
    Utf8Builder builder;
    Utf8Builder second;
    Utf8 view;
    Utf8 again_view;
    Utf8 source;
    Utf8 key;
    Result result = Result_Ok();

    view = Utf8_Empty();
    again_view = Utf8_Empty();
    expect_true(Arena_Create(65536u, &arena), "Json_RoundTrip_Create");
    expect_true(parse_text(arena, raw, sizeof(raw) - 1u, &root, &result), "Json_RoundTrip_Parse");
    expect_true(Utf8Builder_Init(&builder, arena), "Json_RoundTrip_Builder");
    expect_true(Json_Stringify(root, &builder, &result), "Json_RoundTrip_Write");
    view = Utf8Builder_View(&builder);
    expect_true(borrow_text(raw, sizeof(raw) - 1u, &source), "Json_RoundTrip_Source");
    if (!Utf8_Equal(source, view)) {
        fprintf(stderr, "JSON: %.*s\n", (int)view.size, view.data);
    }
    expect_true(Utf8_Equal(source, view), "Json_RoundTrip_Text");
    expect_true(Json_Parse(arena, view, &again, &result), "Json_RoundTrip_Reparse");
    expect_true(Utf8Builder_Init(&second, arena), "Json_RoundTrip_SecondBuilder");
    expect_true(Json_Stringify(again, &second, &result), "Json_RoundTrip_Rewrite");
    again_view = Utf8Builder_View(&second);
    expect_true(Utf8_Equal(view, again_view), "Json_RoundTrip");
    expect_true(borrow_text("n", 1u, &key), "Json_RoundTrip_Key");
    expect_true(Json_Field(again, key, &number), "Json_RoundTrip_Field");
    expect_true(text_equals(number, "9007199254740993", 16u), "Json_RoundTrip_Digits");
    Arena_Destroy(arena);
}

static void test_private_arenas_join(void)
{
    Arena *parent = NULL;
    Arena *left_arena = NULL;
    Arena *right_arena = NULL;
    const char left_raw[] = "{\"id\":1}";
    const char right_raw[] = "{\"id\":2}";
    Json left = seeded_json();
    Json right = seeded_json();
    Json joined = seeded_json();
    Json first = seeded_json();
    Json second = seeded_json();
    Json id = seeded_json();
    Utf8Builder builder;
    Utf8 view;
    Utf8 key;
    Result result = Result_Ok();
    Uint64 count = 0u;

    view = Utf8_Empty();
    expect_true(Arena_Create(65536u, &parent), "Json_Join_Parent");
    expect_true(Arena_Create(65536u, &left_arena), "Json_Join_LeftArena");
    expect_true(Arena_Create(65536u, &right_arena), "Json_Join_RightArena");
    expect_true(parse_text(left_arena, left_raw, sizeof(left_raw) - 1u, &left, &result), "Json_Join_Left");
    expect_true(parse_text(right_arena, right_raw, sizeof(right_raw) - 1u, &right, &result), "Json_Join_Right");
    expect_true(Utf8Builder_Init(&builder, parent), "Json_Join_Builder");
    expect_true(borrow_text("[", 1u, &view) && Utf8Builder_Append(&builder, view), "Json_Join_Open");
    expect_true(Json_Stringify(left, &builder, &result), "Json_Join_WriteLeft");
    expect_true(borrow_text(",", 1u, &view) && Utf8Builder_Append(&builder, view), "Json_Join_Comma");
    expect_true(Json_Stringify(right, &builder, &result), "Json_Join_WriteRight");
    expect_true(borrow_text("]", 1u, &view) && Utf8Builder_Append(&builder, view), "Json_Join_Close");
    view = Utf8Builder_View(&builder);
    expect_true(Json_Parse(parent, view, &joined, &result), "Json_Join_Parse");
    expect_true(Json_Count(joined, &count) && count == 2u, "Json_Join_Count");
    expect_true(Json_Element(joined, 0u, &first), "Json_Join_First");
    expect_true(Json_Element(joined, 1u, &second), "Json_Join_Second");
    expect_true(borrow_text("id", 2u, &key), "Json_Join_Key");
    expect_true(Json_Field(first, key, &id) && text_equals(id, "1", 1u), "Json_Join_LeftId");
    expect_true(Json_Field(second, key, &id) && text_equals(id, "2", 1u), "Json_Join_RightId");
    Arena_Destroy(right_arena);
    Arena_Destroy(left_arena);
    Arena_Destroy(parent);
}

static void test_short_arena_leaves_output_untouched(void)
{
    Arena *arena = NULL;
    const char raw[] = "{\"name\":\"forge\"}";
    Json root = seeded_json();
    Json seeded = root;
    Result result = Result_Ok();
    Utf8 actual;
    Uint64 mark = 1u;

    expect_true(Arena_Create(1u, &arena), "Json_ShortArena_Create");
    mark = Arena_Mark(arena);
    expect_true(!parse_text(arena, raw, sizeof(raw) - 1u, &root, &result), "Json_ShortArena");
    expect_true(root.node == seeded.node, "Json_ShortArena_Untouched");
    expect_true(!result.ok, "Json_ShortArena_NotOk");
    expect_true(result.code == (Uint32)JsonCode_Invalid, "Json_ShortArena_Code");
    expect_true(borrow_text("the arena could not hold the JSON value",
                            sizeof("the arena could not hold the JSON value") - 1u, &actual),
                "Json_ShortArena_ActualBorrow");
    expect_true(Utf8_Equal(result.actual, actual), "Json_ShortArena_Actual");
    expect_true(Arena_Mark(arena) == mark, "Json_ShortArena_Mark");
    Arena_Destroy(arena);
}

int JsonTests_Run(void)
{
    g_failed = 0;
    test_empty_input_is_rejected();
    test_empty_containers();
    test_nested_object();
    test_escapes();
    test_rejected_document_leaves_output_untouched();
    test_number_keeps_source_digits();
    test_stringify_round_trip();
    test_private_arenas_join();
    test_short_arena_leaves_output_untouched();
    return g_failed;
}

#ifndef ASCEND_FORGE_JSON_TESTS_NO_MAIN
int main(void)
{
    const int failed = JsonTests_Run();

    if (failed != 0) {
        fprintf(stderr, "%d json checks failed\n", failed);
        return 1;
    }
    printf("json checks passed\n");
    return 0;
}
#endif
