#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "builtin.h"
#include "logging.h"

extern char **environ;

int moss_env(char **args)
{
    (void)args;

    for (char **env = environ; *env != NULL; env++)
        printf("%s\n", *env);

    return 1;
}

int moss_export(char **args)
{
    if (!args[1])
    {
        moss_env(args);
        return 1;
    }

    char *eq = strchr(args[1], '=');

    if (!eq)
    {
        SAFE_ERROR(ERR_CATEGORY_INPUT, "Usage: export NAME=VALUE");
        return 1;
    }

    *eq = '\0';
    const char *name = args[1];
    const char *value = eq + 1;

    if (setenv(name, value, 1) != 0)
        SAFE_ERROR(ERR_CATEGORY_SYSTEM, "Failed to set env variable");

    return 1;
}

int moss_unset(char **args)
{
    if (!args[1])
    {
        SAFE_ERROR(ERR_CATEGORY_INPUT, "Usage: unset NAME");
        return 1;
    }

    if (unsetenv(args[1]) != 0)
        SAFE_ERROR(ERR_CATEGORY_SYSTEM, "Failed to unset variable");

    return 1;
}
