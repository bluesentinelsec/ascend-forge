/**
 * @file cli.c
 * @brief Argparse-style command-line parser. No printing and no plugin policy.
 */

#include "ascend_forge/cli.h"

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum {
    CLI_MAX_DEPTH = 16,
    CLI_MAX_OPTIONS = 256,
    CLI_MAX_CHOICES = 32,
    CLI_USAGE_WIDTH = 79
};

typedef enum CliStop {
    CLI_STOP_NONE = 0,
    CLI_STOP_HELP = 1,
    CLI_STOP_VERSION = 2,
    CLI_STOP_ERROR = 3,
    CLI_STOP_REJECT = 4
} CliStop;

typedef struct CliFrame {
    const CliCommand *command;
} CliFrame;

typedef struct CliFailure {
    CliError error;
    const CliCommand *command;
    const CliOption *option;
    const char *token;
    char detail[CLI_DETAIL_CAPACITY];
} CliFailure;

typedef struct CliScratch {
    CliValue *values;
    const char **positionals;
    const char **remainder;
    Uint32 positional_count;
    Uint32 remainder_count;
    Uint32 storage_limit;
    bool remainder_present;
    Uint32 remainder_id;
} CliScratch;

typedef struct CliText {
    char *buffer;
    Uint32 capacity;
    Uint32 length;
    Uint32 column;
} CliText;

static Uint32 cli_length(const char *text)
{
    size_t length = 0;

    if (text == NULL) {
        return 0u;
    }
    length = SDL_strlen(text);
    if (length > 0xffffffffu) {
        return 0xffffffffu;
    }
    return (Uint32)length;
}

static void *cli_alloc(Uint32 count, size_t element)
{
    void *memory = NULL;

    if (element == 0u || (size_t)count > (SIZE_MAX / element)) {
        SDL_Log("Cli_Parse expected an allocation that fits, received %" SDL_PRIu32, count);
        exit(EXIT_FAILURE);
    }
    memory = SDL_malloc((size_t)count * element);
    if (memory == NULL) {
        SDL_Log("Cli_Parse expected %" SDL_PRIu32 " records from SDL_malloc, received none", count);
        exit(EXIT_FAILURE);
    }
    return memory;
}

static bool cli_spelling_ok(const char *spelling, bool required)
{
    Uint32 length = 0;

    if (spelling == NULL) {
        return !required;
    }
    length = cli_length(spelling);
    if (length < 2u || spelling[0] != '-') {
        return false;
    }
    if (SDL_strcmp(spelling, "--") == 0) {
        return false;
    }
    if (SDL_strcmp(spelling, "-h") == 0 || SDL_strcmp(spelling, "--help") == 0) {
        return false;
    }
    return true;
}

static bool cli_choices_ok(const char *const *choices)
{
    Uint32 count = 0;

    if (choices == NULL) {
        return true;
    }
    while (count < (Uint32)CLI_MAX_CHOICES) {
        if (choices[count] == NULL) {
            return count > 0u;
        }
        if (choices[count][0] == '\0') {
            return false;
        }
        count += 1u;
    }
    return false;
}

static bool cli_option_ok(const CliOption *option, Uint8 *seen, Uint32 value_count, Uint32 *remainders)
{
    if (option->spelling == NULL || !cli_spelling_ok(option->spelling, true) ||
        !cli_spelling_ok(option->alias, false)) {
        SDL_Log("Cli_Parse expected an option spelling that starts with a dash");
        return false;
    }
    if (option->alias != NULL && SDL_strcmp(option->spelling, option->alias) == 0) {
        SDL_Log("Cli_Parse expected distinct spellings for %s", option->spelling);
        return false;
    }
    if (option->arity != CLI_ARITY_NONE && option->arity != CLI_ARITY_ONE &&
        option->arity != CLI_ARITY_REMAINDER) {
        SDL_Log("Cli_Parse expected an arity for %s", option->spelling);
        return false;
    }
    if (option->id >= value_count || seen[option->id] != 0u) {
        SDL_Log("Cli_Parse expected a unique option id below %" SDL_PRIu32 ", received %" SDL_PRIu32,
                value_count, option->id);
        return false;
    }
    if (!cli_choices_ok(option->choices)) {
        SDL_Log("Cli_Parse expected NULL-terminated choices for %s", option->spelling);
        return false;
    }
    if (option->choices != NULL && option->arity != CLI_ARITY_ONE) {
        SDL_Log("Cli_Parse expected choices only on a value for %s", option->spelling);
        return false;
    }
    if (option->default_text != NULL && option->arity != CLI_ARITY_ONE) {
        SDL_Log("Cli_Parse expected a default only on a value for %s", option->spelling);
        return false;
    }
    if (option->version && (option->arity != CLI_ARITY_NONE || option->required)) {
        SDL_Log("Cli_Parse expected version to be an optional flag, received %s", option->spelling);
        return false;
    }
    if (option->arity == CLI_ARITY_REMAINDER) {
        *remainders += 1u;
        if (*remainders > 1u) {
            SDL_Log("Cli_Parse expected one remainder on a command path, received %s",
                    option->spelling);
            return false;
        }
    }
    seen[option->id] = 1u;
    return true;
}

static bool cli_command_ok(const CliCommand *command, Uint8 *seen, Uint32 value_count,
                           Uint32 remainders, Uint32 depth)
{
    Uint32 option_index = 0;
    Uint32 sub_index = 0;
    Uint32 path_remainders = remainders;

    if (command == NULL || command->name == NULL || command->name[0] == '\0' || command->name[0] == '-') {
        SDL_Log("Cli_Parse expected a command name");
        return false;
    }
    if (depth >= (Uint32)CLI_MAX_DEPTH) {
        SDL_Log("Cli_Parse expected a command depth below %d, received %" SDL_PRIu32, CLI_MAX_DEPTH,
                depth);
        return false;
    }
    if ((command->option_count > 0u && command->options == NULL) ||
        (command->sub_count > 0u && command->subs == NULL)) {
        SDL_Log("Cli_Parse expected options and subcommands for %s", command->name);
        return false;
    }
    if (command->positionals != CLI_POSITIONALS_NONE && command->positionals != CLI_POSITIONALS_OPTIONAL &&
        command->positionals != CLI_POSITIONALS_REQUIRED && command->positionals != CLI_POSITIONALS_MANY) {
        SDL_Log("Cli_Parse expected a positional arity for %s", command->name);
        return false;
    }
    if (command->subcommand_required && command->positionals != CLI_POSITIONALS_NONE) {
        SDL_Log("Cli_Parse expected positionals or a required subcommand for %s", command->name);
        return false;
    }
    if (command->positionals != CLI_POSITIONALS_NONE &&
        (command->positional_name == NULL || command->positional_name[0] == '\0')) {
        SDL_Log("Cli_Parse expected a positional name for %s", command->name);
        return false;
    }
    for (option_index = 0; option_index < command->option_count; option_index += 1u) {
        if (!cli_option_ok(&command->options[option_index], seen, value_count, &path_remainders)) {
            return false;
        }
    }
    for (sub_index = 0; sub_index < command->sub_count; sub_index += 1u) {
        if (!cli_command_ok(&command->subs[sub_index], seen, value_count, path_remainders, depth + 1u)) {
            return false;
        }
    }
    return true;
}

static bool cli_table_ok(const CliCommand *root, Uint32 value_count)
{
    Uint8 seen[CLI_MAX_OPTIONS];

    SDL_memset(seen, 0, sizeof seen);
    if (value_count > (Uint32)CLI_MAX_OPTIONS) {
        SDL_Log("Cli_Parse expected at most %d option ids, received %" SDL_PRIu32, CLI_MAX_OPTIONS,
                value_count);
        return false;
    }
    return cli_command_ok(root, seen, value_count, 0u, 0u);
}

static const CliCommand *cli_current(const CliFrame *frames, Uint32 depth)
{
    return frames[depth - 1u].command;
}

static Uint32 cli_depth(const CliFrame *frames)
{
    Uint32 depth = 1u;

    while (depth < (Uint32)CLI_MAX_DEPTH && frames[depth].command != NULL) {
        depth += 1u;
    }
    return depth;
}

static bool cli_match_spelling(const char *spelling, const char *token, bool *has_explicit,
                               const char **explicit_text, const char **matched)
{
    Uint32 length = 0;

    if (spelling == NULL) {
        return false;
    }
    length = cli_length(spelling);
    if (SDL_strcmp(token, spelling) == 0) {
        *has_explicit = false;
        *explicit_text = NULL;
        *matched = spelling;
        return true;
    }
    if (length > 0u && SDL_strncmp(token, spelling, length) == 0 && token[length] == '=') {
        *has_explicit = true;
        *explicit_text = token + length + 1u;
        *matched = spelling;
        return true;
    }
    return false;
}

static const CliOption *cli_find_option(const CliFrame *frames, Uint32 depth, const char *token,
                                        bool *has_explicit, const char **explicit_text,
                                        const char **matched)
{
    Uint32 level = depth;

    while (level > 0u) {
        const CliCommand *command = frames[level - 1u].command;
        Uint32 option_index = 0;

        level -= 1u;
        for (option_index = 0; option_index < command->option_count; option_index += 1u) {
            const CliOption *option = &command->options[option_index];

            if (cli_match_spelling(option->spelling, token, has_explicit, explicit_text, matched) ||
                cli_match_spelling(option->alias, token, has_explicit, explicit_text, matched)) {
                return option;
            }
        }
    }
    return NULL;
}

static const CliOption *cli_find_short(const CliFrame *frames, Uint32 depth, char letter)
{
    char spelling[3];
    Uint32 level = depth;

    spelling[0] = '-';
    spelling[1] = letter;
    spelling[2] = '\0';
    while (level > 0u) {
        const CliCommand *command = frames[level - 1u].command;
        Uint32 option_index = 0;

        level -= 1u;
        for (option_index = 0; option_index < command->option_count; option_index += 1u) {
            const CliOption *option = &command->options[option_index];

            if ((option->spelling != NULL && SDL_strcmp(option->spelling, spelling) == 0) ||
                (option->alias != NULL && SDL_strcmp(option->alias, spelling) == 0)) {
                return option;
            }
        }
    }
    return NULL;
}

static const CliCommand *cli_find_sub(const CliCommand *command, const char *token)
{
    Uint32 sub_index = 0;

    for (sub_index = 0; sub_index < command->sub_count; sub_index += 1u) {
        if (SDL_strcmp(command->subs[sub_index].name, token) == 0) {
            return &command->subs[sub_index];
        }
    }
    return NULL;
}

static bool cli_choice_ok(const CliOption *option, const char *value)
{
    Uint32 choice_index = 0;

    if (option->choices == NULL) {
        return true;
    }
    while (choice_index < (Uint32)CLI_MAX_CHOICES && option->choices[choice_index] != NULL) {
        if (SDL_strcmp(option->choices[choice_index], value) == 0) {
            return true;
        }
        choice_index += 1u;
    }
    return false;
}

static void cli_join_choices(const char *const *choices, char *buffer, Uint32 capacity)
{
    Uint32 choice_index = 0;

    buffer[0] = '\0';
    while (choice_index < (Uint32)CLI_MAX_CHOICES && choices[choice_index] != NULL) {
        if (choice_index > 0u) {
            SDL_strlcat(buffer, ", ", capacity);
        }
        SDL_strlcat(buffer, choices[choice_index], capacity);
        choice_index += 1u;
    }
}

static void cli_fail(CliFailure *failure, CliError error, const CliCommand *command,
                     const CliOption *option, const char *token, const char *detail)
{
    failure->error = error;
    failure->command = command;
    failure->option = option;
    failure->token = token;
    SDL_strlcpy(failure->detail, detail, CLI_DETAIL_CAPACITY);
}

static void cli_fail_unrecognized(CliFailure *failure, const CliCommand *command, const char *token)
{
    char detail[CLI_DETAIL_CAPACITY];

    SDL_snprintf(detail, sizeof detail, "unrecognized arguments: %s", token);
    cli_fail(failure, CLI_ERROR_UNRECOGNIZED, command, NULL, token, detail);
}

static void cli_fail_missing_value(CliFailure *failure, const CliCommand *command,
                                   const CliOption *option, const char *spelling)
{
    char detail[CLI_DETAIL_CAPACITY];

    SDL_snprintf(detail, sizeof detail, "argument %s: expected one argument", spelling);
    cli_fail(failure, CLI_ERROR_MISSING_VALUE, command, option, spelling, detail);
}

static void cli_fail_explicit(CliFailure *failure, const CliCommand *command, const CliOption *option,
                              const char *spelling, const char *value)
{
    char detail[CLI_DETAIL_CAPACITY];

    SDL_snprintf(detail, sizeof detail, "argument %s: ignored explicit argument '%s'", spelling, value);
    cli_fail(failure, CLI_ERROR_EXPLICIT_VALUE, command, option, spelling, detail);
}

static void cli_fail_choice(CliFailure *failure, const CliCommand *command, const CliOption *option,
                            const char *spelling, const char *value)
{
    char choices[192];
    char detail[CLI_DETAIL_CAPACITY];

    cli_join_choices(option->choices, choices, (Uint32)sizeof choices);
    SDL_snprintf(detail, sizeof detail, "argument %s: invalid choice: '%s' (choose from %s)", spelling,
                 value, choices);
    cli_fail(failure, CLI_ERROR_BAD_CHOICE, command, option, value, detail);
}

static void cli_fail_command(CliFailure *failure, const CliCommand *command, const char *token)
{
    char names[192];
    char detail[CLI_DETAIL_CAPACITY];
    Uint32 sub_index = 0;

    names[0] = '\0';
    for (sub_index = 0; sub_index < command->sub_count; sub_index += 1u) {
        if (sub_index > 0u) {
            SDL_strlcat(names, ", ", sizeof names);
        }
        SDL_strlcat(names, command->subs[sub_index].name, sizeof names);
    }
    SDL_snprintf(detail, sizeof detail, "argument command: invalid choice: '%s' (choose from %s)", token,
                 names);
    cli_fail(failure, CLI_ERROR_BAD_COMMAND, command, NULL, token, detail);
}

static const char *cli_display_spelling(const CliOption *option)
{
    if (option->spelling != NULL && option->spelling[1] == '-') {
        return option->spelling;
    }
    if (option->alias != NULL && option->alias[1] == '-') {
        return option->alias;
    }
    return option->spelling;
}

static bool cli_push_text(const char **slot, Uint32 *count, Uint32 capacity, const char *text,
                          const char *label)
{
    if (*count >= capacity || slot == NULL) {
        SDL_Log("Cli_Parse expected room for a %s, received %" SDL_PRIu32, label, capacity);
        return false;
    }
    slot[*count] = text;
    *count += 1u;
    return true;
}

static CliStop cli_store_value(CliScratch *scratch, const CliOption *option, const char *matched,
                               const char *value, const CliCommand *command, CliFailure *failure)
{
    if (!cli_choice_ok(option, value)) {
        cli_fail_choice(failure, command, option, matched, value);
        return CLI_STOP_ERROR;
    }
    scratch->values[option->id].present = true;
    scratch->values[option->id].text = value;
    if (option->version) {
        return CLI_STOP_VERSION;
    }
    return CLI_STOP_NONE;
}

static CliStop cli_store_remainder(CliScratch *scratch, const CliOption *option, int argc, char **argv,
                                   int *cursor, bool has_explicit, const char *explicit_text)
{
    scratch->remainder_present = true;
    scratch->remainder_id = option->id;
    scratch->values[option->id].present = true;
    scratch->values[option->id].text = NULL;
    if (has_explicit &&
        !cli_push_text(scratch->remainder, &scratch->remainder_count, scratch->storage_limit, explicit_text,
                       "remainder")) {
        return CLI_STOP_REJECT;
    }
    *cursor += 1;
    while (*cursor < argc) {
        if (argv[*cursor] == NULL) {
            SDL_Log("Cli_Parse expected an argument, received NULL");
            return CLI_STOP_REJECT;
        }
        if (!cli_push_text(scratch->remainder, &scratch->remainder_count, scratch->storage_limit, argv[*cursor],
                           "remainder")) {
            return CLI_STOP_REJECT;
        }
        *cursor += 1;
    }
    return CLI_STOP_NONE;
}

static CliStop cli_take_matched(CliScratch *scratch, const CliFrame *frames, Uint32 depth,
                                const CliOption *option, const char *matched, bool has_explicit,
                                const char *explicit_text, int argc, char **argv, int *cursor,
                                CliFailure *failure)
{
    const CliCommand *command = cli_current(frames, depth);
    const char *value = NULL;

    if (option->arity == CLI_ARITY_NONE) {
        if (has_explicit) {
            cli_fail_explicit(failure, command, option, matched, explicit_text);
            return CLI_STOP_ERROR;
        }
        scratch->values[option->id].present = true;
        scratch->values[option->id].text = NULL;
        *cursor += 1;
        if (option->version) {
            return CLI_STOP_VERSION;
        }
        return CLI_STOP_NONE;
    }
    if (option->arity == CLI_ARITY_REMAINDER) {
        return cli_store_remainder(scratch, option, argc, argv, cursor, has_explicit, explicit_text);
    }
    if (has_explicit) {
        value = explicit_text;
        *cursor += 1;
    } else if (*cursor + 1 >= argc || argv[*cursor + 1] == NULL) {
        if (*cursor + 1 < argc && argv[*cursor + 1] == NULL) {
            SDL_Log("Cli_Parse expected an argument, received NULL");
            return CLI_STOP_REJECT;
        }
        cli_fail_missing_value(failure, command, option, matched);
        return CLI_STOP_ERROR;
    } else {
        value = argv[*cursor + 1];
        *cursor += 2;
    }
    return cli_store_value(scratch, option, matched, value, command, failure);
}

static bool cli_help_token(const char *token, bool *has_explicit, const char **explicit_text,
                           const char **matched)
{
    if (cli_match_spelling("-h", token, has_explicit, explicit_text, matched) ||
        cli_match_spelling("--help", token, has_explicit, explicit_text, matched)) {
        return true;
    }
    return false;
}

static CliStop cli_take_cluster(CliScratch *scratch, const CliFrame *frames, Uint32 depth,
                                const char *token, int argc, char **argv, int *cursor,
                                CliFailure *failure)
{
    const CliCommand *command = cli_current(frames, depth);
    Uint32 letter = 1u;
    bool saw_help = false;

    while (token[letter] != '\0') {
        const CliOption *option = NULL;
        const char *value = NULL;
        CliStop stop_value = CLI_STOP_NONE;

        if (token[letter] == 'h') {
            saw_help = true;
            letter += 1u;
            continue;
        }
        option = cli_find_short(frames, depth, token[letter]);
        if (option == NULL || option->arity == CLI_ARITY_REMAINDER) {
            cli_fail_unrecognized(failure, command, token);
            return CLI_STOP_ERROR;
        }
        if (option->arity == CLI_ARITY_NONE) {
            scratch->values[option->id].present = true;
            scratch->values[option->id].text = NULL;
            if (option->version) {
                *cursor += 1;
                return CLI_STOP_VERSION;
            }
            letter += 1u;
            continue;
        }
        if (token[letter + 1u] != '\0') {
            value = token + letter + 1u;
            *cursor += 1;
        } else if (*cursor + 1 >= argc || argv[*cursor + 1] == NULL) {
            if (*cursor + 1 < argc && argv[*cursor + 1] == NULL) {
                SDL_Log("Cli_Parse expected an argument, received NULL");
                return CLI_STOP_REJECT;
            }
            cli_fail_missing_value(failure, command, option, option->spelling);
            return CLI_STOP_ERROR;
        } else {
            value = argv[*cursor + 1];
            *cursor += 2;
        }
        stop_value = cli_store_value(scratch, option, option->spelling, value, command, failure);
        if (stop_value != CLI_STOP_NONE) {
            return stop_value;
        }
        if (saw_help) {
            return CLI_STOP_HELP;
        }
        return CLI_STOP_NONE;
    }
    *cursor += 1;
    if (saw_help) {
        return CLI_STOP_HELP;
    }
    return CLI_STOP_NONE;
}

static CliStop cli_take_option(CliScratch *scratch, const CliFrame *frames, Uint32 depth, int argc,
                               char **argv, int *cursor, CliFailure *failure)
{
    const char *token = argv[*cursor];
    const CliOption *option = NULL;
    bool has_explicit = false;
    const char *explicit_text = NULL;
    const char *matched = NULL;

    if (cli_help_token(token, &has_explicit, &explicit_text, &matched)) {
        if (has_explicit) {
            cli_fail_explicit(failure, cli_current(frames, depth), NULL, matched, explicit_text);
            return CLI_STOP_ERROR;
        }
        *cursor += 1;
        return CLI_STOP_HELP;
    }
    option = cli_find_option(frames, depth, token, &has_explicit, &explicit_text, &matched);
    if (option != NULL) {
        return cli_take_matched(scratch, frames, depth, option, matched, has_explicit, explicit_text, argc,
                                argv, cursor, failure);
    }
    if (token[1] == '-') {
        cli_fail_unrecognized(failure, cli_current(frames, depth), token);
        return CLI_STOP_ERROR;
    }
    return cli_take_cluster(scratch, frames, depth, token, argc, argv, cursor, failure);
}

static CliStop cli_take_positional(CliScratch *scratch, const CliFrame *frames, Uint32 depth,
                                   const char *token, CliFailure *failure)
{
    const CliCommand *command = cli_current(frames, depth);

    if (command->sub_count > 0u && scratch->positional_count == 0u && command->positionals == CLI_POSITIONALS_NONE) {
        cli_fail_command(failure, command, token);
        return CLI_STOP_ERROR;
    }
    if (command->positionals == CLI_POSITIONALS_NONE ||
        (command->positionals == CLI_POSITIONALS_OPTIONAL && scratch->positional_count >= 1u) ||
        (command->positionals == CLI_POSITIONALS_REQUIRED && scratch->positional_count >= 1u)) {
        if (command->sub_count > 0u && scratch->positional_count == 0u) {
            cli_fail_command(failure, command, token);
            return CLI_STOP_ERROR;
        }
        cli_fail_unrecognized(failure, command, token);
        return CLI_STOP_ERROR;
    }
    if (!cli_push_text(scratch->positionals, &scratch->positional_count, scratch->storage_limit, token,
                       "positional")) {
        return CLI_STOP_REJECT;
    }
    return CLI_STOP_NONE;
}

static void cli_missing_command(CliFailure *failure, const CliCommand *command)
{
    char detail[CLI_DETAIL_CAPACITY];
    Uint32 sub_index = 0;

    SDL_strlcpy(detail, "the following arguments are required: {", sizeof detail);
    for (sub_index = 0; sub_index < command->sub_count; sub_index += 1u) {
        if (sub_index > 0u) {
            SDL_strlcat(detail, ",", sizeof detail);
        }
        SDL_strlcat(detail, command->subs[sub_index].name, sizeof detail);
    }
    SDL_strlcat(detail, "}", sizeof detail);
    cli_fail(failure, CLI_ERROR_MISSING_COMMAND, command, NULL, NULL, detail);
}

static bool cli_missing_options(const CliFrame *frames, Uint32 depth, const CliScratch *scratch,
                                CliFailure *failure)
{
    char detail[CLI_DETAIL_CAPACITY];
    Uint32 level = 0;
    Uint32 missing = 0;

    SDL_strlcpy(detail, "the following arguments are required: ", sizeof detail);
    for (level = 0; level < depth; level += 1u) {
        const CliCommand *command = frames[level].command;
        Uint32 option_index = 0;

        for (option_index = 0; option_index < command->option_count; option_index += 1u) {
            const CliOption *option = &command->options[option_index];

            if (!option->required || scratch->values[option->id].present || option->default_text != NULL) {
                continue;
            }
            if (missing > 0u) {
                SDL_strlcat(detail, ", ", sizeof detail);
            }
            SDL_strlcat(detail, cli_display_spelling(option), sizeof detail);
            missing += 1u;
        }
    }
    if (missing == 0u) {
        return false;
    }
    cli_fail(failure, CLI_ERROR_MISSING_OPTION, cli_current(frames, depth), NULL, NULL, detail);
    return true;
}

static void cli_apply_defaults(const CliFrame *frames, Uint32 depth, CliScratch *scratch)
{
    Uint32 level = 0;

    for (level = 0; level < depth; level += 1u) {
        const CliCommand *command = frames[level].command;
        Uint32 option_index = 0;

        for (option_index = 0; option_index < command->option_count; option_index += 1u) {
            const CliOption *option = &command->options[option_index];

            if (option->arity == CLI_ARITY_ONE && !scratch->values[option->id].present &&
                option->default_text != NULL) {
                scratch->values[option->id].text = option->default_text;
            }
        }
    }
}

static void cli_commit(CliParse *out, const CliScratch *scratch, const CliCommand *command, CliStatus status)
{
    Uint32 index = 0;

    for (index = 0; index < out->value_count; index += 1u) {
        out->values[index] = scratch->values[index];
    }
    for (index = 0; index < scratch->positional_count; index += 1u) {
        out->positionals[index] = scratch->positionals[index];
    }
    for (index = 0; index < scratch->remainder_count; index += 1u) {
        out->remainder[index] = scratch->remainder[index];
    }
    out->status = status;
    out->error = CLI_ERROR_NONE;
    out->command = command;
    out->error_option = NULL;
    out->error_token = NULL;
    out->detail[0] = '\0';
    out->positional_count = scratch->positional_count;
    out->remainder_count = scratch->remainder_count;
    out->remainder_present = scratch->remainder_present;
    out->remainder_id = scratch->remainder_id;
}

static void cli_commit_error(CliParse *out, const CliFailure *failure)
{
    out->status = CLI_STATUS_ERROR;
    out->error = failure->error;
    out->command = failure->command;
    out->error_option = failure->option;
    out->error_token = failure->token;
    SDL_strlcpy(out->detail, failure->detail, CLI_DETAIL_CAPACITY);
}

static bool cli_storage_ok(const CliParse *out)
{
    if (out->value_count > 0u && out->values == NULL) {
        SDL_Log("Cli_Parse expected value storage, received NULL");
        return false;
    }
    if (out->positional_capacity > 0u && out->positionals == NULL) {
        SDL_Log("Cli_Parse expected positional storage, received NULL");
        return false;
    }
    if (out->remainder_capacity > 0u && out->remainder == NULL) {
        SDL_Log("Cli_Parse expected remainder storage, received NULL");
        return false;
    }
    return true;
}

static bool cli_fits(const CliScratch *scratch, const CliParse *out)
{
    if (scratch->positional_count > out->positional_capacity) {
        SDL_Log("Cli_Parse expected room for %" SDL_PRIu32 " positionals, received %" SDL_PRIu32,
                scratch->positional_count, out->positional_capacity);
        return false;
    }
    if (scratch->remainder_count > out->remainder_capacity) {
        SDL_Log("Cli_Parse expected room for %" SDL_PRIu32 " remainder arguments, received %" SDL_PRIu32,
                scratch->remainder_count, out->remainder_capacity);
        return false;
    }
    return true;
}

bool Cli_Parse(const CliCommand *root, int argc, char **argv, CliParse *out)
{
    CliFrame frames[CLI_MAX_DEPTH];
    CliFailure failure;
    CliScratch scratch;
    CliStop stop = CLI_STOP_NONE;
    void *block = NULL;
    size_t values_bytes = 0;
    size_t argv_bytes = 0;
    Uint32 argc_count = 0;
    int cursor = 1;
    bool options_done = false;

    SDL_memset(&failure, 0, sizeof failure);
    SDL_memset(&scratch, 0, sizeof scratch);
    SDL_memset(frames, 0, sizeof frames);
    if (root == NULL || argv == NULL || out == NULL || argc < 1) {
        SDL_Log("Cli_Parse expected a command table, argv, and a result");
        return false;
    }
    if (!cli_storage_ok(out) || !cli_table_ok(root, out->value_count)) {
        return false;
    }
    argc_count = (Uint32)argc;
    values_bytes = (size_t)out->value_count * sizeof(CliValue);
    argv_bytes = (size_t)argc_count * sizeof(const char *);
    if (argv_bytes > (SIZE_MAX / 2u) || values_bytes > SIZE_MAX - argv_bytes - argv_bytes) {
        SDL_Log("Cli_Parse expected an allocation that fits, received %" SDL_PRIu32, argc_count);
        exit(EXIT_FAILURE);
    }
    block = cli_alloc(1u, values_bytes + argv_bytes + argv_bytes);
    {
        unsigned char *bytes = block;
        void *positional_memory = bytes + values_bytes;
        void *remainder_memory = bytes + values_bytes + argv_bytes;

        scratch.values = block;
        scratch.positionals = positional_memory;
        scratch.remainder = remainder_memory;
    }
    SDL_memset(scratch.values, 0, values_bytes);
    scratch.storage_limit = argc_count;
    frames[0].command = root;

    while (cursor < argc && stop == CLI_STOP_NONE) {
        const char *token = argv[cursor];
        const Uint32 depth = cli_depth(frames);
        const CliCommand *current = cli_current(frames, depth);
        if (token == NULL) {
            SDL_Log("Cli_Parse expected an argument, received NULL");
            stop = CLI_STOP_REJECT;
            break;
        }
        if (!options_done && SDL_strcmp(token, "--") == 0) {
            options_done = true;
            cursor += 1;
            continue;
        }
        if (!options_done && token[0] == '-' && token[1] != '\0') {
            stop = cli_take_option(&scratch, frames, depth, argc, argv, &cursor, &failure);
            continue;
        }
        if (!options_done && scratch.positional_count == 0u) {
            const CliCommand *sub = cli_find_sub(current, token);

            if (sub != NULL) {
                if (depth >= (Uint32)CLI_MAX_DEPTH) {
                    SDL_Log("Cli_Parse expected a command depth below %d", CLI_MAX_DEPTH);
                    stop = CLI_STOP_REJECT;
                    break;
                }
                frames[depth].command = sub;
                cursor += 1;
                continue;
            }
        }
        stop = cli_take_positional(&scratch, frames, depth, token, &failure);
        if (stop == CLI_STOP_NONE) {
            cursor += 1;
        }
    }

    if (stop == CLI_STOP_REJECT) {
        SDL_free(block);
        return false;
    }
    if (stop == CLI_STOP_NONE) {
        const Uint32 depth = cli_depth(frames);
        const CliCommand *current = cli_current(frames, depth);
        if (current->subcommand_required && current->sub_count > 0u) {
            cli_missing_command(&failure, current);
            stop = CLI_STOP_ERROR;
        } else if (cli_missing_options(frames, depth, &scratch, &failure)) {
            stop = CLI_STOP_ERROR;
        } else if (current->positionals == CLI_POSITIONALS_REQUIRED && scratch.positional_count == 0u) {
            char detail[CLI_DETAIL_CAPACITY];

            SDL_snprintf(detail, sizeof detail, "the following arguments are required: %s",
                         current->positional_name);
            cli_fail(&failure, CLI_ERROR_MISSING_POSITIONAL, current, NULL, NULL, detail);
            stop = CLI_STOP_ERROR;
        } else {
            cli_apply_defaults(frames, depth, &scratch);
            if (!cli_fits(&scratch, out)) {
                SDL_free(block);
                return false;
            }
            cli_commit(out, &scratch, current, CLI_STATUS_OK);
            SDL_free(block);
            return true;
        }
    }
    if (stop == CLI_STOP_HELP || stop == CLI_STOP_VERSION) {
        const Uint32 depth = cli_depth(frames);
        const CliStatus status = stop == CLI_STOP_HELP ? CLI_STATUS_HELP : CLI_STATUS_VERSION;

        if (!cli_fits(&scratch, out)) {
            SDL_free(block);
            return false;
        }
        cli_commit(out, &scratch, cli_current(frames, depth), status);
        SDL_free(block);
        return true;
    }
    SDL_free(block);
    cli_commit_error(out, &failure);
    return true;
}

static void cli_text_add(CliText *text, const char *piece)
{
    Uint32 index = 0;

    if (piece == NULL) {
        return;
    }
    while (piece[index] != '\0') {
        if (text->buffer != NULL && text->length + 1u < text->capacity) {
            text->buffer[text->length] = piece[index];
        }
        if (text->length < 0xffffffffu) {
            text->length += 1u;
        }
        index += 1u;
    }
}

static void cli_usage_word(CliText *text, const char *word)
{
    Uint32 length = cli_length(word);

    if (text->column > 7u && text->column + 1u + length > (Uint32)CLI_USAGE_WIDTH) {
        cli_text_add(text, "\n       ");
        text->column = 7u;
    } else if (text->column > 7u) {
        cli_text_add(text, " ");
        text->column += 1u;
    }
    cli_text_add(text, word);
    text->column += length;
}

static void cli_upper_name(const char *spelling, char *buffer, Uint32 capacity)
{
    Uint32 in_index = 0;
    Uint32 out_index = 0;

    while (spelling[in_index] == '-') {
        in_index += 1u;
    }
    while (spelling[in_index] != '\0' && out_index + 1u < capacity) {
        char letter = spelling[in_index];

        if (letter >= 'a' && letter <= 'z') {
            letter = (char)(letter - 'a' + 'A');
        }
        buffer[out_index] = letter;
        out_index += 1u;
        in_index += 1u;
    }
    buffer[out_index] = '\0';
}

static void cli_value_label(const CliOption *option, char *buffer, Uint32 capacity)
{
    if (option->arity == CLI_ARITY_NONE) {
        buffer[0] = '\0';
        return;
    }
    if (option->arity == CLI_ARITY_REMAINDER) {
        const char *metavar = option->metavar != NULL ? option->metavar : "ARG";

        SDL_snprintf(buffer, capacity, "[%s ...]", metavar);
        return;
    }
    if (option->choices != NULL) {
        Uint32 choice_index = 0;

        SDL_strlcpy(buffer, "{", capacity);
        while (choice_index < (Uint32)CLI_MAX_CHOICES && option->choices[choice_index] != NULL) {
            if (choice_index > 0u) {
                SDL_strlcat(buffer, ",", capacity);
            }
            SDL_strlcat(buffer, option->choices[choice_index], capacity);
            choice_index += 1u;
        }
        SDL_strlcat(buffer, "}", capacity);
        return;
    }
    if (option->metavar != NULL) {
        SDL_strlcpy(buffer, option->metavar, capacity);
        return;
    }
    cli_upper_name(option->spelling, buffer, capacity);
}

static void cli_option_usage(const CliOption *option, CliText *text)
{
    char label[160];
    char word[192];

    cli_value_label(option, label, (Uint32)sizeof label);
    if (label[0] == '\0') {
        if (option->required) {
            SDL_snprintf(word, sizeof word, "%s", option->spelling);
        } else {
            SDL_snprintf(word, sizeof word, "[%s]", option->spelling);
        }
    } else if (option->required) {
        SDL_snprintf(word, sizeof word, "%s %s", option->spelling, label);
    } else {
        SDL_snprintf(word, sizeof word, "[%s %s]", option->spelling, label);
    }
    cli_usage_word(text, word);
}

static void cli_subcommand_usage(const CliCommand *command, CliText *text)
{
    char group[256];
    char word[288];
    Uint32 sub_index = 0;

    if (command->sub_count == 0u) {
        return;
    }
    SDL_strlcpy(group, "{", sizeof group);
    for (sub_index = 0; sub_index < command->sub_count; sub_index += 1u) {
        if (sub_index > 0u) {
            SDL_strlcat(group, ",", sizeof group);
        }
        SDL_strlcat(group, command->subs[sub_index].name, sizeof group);
    }
    SDL_strlcat(group, "}", sizeof group);
    if (command->subcommand_required) {
        SDL_snprintf(word, sizeof word, "%s ...", group);
    } else {
        SDL_snprintf(word, sizeof word, "[%s ...]", group);
    }
    cli_usage_word(text, word);
}

static void cli_positional_usage(const CliCommand *command, CliText *text)
{
    char word[192];

    if (command->positionals == CLI_POSITIONALS_NONE) {
        return;
    }
    if (command->positionals == CLI_POSITIONALS_REQUIRED) {
        SDL_snprintf(word, sizeof word, "%s", command->positional_name);
    } else if (command->positionals == CLI_POSITIONALS_OPTIONAL) {
        SDL_snprintf(word, sizeof word, "[%s]", command->positional_name);
    } else {
        SDL_snprintf(word, sizeof word, "[%s ...]", command->positional_name);
    }
    cli_usage_word(text, word);
}

static bool cli_path(const CliCommand *command, const CliCommand *target, CliFrame *frames, Uint32 *depth)
{
    Uint32 sub_index = 0;

    if (*depth >= (Uint32)CLI_MAX_DEPTH) {
        return false;
    }
    frames[*depth].command = command;
    *depth += 1u;
    if (command == target) {
        return true;
    }
    for (sub_index = 0; sub_index < command->sub_count; sub_index += 1u) {
        if (cli_path(&command->subs[sub_index], target, frames, depth)) {
            return true;
        }
    }
    *depth -= 1u;
    frames[*depth].command = NULL;
    return false;
}

static void cli_write_usage(const CliFrame *frames, Uint32 depth, CliText *text)
{
    const CliCommand *leaf = cli_current(frames, depth);
    Uint32 level = 0;

    cli_text_add(text, "usage: ");
    text->column = 7u;
    for (level = 0; level < depth; level += 1u) {
        cli_usage_word(text, frames[level].command->name);
    }
    cli_usage_word(text, "[-h]");
    for (level = 0; level < depth; level += 1u) {
        const CliCommand *command = frames[level].command;
        Uint32 option_index = 0;

        for (option_index = 0; option_index < command->option_count; option_index += 1u) {
            cli_option_usage(&command->options[option_index], text);
        }
    }
    cli_positional_usage(leaf, text);
    cli_subcommand_usage(leaf, text);
    cli_text_add(text, "\n");
}

static void cli_help_row(CliText *text, const char *left, const char *help, Uint32 indent)
{
    Uint32 length = cli_length(left);
    Uint32 pad = 0;
    Uint32 spaces = 0;

    while (spaces < indent) {
        cli_text_add(text, " ");
        spaces += 1u;
    }
    cli_text_add(text, left);
    if (help == NULL || help[0] == '\0') {
        cli_text_add(text, "\n");
        return;
    }
    if (indent + length >= 24u) {
        cli_text_add(text, "\n                        ");
    } else {
        pad = 24u - (indent + length);
        while (pad > 0u) {
            cli_text_add(text, " ");
            pad -= 1u;
        }
    }
    cli_text_add(text, help);
    cli_text_add(text, "\n");
}

static void cli_option_left(const CliOption *option, char *buffer, Uint32 capacity)
{
    const char *extra = NULL;
    char label[160];
    const char *first = option->spelling;
    const char *second = option->alias;

    if (option->alias != NULL && cli_length(option->alias) == 2u && option->alias[0] == '-' &&
        option->alias[1] != '-') {
        first = option->alias;
        second = option->spelling;
    }
    if (option->spelling != NULL && cli_length(option->spelling) == 2u && option->spelling[1] != '-' &&
        option->alias != NULL) {
        first = option->spelling;
        second = option->alias;
    }
    cli_value_label(option, label, (Uint32)sizeof label);
    if (second != NULL) {
        extra = second;
    }
    if (extra == NULL) {
        if (label[0] == '\0') {
            SDL_snprintf(buffer, capacity, "%s", first);
        } else {
            SDL_snprintf(buffer, capacity, "%s %s", first, label);
        }
        return;
    }
    if (label[0] == '\0') {
        SDL_snprintf(buffer, capacity, "%s, %s", first, extra);
        return;
    }
    SDL_snprintf(buffer, capacity, "%s, %s %s", first, extra, label);
}

static void cli_write_help_body(const CliFrame *frames, Uint32 depth, CliText *text)
{
    const CliCommand *leaf = cli_current(frames, depth);
    Uint32 level = 0;
    char left[192];

    cli_write_usage(frames, depth, text);
    if (leaf->description != NULL && leaf->description[0] != '\0') {
        cli_text_add(text, "\n");
        cli_text_add(text, leaf->description);
        cli_text_add(text, "\n");
    }
    if (leaf->positionals != CLI_POSITIONALS_NONE || leaf->sub_count > 0u) {
        cli_text_add(text, "\npositional arguments:\n");
        if (leaf->positionals != CLI_POSITIONALS_NONE) {
            cli_help_row(text, leaf->positional_name, leaf->positional_help, 2u);
        }
        if (leaf->sub_count > 0u) {
            char group[256];
            Uint32 sub_index = 0;

            SDL_strlcpy(group, "{", sizeof group);
            for (sub_index = 0; sub_index < leaf->sub_count; sub_index += 1u) {
                if (sub_index > 0u) {
                    SDL_strlcat(group, ",", sizeof group);
                }
                SDL_strlcat(group, leaf->subs[sub_index].name, sizeof group);
            }
            SDL_strlcat(group, "}", sizeof group);
            cli_help_row(text, group, "command", 2u);
            for (sub_index = 0; sub_index < leaf->sub_count; sub_index += 1u) {
                cli_help_row(text, leaf->subs[sub_index].name, leaf->subs[sub_index].help, 4u);
            }
        }
    }
    cli_text_add(text, "\noptions:\n");
    cli_help_row(text, "-h, --help", "show this help message and exit", 2u);
    for (level = 0; level < depth; level += 1u) {
        const CliCommand *command = frames[level].command;
        Uint32 option_index = 0;

        for (option_index = 0; option_index < command->option_count; option_index += 1u) {
            cli_option_left(&command->options[option_index], left, (Uint32)sizeof left);
            cli_help_row(text, left, command->options[option_index].help, 2u);
        }
    }
}

static bool cli_text_finish(const CliText *text, char *buffer, Uint32 capacity, Uint32 *written)
{
    if (written != NULL) {
        *written = text->length;
    }
    if (buffer != NULL && capacity > 0u) {
        Uint32 end = text->length;

        if (end >= capacity) {
            end = capacity - 1u;
        }
        buffer[end] = '\0';
    }
    if (buffer == NULL) {
        return true;
    }
    return text->length < capacity;
}

static bool cli_prepare_text(const CliCommand *root, const CliCommand *command, char *buffer,
                             Uint32 capacity, CliFrame *frames, Uint32 *depth, const char *name)
{
    if (root == NULL || command == NULL) {
        SDL_Log("%s expected a root and a command", name);
        return false;
    }
    if ((buffer == NULL && capacity != 0u) || (buffer != NULL && capacity == 0u)) {
        SDL_Log("%s expected a buffer whose capacity matches it", name);
        return false;
    }
    *depth = 0u;
    SDL_memset(frames, 0, sizeof(CliFrame) * (size_t)CLI_MAX_DEPTH);
    if (!cli_path(root, command, frames, depth) || *depth == 0u) {
        SDL_Log("%s expected the command to belong to the table", name);
        return false;
    }
    return true;
}

bool Cli_FormatHelp(const CliCommand *root, const CliCommand *command, char *buffer, Uint32 capacity,
                    Uint32 *written)
{
    CliFrame frames[CLI_MAX_DEPTH];
    CliText text;
    Uint32 depth = 0;

    SDL_memset(&text, 0, sizeof text);
    if (!cli_prepare_text(root, command, buffer, capacity, frames, &depth, "Cli_FormatHelp")) {
        return false;
    }
    text.buffer = buffer;
    text.capacity = capacity;
    cli_write_help_body(frames, depth, &text);
    return cli_text_finish(&text, buffer, capacity, written);
}

bool Cli_FormatError(const CliCommand *root, const CliParse *parse, char *buffer, Uint32 capacity,
                     Uint32 *written)
{
    CliFrame frames[CLI_MAX_DEPTH];
    CliText text;
    Uint32 depth = 0;

    SDL_memset(&text, 0, sizeof text);
    if (parse == NULL || parse->status != CLI_STATUS_ERROR || parse->command == NULL) {
        SDL_Log("Cli_FormatError expected an error parse");
        return false;
    }
    if (!cli_prepare_text(root, parse->command, buffer, capacity, frames, &depth, "Cli_FormatError")) {
        return false;
    }
    text.buffer = buffer;
    text.capacity = capacity;
    cli_write_usage(frames, depth, &text);
    cli_text_add(&text, root->name);
    cli_text_add(&text, ": error: ");
    cli_text_add(&text, parse->detail);
    cli_text_add(&text, "\n");
    return cli_text_finish(&text, buffer, capacity, written);
}
