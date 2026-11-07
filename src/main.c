/**
 * @file main.c
 * @brief Program entrypoint.
 *
 * Keep this file thin. Library code lives under src/<component>/.
 */

#include "ascend_forge/version.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    int index = 1;

    for (; index < argc; index += 1) {
        const char *argument = argv[index];

        if (argument != NULL &&
            (strcmp(argument, "-V") == 0 || strcmp(argument, "--version") == 0)) {
            printf("%s\n", Version_String());
            return 0;
        }
    }
    printf("ascend-forge %s\n", Version_String());
    return 0;
}
