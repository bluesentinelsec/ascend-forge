/**
 * @file cli.h
 * @brief Reusable command-line parser.
 *
 * The caller owns the command table and the output storage. Parse borrows
 * argv and does not print. Help and errors follow argparse: exact names,
 * `--name value` and `--name=value`, choices, defaults, and subcommands.
 * `-h` and `--help` are built in.
 *
 * A remainder option stops option processing and keeps every following
 * argument. The host spells that option `--args`, the way gdb --args keeps
 * the program's arguments. The primitive does not know about plugins.
 *
 * A temporary parse record is one SDL allocation, freed before return. When
 * that allocation fails, the call logs and exits. A user error is a completed
 * parse. The value arrays stay as the caller left them. A null or broken
 * table is rejected and the output is left untouched.
 */

#ifndef ASCEND_FORGE_CLI_H
#define ASCEND_FORGE_CLI_H

#include <SDL3/SDL.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Room for one error sentence, including the terminator. */
#define CLI_DETAIL_CAPACITY 384u

/** How many arguments an option takes. */
typedef enum CliArity {
    CLI_ARITY_NONE = 0,
    CLI_ARITY_ONE = 1,
    CLI_ARITY_REMAINDER = 2
} CliArity;

/** How many positional arguments a command accepts. */
typedef enum CliPositionals {
    CLI_POSITIONALS_NONE = 0,
    CLI_POSITIONALS_OPTIONAL = 1,
    CLI_POSITIONALS_REQUIRED = 2,
    CLI_POSITIONALS_MANY = 3
} CliPositionals;

/** What the parse is asking the caller to do. */
typedef enum CliStatus {
    CLI_STATUS_OK = 0,
    CLI_STATUS_HELP = 1,
    CLI_STATUS_VERSION = 2,
    CLI_STATUS_ERROR = 3
} CliStatus;

/** Why a completed parse is a bad invocation. */
typedef enum CliError {
    CLI_ERROR_NONE = 0,
    CLI_ERROR_UNRECOGNIZED = 1,
    CLI_ERROR_MISSING_VALUE = 2,
    CLI_ERROR_EXPLICIT_VALUE = 3,
    CLI_ERROR_BAD_CHOICE = 4,
    CLI_ERROR_BAD_COMMAND = 5,
    CLI_ERROR_MISSING_COMMAND = 6,
    CLI_ERROR_MISSING_OPTION = 7,
    CLI_ERROR_MISSING_POSITIONAL = 8
} CliError;

/**
 * One option. id is the index in the caller's value array. spelling is the
 * primary form, including its dashes, and alias is the other form or NULL.
 * choices is NULL or a NULL-terminated list. version stops the parse the way
 * argparse's version action does.
 */
typedef struct CliOption {
    Uint32 id;
    const char *spelling;
    const char *alias;
    CliArity arity;
    const char *const *choices;
    const char *metavar;
    const char *help;
    bool required;
    const char *default_text;
    bool version;
} CliOption;

/**
 * One command. name is a single word. options and subs are NULL when their
 * counts are zero. A required subcommand does not also take positionals.
 * positional_name is the metavar shown in help.
 */
typedef struct CliCommand {
    const char *name;
    const char *help;
    const char *description;
    const CliOption *options;
    Uint32 option_count;
    const struct CliCommand *subs;
    Uint32 sub_count;
    bool subcommand_required;
    CliPositionals positionals;
    const char *positional_name;
    const char *positional_help;
} CliCommand;

/** One stored option. text borrows argv or a default from the table. */
typedef struct CliValue {
    bool present;
    const char *text;
} CliValue;

/**
 * Caller-owned parse result. Zero it, then set the storage pointers.
 * On success, help, or version, values and the argument arrays are written.
 * On a user error, those arrays are left alone and detail holds the sentence.
 */
typedef struct CliParse {
    CliStatus status;
    CliError error;
    const CliCommand *command;
    const CliOption *error_option;
    const char *error_token;
    char detail[CLI_DETAIL_CAPACITY];
    CliValue *values;
    Uint32 value_count;
    const char **positionals;
    Uint32 positional_count;
    Uint32 positional_capacity;
    const char **remainder;
    Uint32 remainder_count;
    Uint32 remainder_capacity;
    bool remainder_present;
    Uint32 remainder_id;
} CliParse;

/**
 * @brief Parse argv against a command table.
 *
 * argv[0] is the program name and is not an argument. A remainder option
 * keeps every later argument, including ones that look like options. `--`
 * ends option processing for positionals and is not itself an argument.
 *
 * @param root Command table. NULL is rejected.
 * @param argc Argument count from main. Less than 1 is rejected.
 * @param argv Argument vector from main. NULL is rejected.
 * @param out Receives the parse. NULL is rejected. On rejection this is
 *        left untouched.
 * @return false when the table, argv, or the caller's storage is unusable.
 */
bool Cli_Parse(const CliCommand *root, int argc, char **argv, CliParse *out);

/**
 * @brief Write argparse-style help for command.
 *
 * command must be root or a command under it. The text ends in a newline.
 * A buffer that cannot hold the text is still terminated when capacity is
 * at least 1, written receives the full length without the terminator, and
 * the call returns false. A NULL buffer with a zero capacity only measures.
 *
 * @param written Receives the length without the terminator. NULL is allowed.
 * @return false when an argument is unusable or the buffer is short.
 */
bool Cli_FormatHelp(const CliCommand *root, const CliCommand *command, char *buffer,
                    Uint32 capacity, Uint32 *written);

/**
 * @brief Write the usage line and the error sentence.
 *
 * The sentence is `name: error: ...` on the line after usage. out must be a
 * completed parse whose status is CLI_STATUS_ERROR.
 *
 * @param written Receives the length without the terminator. NULL is allowed.
 * @return false when an argument is unusable, the parse is not an error, or
 *         the buffer is short.
 */
bool Cli_FormatError(const CliCommand *root, const CliParse *parse, char *buffer,
                     Uint32 capacity, Uint32 *written);

#ifdef __cplusplus
}
#endif

#endif /* ASCEND_FORGE_CLI_H */
