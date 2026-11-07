/**
 * @file cli_test.c
 * @brief Self-hosted checks for the command-line parser.
 */

#include "ascend_forge/cli.h"

#include "cli_tests.h"

#include <stdio.h>

enum { FIX_SLOTS = 8 };

static int g_failed = 0;

static const CliOption root_options[] = {
    {.id = 0, .spelling = "--json", .arity = CLI_ARITY_NONE, .help = "print one JSON document on stdout"},
    {.id = 1,
     .spelling = "--config",
     .arity = CLI_ARITY_ONE,
     .metavar = "FILE",
     .help = "read ambient config from FILE"},
    {.id = 2,
     .spelling = "-V",
     .alias = "--version",
     .arity = CLI_ARITY_NONE,
     .version = true,
     .help = "print the version and exit"},
};

static const CliCommand version_command = {
    .name = "version",
    .help = "print the host version",
};

static const CliCommand tool_root = {
    .name = "tool",
    .help = "tool",
    .description = "A sample tool",
    .options = root_options,
    .option_count = 3,
    .subs = &version_command,
    .sub_count = 1,
    .subcommand_required = true,
};

static const char *const kind_choices[] = {"gui", "cli", "server", NULL};

static const CliOption kind_options[] = {
    {.id = 0,
     .spelling = "--kind",
     .arity = CLI_ARITY_ONE,
     .choices = kind_choices,
     .required = true,
     .help = "plugin shape"},
};

static const CliCommand kind_root = {
    .name = "tool",
    .help = "tool",
    .options = kind_options,
    .option_count = 1,
};

static const CliOption pack_options[] = {
    {.id = 0,
     .spelling = "--pack",
     .arity = CLI_ARITY_ONE,
     .metavar = "binary|archive",
     .default_text = "binary",
     .help = "package"},
};

static const CliCommand pack_root = {
    .name = "tool",
    .help = "tool",
    .options = pack_options,
    .option_count = 1,
};

static const CliOption name_options[] = {
    {.id = 0, .spelling = "--plugin", .arity = CLI_ARITY_ONE, .required = true, .metavar = "PLUGIN"},
    {.id = 1, .spelling = "--name", .arity = CLI_ARITY_ONE, .required = true, .metavar = "NAME"},
};

static const CliCommand name_root = {
    .name = "tool",
    .help = "tool",
    .options = name_options,
    .option_count = 2,
};

static const CliOption cluster_options[] = {
    {.id = 0, .spelling = "-a", .arity = CLI_ARITY_NONE, .help = "a"},
    {.id = 1, .spelling = "-b", .arity = CLI_ARITY_NONE, .help = "b"},
    {.id = 2, .spelling = "-o", .arity = CLI_ARITY_ONE, .metavar = "FILE", .help = "output"},
};

static const CliCommand cluster_root = {
    .name = "tool",
    .help = "tool",
    .options = cluster_options,
    .option_count = 3,
};

static const CliOption run_options[] = {
    {.id = 4,
     .spelling = "--plugin",
     .arity = CLI_ARITY_ONE,
     .required = true,
     .metavar = "PLUGIN",
     .help = "plugin to run"},
    {.id = 5, .spelling = "--entry", .arity = CLI_ARITY_ONE, .metavar = "ENTRY", .help = "entry point"},
    {.id = 6, .spelling = "--file", .arity = CLI_ARITY_ONE, .metavar = "FILE", .help = "request file"},
    {.id = 7,
     .spelling = "--args",
     .arity = CLI_ARITY_REMAINDER,
     .metavar = "ARG",
     .help = "arguments for the plugin"},
};

static const CliCommand run_command = {
    .name = "run",
    .help = "run a plugin",
    .options = run_options,
    .option_count = 4,
};

static const CliOption run_root_options[] = {
    {.id = 0, .spelling = "--json", .arity = CLI_ARITY_NONE, .help = "print one JSON document on stdout"},
};

static const CliCommand run_root = {
    .name = "tool",
    .help = "tool",
    .options = run_root_options,
    .option_count = 1,
    .subs = &run_command,
    .sub_count = 1,
    .subcommand_required = true,
};

static const CliOption new_options[] = {
    {.id = 4, .spelling = "--plugin", .arity = CLI_ARITY_ONE, .required = true, .metavar = "PLUGIN"},
    {.id = 5, .spelling = "--name", .arity = CLI_ARITY_ONE, .required = true, .metavar = "NAME"},
};

static const CliCommand new_command = {
    .name = "new",
    .help = "add a test",
    .options = new_options,
    .option_count = 2,
};

static const CliOption test_options[] = {
    {.id = 1, .spelling = "-run", .arity = CLI_ARITY_ONE, .metavar = "name", .help = "select tests"},
    {.id = 2, .spelling = "-list", .arity = CLI_ARITY_ONE, .metavar = "name", .help = "list tests"},
    {.id = 3, .spelling = "-v", .arity = CLI_ARITY_NONE, .help = "verbose"},
};

static const CliCommand test_command = {
    .name = "test",
    .help = "run tests",
    .options = test_options,
    .option_count = 3,
    .subs = &new_command,
    .sub_count = 1,
    .positionals = CLI_POSITIONALS_MANY,
    .positional_name = "plugin",
    .positional_help = "plugin path",
};

static const CliOption test_root_options[] = {
    {.id = 0, .spelling = "--json", .arity = CLI_ARITY_NONE, .help = "print one JSON document on stdout"},
};

static const CliCommand test_root = {
    .name = "tool",
    .help = "tool",
    .options = test_root_options,
    .option_count = 1,
    .subs = &test_command,
    .sub_count = 1,
    .subcommand_required = true,
};

static const CliOption duplicate_options[] = {
    {.id = 0, .spelling = "--one", .arity = CLI_ARITY_NONE},
    {.id = 0, .spelling = "--two", .arity = CLI_ARITY_NONE},
};

static const CliCommand duplicate_root = {
    .name = "tool",
    .help = "tool",
    .options = duplicate_options,
    .option_count = 2,
};

static const CliCommand many_root = {
    .name = "tool",
    .help = "tool",
    .positionals = CLI_POSITIONALS_MANY,
    .positional_name = "plugin",
    .positional_help = "plugin path",
};

typedef struct Fixture {
    CliValue values[FIX_SLOTS];
    const char *positionals[FIX_SLOTS];
    const char *remainder[FIX_SLOTS];
    CliParse parse;
} Fixture;

static void expect_true(bool condition, const char *name)
{
    if (condition) {
        return;
    }
    fprintf(stderr, "FAIL: %s\n", name);
    g_failed += 1;
}

static bool same_text(const char *left, const char *right)
{
    if (left == NULL || right == NULL) {
        return left == right;
    }
    return SDL_strcmp(left, right) == 0;
}

static bool contains_text(const char *haystack, const char *needle)
{
    if (haystack == NULL || needle == NULL) {
        return false;
    }
    return SDL_strstr(haystack, needle) != NULL;
}

static void fixture_init(Fixture *fixture)
{
    SDL_memset(fixture, 0, sizeof *fixture);
    fixture->parse.values = fixture->values;
    fixture->parse.value_count = (Uint32)FIX_SLOTS;
    fixture->parse.positionals = fixture->positionals;
    fixture->parse.positional_capacity = (Uint32)FIX_SLOTS;
    fixture->parse.remainder = fixture->remainder;
    fixture->parse.remainder_capacity = (Uint32)FIX_SLOTS;
}

static bool parse_at(Fixture *fixture, const CliCommand *root, int argc, char **argv)
{
    fixture_init(fixture);
    return Cli_Parse(root, argc, argv, &fixture->parse);
}

static void test_null_root_leaves_the_parse_untouched(void)
{
    Fixture fixture;
    char *argv[] = {"tool", NULL};

    fixture_init(&fixture);
    fixture.parse.positional_count = 42u;
    fixture.values[0].present = true;
    fixture.values[0].text = "sentinel";
    fixture.parse.detail[0] = 'Z';
    fixture.parse.detail[1] = '\0';
    expect_true(!Cli_Parse(NULL, 1, argv, &fixture.parse), "Cli_NullRoot");
    expect_true(fixture.parse.positional_count == 42u, "Cli_NullRoot_Count");
    expect_true(fixture.values[0].present && same_text(fixture.values[0].text, "sentinel"),
                "Cli_NullRoot_Value");
    expect_true(fixture.parse.detail[0] == 'Z', "Cli_NullRoot_Detail");
}

static void test_duplicate_option_id_leaves_the_parse_untouched(void)
{
    Fixture fixture;
    char *argv[] = {"tool", NULL};

    fixture_init(&fixture);
    fixture.parse.positional_count = 42u;
    fixture.values[0].text = "sentinel";
    expect_true(!Cli_Parse(&duplicate_root, 1, argv, &fixture.parse), "Cli_DuplicateId");
    expect_true(fixture.parse.positional_count == 42u, "Cli_DuplicateId_Count");
    expect_true(same_text(fixture.values[0].text, "sentinel"), "Cli_DuplicateId_Value");
}

static void test_short_storage_leaves_the_parse_untouched(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "plugin", NULL};

    fixture_init(&fixture);
    fixture.parse.positional_capacity = 0u;
    fixture.parse.positionals = NULL;
    fixture.values[0].text = "sentinel";
    expect_true(!Cli_Parse(&many_root, 2, argv, &fixture.parse), "Cli_ShortStorage");
    expect_true(same_text(fixture.values[0].text, "sentinel"), "Cli_ShortStorage_Value");
    expect_true(fixture.parse.positional_count == 0u, "Cli_ShortStorage_Count");
}

static void test_help_flag_selects_the_root_command(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "-h", NULL};

    expect_true(parse_at(&fixture, &tool_root, 2, argv), "Cli_Help_Parse");
    expect_true(fixture.parse.status == CLI_STATUS_HELP, "Cli_Help_Status");
    expect_true(fixture.parse.command == &tool_root, "Cli_Help_Command");
}

static void test_help_on_a_subcommand_selects_that_command(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "version", "--help", NULL};

    expect_true(parse_at(&fixture, &tool_root, 3, argv), "Cli_SubHelp_Parse");
    expect_true(fixture.parse.status == CLI_STATUS_HELP, "Cli_SubHelp_Status");
    expect_true(fixture.parse.command == &version_command, "Cli_SubHelp_Command");
}

static void test_flag_before_help_is_recorded(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "--json", "-h", NULL};

    expect_true(parse_at(&fixture, &tool_root, 3, argv), "Cli_HelpFlag_Parse");
    expect_true(fixture.parse.status == CLI_STATUS_HELP, "Cli_HelpFlag_Status");
    expect_true(fixture.values[0].present, "Cli_HelpFlag_Json");
}

static void test_required_option_does_not_block_help(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "-h", NULL};

    expect_true(parse_at(&fixture, &name_root, 2, argv), "Cli_HelpRequired_Parse");
    expect_true(fixture.parse.status == CLI_STATUS_HELP, "Cli_HelpRequired_Status");
}

static void test_version_flag_does_not_require_a_command(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "--version", "nope", NULL};

    expect_true(parse_at(&fixture, &tool_root, 3, argv), "Cli_VersionFlag_Parse");
    expect_true(fixture.parse.status == CLI_STATUS_VERSION, "Cli_VersionFlag_Status");
    expect_true(fixture.values[2].present, "Cli_VersionFlag_Present");
}

static void test_short_version_flag_requests_version(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "-V", NULL};

    expect_true(parse_at(&fixture, &tool_root, 2, argv), "Cli_ShortVersion_Parse");
    expect_true(fixture.parse.status == CLI_STATUS_VERSION, "Cli_ShortVersion_Status");
}

static void test_version_command_is_selected(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "version", NULL};

    expect_true(parse_at(&fixture, &tool_root, 2, argv), "Cli_Version_Parse");
    expect_true(fixture.parse.status == CLI_STATUS_OK, "Cli_Version_Status");
    expect_true(fixture.parse.command == &version_command, "Cli_Version_Command");
}

static void test_missing_command_names_the_choices(void)
{
    Fixture fixture;
    char buffer[512];
    char *argv[] = {"tool", NULL};
    Uint32 written = 0u;

    expect_true(parse_at(&fixture, &tool_root, 1, argv), "Cli_MissingCommand_Parse");
    expect_true(fixture.parse.status == CLI_STATUS_ERROR, "Cli_MissingCommand_Status");
    expect_true(fixture.parse.error == CLI_ERROR_MISSING_COMMAND, "Cli_MissingCommand_Error");
    expect_true(Cli_FormatError(&tool_root, &fixture.parse, buffer, (Uint32)sizeof buffer, &written),
                "Cli_MissingCommand_Format");
    expect_true(contains_text(buffer, "usage: tool"), "Cli_MissingCommand_Usage");
    expect_true(contains_text(buffer, "tool: error: "), "Cli_MissingCommand_Label");
    expect_true(contains_text(buffer, "{version}"), "Cli_MissingCommand_Choices");
}

static void test_unknown_command_names_the_choices(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "nope", NULL};

    expect_true(parse_at(&fixture, &tool_root, 2, argv), "Cli_UnknownCommand_Parse");
    expect_true(fixture.parse.error == CLI_ERROR_BAD_COMMAND, "Cli_UnknownCommand_Error");
    expect_true(same_text(fixture.parse.error_token, "nope"), "Cli_UnknownCommand_Token");
    expect_true(contains_text(fixture.parse.detail, "choose from version"), "Cli_UnknownCommand_Choices");
}

static void test_unknown_option_is_unrecognized(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "--nope", NULL};

    expect_true(parse_at(&fixture, &tool_root, 2, argv), "Cli_UnknownOption_Parse");
    expect_true(fixture.parse.error == CLI_ERROR_UNRECOGNIZED, "Cli_UnknownOption_Error");
    expect_true(contains_text(fixture.parse.detail, "unrecognized arguments: --nope"),
                "Cli_UnknownOption_Detail");
}

static void test_user_error_leaves_values_untouched(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "--nope", NULL};

    fixture_init(&fixture);
    fixture.values[0].present = true;
    fixture.values[0].text = "sentinel";
    fixture.parse.positional_count = 42u;
    expect_true(Cli_Parse(&tool_root, 2, argv, &fixture.parse), "Cli_UserError_Parse");
    expect_true(fixture.parse.status == CLI_STATUS_ERROR, "Cli_UserError_Status");
    expect_true(fixture.values[0].present && same_text(fixture.values[0].text, "sentinel"),
                "Cli_UserError_Value");
    expect_true(fixture.parse.positional_count == 42u, "Cli_UserError_Count");
}

static void test_missing_option_value_names_the_option(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "--config", NULL};

    expect_true(parse_at(&fixture, &tool_root, 2, argv), "Cli_MissingValue_Parse");
    expect_true(fixture.parse.error == CLI_ERROR_MISSING_VALUE, "Cli_MissingValue_Error");
    expect_true(contains_text(fixture.parse.detail, "argument --config: expected one argument"),
                "Cli_MissingValue_Detail");
}

static void test_equals_supplies_the_option_value(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "--config=./forge.json", "version", NULL};

    expect_true(parse_at(&fixture, &tool_root, 3, argv), "Cli_Equals_Parse");
    expect_true(fixture.parse.status == CLI_STATUS_OK, "Cli_Equals_Status");
    expect_true(fixture.values[1].present && same_text(fixture.values[1].text, "./forge.json"),
                "Cli_Equals_Value");
}

static void test_separate_token_supplies_the_option_value(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "--config", "./forge.json", "version", NULL};

    expect_true(parse_at(&fixture, &tool_root, 4, argv), "Cli_Separate_Parse");
    expect_true(fixture.values[1].present && same_text(fixture.values[1].text, "./forge.json"),
                "Cli_Separate_Value");
}

static void test_option_value_may_look_like_a_flag(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "--config", "--json", "version", NULL};

    expect_true(parse_at(&fixture, &tool_root, 4, argv), "Cli_FlagValue_Parse");
    expect_true(fixture.parse.status == CLI_STATUS_OK, "Cli_FlagValue_Status");
    expect_true(same_text(fixture.values[1].text, "--json"), "Cli_FlagValue_Config");
    expect_true(!fixture.values[0].present, "Cli_FlagValue_Json");
}

static void test_boolean_rejects_an_explicit_value(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "--json=true", "version", NULL};

    expect_true(parse_at(&fixture, &tool_root, 3, argv), "Cli_Explicit_Parse");
    expect_true(fixture.parse.error == CLI_ERROR_EXPLICIT_VALUE, "Cli_Explicit_Error");
    expect_true(contains_text(fixture.parse.detail, "ignored explicit argument 'true'"),
                "Cli_Explicit_Detail");
}

static void test_global_option_before_the_command_is_recorded(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "--json", "version", NULL};

    expect_true(parse_at(&fixture, &tool_root, 3, argv), "Cli_GlobalBefore_Parse");
    expect_true(fixture.parse.command == &version_command, "Cli_GlobalBefore_Command");
    expect_true(fixture.values[0].present, "Cli_GlobalBefore_Json");
}

static void test_global_option_after_the_command_is_recorded(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "version", "--json", NULL};

    expect_true(parse_at(&fixture, &tool_root, 3, argv), "Cli_GlobalAfter_Parse");
    expect_true(fixture.parse.command == &version_command, "Cli_GlobalAfter_Command");
    expect_true(fixture.values[0].present, "Cli_GlobalAfter_Json");
}

static void test_last_repeated_option_wins(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "--config", "a", "--config", "b", "version", NULL};

    expect_true(parse_at(&fixture, &tool_root, 6, argv), "Cli_LastWins_Parse");
    expect_true(fixture.values[1].present && same_text(fixture.values[1].text, "b"), "Cli_LastWins_Value");
}

static void test_abbreviation_is_not_an_option(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "--conf", "file", "version", NULL};

    expect_true(parse_at(&fixture, &tool_root, 4, argv), "Cli_Abbrev_Parse");
    expect_true(fixture.parse.error == CLI_ERROR_UNRECOGNIZED, "Cli_Abbrev_Error");
    expect_true(same_text(fixture.parse.error_token, "--conf"), "Cli_Abbrev_Token");
}

static void test_choice_rejects_an_unknown_value(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "--kind", "nope", NULL};

    expect_true(parse_at(&fixture, &kind_root, 3, argv), "Cli_BadChoice_Parse");
    expect_true(fixture.parse.error == CLI_ERROR_BAD_CHOICE, "Cli_BadChoice_Error");
    expect_true(contains_text(fixture.parse.detail, "invalid choice: 'nope' (choose from gui, cli, server)"),
                "Cli_BadChoice_Detail");
}

static void test_choice_accepts_a_declared_value(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "--kind", "cli", NULL};

    expect_true(parse_at(&fixture, &kind_root, 3, argv), "Cli_Choice_Parse");
    expect_true(fixture.parse.status == CLI_STATUS_OK, "Cli_Choice_Status");
    expect_true(fixture.values[0].present && same_text(fixture.values[0].text, "cli"), "Cli_Choice_Value");
}

static void test_missing_required_options_are_listed(void)
{
    Fixture fixture;
    char *argv[] = {"tool", NULL};

    expect_true(parse_at(&fixture, &name_root, 1, argv), "Cli_Required_Parse");
    expect_true(fixture.parse.error == CLI_ERROR_MISSING_OPTION, "Cli_Required_Error");
    expect_true(contains_text(fixture.parse.detail, "--plugin"), "Cli_Required_Plugin");
    expect_true(contains_text(fixture.parse.detail, "--name"), "Cli_Required_Name");
}

static void test_default_fills_an_absent_option(void)
{
    Fixture fixture;
    char *argv[] = {"tool", NULL};

    expect_true(parse_at(&fixture, &pack_root, 1, argv), "Cli_Default_Parse");
    expect_true(fixture.parse.status == CLI_STATUS_OK, "Cli_Default_Status");
    expect_true(!fixture.values[0].present && same_text(fixture.values[0].text, "binary"), "Cli_Default_Value");
}

static void test_user_value_replaces_the_default(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "--pack", "archive", NULL};

    expect_true(parse_at(&fixture, &pack_root, 3, argv), "Cli_Override_Parse");
    expect_true(fixture.values[0].present && same_text(fixture.values[0].text, "archive"),
                "Cli_Override_Value");
}

static void test_single_dash_name_takes_the_next_token(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "test", "-run", "RejectsEmpty", NULL};

    expect_true(parse_at(&fixture, &test_root, 4, argv), "Cli_RunFlag_Parse");
    expect_true(fixture.parse.status == CLI_STATUS_OK, "Cli_RunFlag_Status");
    expect_true(fixture.parse.command == &test_command, "Cli_RunFlag_Command");
    expect_true(fixture.values[1].present && same_text(fixture.values[1].text, "RejectsEmpty"),
                "Cli_RunFlag_Value");
}

static void test_single_dash_name_takes_an_equals_value(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "test", "-run=RejectsEmpty", NULL};

    expect_true(parse_at(&fixture, &test_root, 3, argv), "Cli_RunEquals_Parse");
    expect_true(fixture.values[1].present && same_text(fixture.values[1].text, "RejectsEmpty"),
                "Cli_RunEquals_Value");
}

static void test_positionals_and_options_may_intermix(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "test", "./plugins/png-viewer", "-run", "RejectsEmpty", "-v", NULL};

    expect_true(parse_at(&fixture, &test_root, 6, argv), "Cli_Intermix_Parse");
    expect_true(fixture.parse.positional_count == 1u, "Cli_Intermix_Count");
    expect_true(same_text(fixture.positionals[0], "./plugins/png-viewer"), "Cli_Intermix_Plugin");
    expect_true(same_text(fixture.values[1].text, "RejectsEmpty"), "Cli_Intermix_Run");
    expect_true(fixture.values[3].present, "Cli_Intermix_Verbose");
}

static void test_subcommand_wins_over_a_positional_name(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "test", "new", "--plugin", "p", "--name", "n", NULL};

    expect_true(parse_at(&fixture, &test_root, 7, argv), "Cli_SubWins_Parse");
    expect_true(fixture.parse.command == &new_command, "Cli_SubWins_Command");
    expect_true(fixture.parse.positional_count == 0u, "Cli_SubWins_Positionals");
    expect_true(same_text(fixture.values[4].text, "p") && same_text(fixture.values[5].text, "n"),
                "Cli_SubWins_Values");
}

static void test_double_dash_keeps_a_positional_that_looks_like_a_flag(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "test", "--", "--json", NULL};

    expect_true(parse_at(&fixture, &test_root, 4, argv), "Cli_EndOptions_Parse");
    expect_true(fixture.parse.status == CLI_STATUS_OK, "Cli_EndOptions_Status");
    expect_true(fixture.parse.positional_count == 1u && same_text(fixture.positionals[0], "--json"),
                "Cli_EndOptions_Positional");
    expect_true(!fixture.values[0].present, "Cli_EndOptions_Json");
}

static void test_clustered_short_flags_are_recorded(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "-ab", NULL};

    expect_true(parse_at(&fixture, &cluster_root, 2, argv), "Cli_Cluster_Parse");
    expect_true(fixture.parse.status == CLI_STATUS_OK, "Cli_Cluster_Status");
    expect_true(fixture.values[0].present && fixture.values[1].present, "Cli_Cluster_Flags");
}

static void test_attached_short_value_is_recorded(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "-oout.txt", NULL};

    expect_true(parse_at(&fixture, &cluster_root, 2, argv), "Cli_Attached_Parse");
    expect_true(fixture.values[2].present && same_text(fixture.values[2].text, "out.txt"),
                "Cli_Attached_Value");
}

static void test_separate_short_value_is_recorded(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "-o", "out.txt", NULL};

    expect_true(parse_at(&fixture, &cluster_root, 3, argv), "Cli_ShortValue_Parse");
    expect_true(fixture.values[2].present && same_text(fixture.values[2].text, "out.txt"),
                "Cli_ShortValue_Value");
}

static void test_remainder_keeps_the_following_tokens(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "run", "--plugin", "p", "--args", "sample.png", "--out", "thumb.png", "-h", NULL};

    expect_true(parse_at(&fixture, &run_root, 9, argv), "Cli_Remainder_Parse");
    expect_true(fixture.parse.status == CLI_STATUS_OK, "Cli_Remainder_Status");
    expect_true(fixture.parse.command == &run_command, "Cli_Remainder_Command");
    expect_true(fixture.parse.remainder_present && fixture.parse.remainder_count == 4u, "Cli_Remainder_Count");
    expect_true(same_text(fixture.remainder[0], "sample.png") && same_text(fixture.remainder[1], "--out") &&
                    same_text(fixture.remainder[2], "thumb.png") && same_text(fixture.remainder[3], "-h"),
                "Cli_Remainder_Tokens");
    expect_true(!fixture.values[0].present, "Cli_Remainder_Json");
}

static void test_remainder_equals_starts_the_tail(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "run", "--plugin", "p", "--args=sample.png", "--out", "thumb.png", NULL};

    expect_true(parse_at(&fixture, &run_root, 7, argv), "Cli_RemainderEquals_Parse");
    expect_true(fixture.parse.remainder_count == 3u && same_text(fixture.remainder[0], "sample.png"),
                "Cli_RemainderEquals_First");
    expect_true(same_text(fixture.remainder[1], "--out") && same_text(fixture.remainder[2], "thumb.png"),
                "Cli_RemainderEquals_Rest");
}

static void test_empty_remainder_is_present(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "run", "--plugin", "p", "--args", NULL};

    expect_true(parse_at(&fixture, &run_root, 5, argv), "Cli_EmptyRemainder_Parse");
    expect_true(fixture.parse.status == CLI_STATUS_OK, "Cli_EmptyRemainder_Status");
    expect_true(fixture.parse.remainder_present && fixture.parse.remainder_count == 0u, "Cli_EmptyRemainder_Count");
}

static void test_double_dash_does_not_become_the_remainder(void)
{
    Fixture fixture;
    char *argv[] = {"tool", "run", "--plugin", "p", "--", "sample.png", NULL};

    expect_true(parse_at(&fixture, &run_root, 6, argv), "Cli_BareDash_Parse");
    expect_true(fixture.parse.error == CLI_ERROR_UNRECOGNIZED, "Cli_BareDash_Error");
    expect_true(same_text(fixture.parse.error_token, "sample.png"), "Cli_BareDash_Token");
    expect_true(!fixture.parse.remainder_present, "Cli_BareDash_Remainder");
}

static void test_help_text_lists_the_commands(void)
{
    char buffer[2048];
    Uint32 written = 0u;

    expect_true(Cli_FormatHelp(&tool_root, &tool_root, buffer, (Uint32)sizeof buffer, &written), "Cli_HelpText");
    expect_true(contains_text(buffer, "usage: tool "), "Cli_HelpText_Usage");
    expect_true(contains_text(buffer, "-h, --help"), "Cli_HelpText_Help");
    expect_true(contains_text(buffer, "show this help message and exit"), "Cli_HelpText_Sentence");
    expect_true(contains_text(buffer, "version"), "Cli_HelpText_Command");
    expect_true(contains_text(buffer, "A sample tool"), "Cli_HelpText_Description");
}

static void test_help_text_lists_the_remainder(void)
{
    char buffer[2048];
    Uint32 written = 0u;

    expect_true(Cli_FormatHelp(&run_root, &run_command, buffer, (Uint32)sizeof buffer, &written),
                "Cli_RemainderHelp");
    expect_true(contains_text(buffer, "--args"), "Cli_RemainderHelp_Flag");
    expect_true(contains_text(buffer, "arguments for the plugin"), "Cli_RemainderHelp_Sentence");
}

static void test_help_rejects_a_command_outside_the_table(void)
{
    char buffer[64];
    Uint32 written = 11u;

    buffer[0] = '\0';
    expect_true(!Cli_FormatHelp(&run_root, &version_command, buffer, (Uint32)sizeof buffer, &written),
                "Cli_ForeignHelp");
    expect_true(written == 11u, "Cli_ForeignHelp_Written");
    expect_true(buffer[0] == '\0', "Cli_ForeignHelp_Buffer");
}

int CliTests_Run(void)
{
    g_failed = 0;
    test_null_root_leaves_the_parse_untouched();
    test_duplicate_option_id_leaves_the_parse_untouched();
    test_short_storage_leaves_the_parse_untouched();
    test_help_flag_selects_the_root_command();
    test_help_on_a_subcommand_selects_that_command();
    test_flag_before_help_is_recorded();
    test_required_option_does_not_block_help();
    test_version_flag_does_not_require_a_command();
    test_short_version_flag_requests_version();
    test_version_command_is_selected();
    test_missing_command_names_the_choices();
    test_unknown_command_names_the_choices();
    test_unknown_option_is_unrecognized();
    test_user_error_leaves_values_untouched();
    test_missing_option_value_names_the_option();
    test_equals_supplies_the_option_value();
    test_separate_token_supplies_the_option_value();
    test_option_value_may_look_like_a_flag();
    test_boolean_rejects_an_explicit_value();
    test_global_option_before_the_command_is_recorded();
    test_global_option_after_the_command_is_recorded();
    test_last_repeated_option_wins();
    test_abbreviation_is_not_an_option();
    test_choice_rejects_an_unknown_value();
    test_choice_accepts_a_declared_value();
    test_missing_required_options_are_listed();
    test_default_fills_an_absent_option();
    test_user_value_replaces_the_default();
    test_single_dash_name_takes_the_next_token();
    test_single_dash_name_takes_an_equals_value();
    test_positionals_and_options_may_intermix();
    test_subcommand_wins_over_a_positional_name();
    test_double_dash_keeps_a_positional_that_looks_like_a_flag();
    test_clustered_short_flags_are_recorded();
    test_attached_short_value_is_recorded();
    test_separate_short_value_is_recorded();
    test_remainder_keeps_the_following_tokens();
    test_remainder_equals_starts_the_tail();
    test_empty_remainder_is_present();
    test_double_dash_does_not_become_the_remainder();
    test_help_text_lists_the_commands();
    test_help_text_lists_the_remainder();
    test_help_rejects_a_command_outside_the_table();
    return g_failed;
}

#ifndef ASCEND_FORGE_CLI_TESTS_NO_MAIN
int main(void)
{
    const int failed = CliTests_Run();

    if (failed != 0) {
        fprintf(stderr, "%d cli checks failed\n", failed);
        return 1;
    }
    printf("cli checks passed\n");
    return 0;
}
#endif
