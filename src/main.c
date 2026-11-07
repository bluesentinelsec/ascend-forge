/**
 * @file main.c
 * @brief Program entrypoint.
 *
 * Keep this file thin. Library code lives under src/<component>/.
 */

#include "ascend_forge/cli.h"
#include "ascend_forge/version.h"

#include <stdio.h>

#define HOST_TEXT_CAPACITY 4096u

static const CliOption host_options[] = {
    {.id = 0,
     .spelling = "--json",
     .arity = CLI_ARITY_NONE,
     .help = "print one JSON document on stdout"},
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

static const CliCommand host_version = {
    .name = "version",
    .help = "print the host version",
};

static const CliCommand host_root = {
    .name = "ascend-forge",
    .help = "Ascend Forge host",
    .description = "Ascend Forge host",
    .options = host_options,
    .option_count = 3,
    .subs = &host_version,
    .sub_count = 1,
    .subcommand_required = true,
};

static bool write_message(bool formatted, const char *text, FILE *stream)
{
    if (!formatted || text == NULL) {
        fprintf(stderr, "ascend-forge: error: could not format the message\n");
        return false;
    }
    fputs(text, stream);
    return true;
}

int main(int argc, char **argv)
{
    CliValue values[3];
    const char *positionals[1] = {NULL};
    const char *remainder[1] = {NULL};
    CliParse parse;
    char text[HOST_TEXT_CAPACITY];
    Uint32 written = 0u;

    SDL_memset(&parse, 0, sizeof parse);
    SDL_memset(values, 0, sizeof values);
    parse.values = values;
    parse.value_count = 3u;
    parse.positionals = positionals;
    parse.positional_capacity = 1u;
    parse.remainder = remainder;
    parse.remainder_capacity = 1u;
    text[0] = '\0';
    if (!Cli_Parse(&host_root, argc, argv, &parse)) {
        return 1;
    }
    if (parse.status == CLI_STATUS_HELP) {
        if (!write_message(Cli_FormatHelp(&host_root, parse.command, text, HOST_TEXT_CAPACITY, &written),
                           text, stdout)) {
            return 1;
        }
        return 0;
    }
    if (parse.status == CLI_STATUS_VERSION ||
        (parse.status == CLI_STATUS_OK && parse.command == &host_version)) {
        printf("%s\n", Version_String());
        return 0;
    }
    if (parse.status == CLI_STATUS_ERROR) {
        if (!write_message(Cli_FormatError(&host_root, &parse, text, HOST_TEXT_CAPACITY, &written), text,
                           stderr)) {
            return 1;
        }
        return 1;
    }
    fprintf(stderr, "ascend-forge: error: unrecognized command\n");
    return 1;
}
