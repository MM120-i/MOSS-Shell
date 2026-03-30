#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>

#include "builtin.h"
#include "logging.h"

extern const struct builtin builtins[];
extern const size_t NUM_BUILTINS;

int moss_help(char **args)
{
    (void)args;
    printf("Type programs names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (size_t i = 0; i < NUM_BUILTINS; i++)
        printf("  %s\n", builtins[i].name);

    return 1;
}

int moss_exit(char **args)
{
    (void)args;
    return 0;
}

int moss_pwd(char **args)
{
    (void)args;

#ifdef PATH_MAX
    size_t bufsize = PATH_MAX;
#else
    size_t bufsize = 4096;
#endif

    char *cwd = (char *)malloc(bufsize);

    if (!cwd)
    {
        SAFE_ERROR(ERR_CATEGORY_MEMORY, "memory allocation failed");
        return 1;
    }

    if (getcwd(cwd, bufsize) != NULL)
        printf("%s\n", cwd);
    else
    {
        if (errno == ERANGE)
            SAFE_ERROR(ERR_CATEGORY_PATH, "path too long");
        else
            SAFE_ERROR(ERR_CATEGORY_PATH, "failed to get current directory");
    }

    free(cwd);

    return 1;
}

int moss_clear(char **args)
{
    (void)args;
    printf("\033[H\033[J");
    return 1;
}

int moss_echo(char **args)
{
    for (size_t i = 1; args[i] != NULL; i++)
    {
        printf("%s", args[i]);

        if (args[i + 1] != NULL)
            printf(" ");
    }

    printf("\n");

    return 1;
}

int moss_whoami(char **args)
{
    (void)args;
    const char *userName = getenv("USER");

    if (!userName)
        userName = getenv("USERNAME");

    if (userName)
        printf("%s\n", userName);
    else
        SAFE_ERROR(ERR_CATEGORY_SYSTEM, "Could not find the user");

    return 1;
}

int moss_type(char **args)
{
    if (!args[1])
    {
        SAFE_ERROR(ERR_CATEGORY_INPUT, "Usage: type COMMAND");
        return 1;
    }

    for (size_t i = 0; i < NUM_BUILTINS; i++)
    {
        if (strcmp(args[1], builtins[i].name) == 0)
        {
            printf("%s is a shell builtin\n", args[1]);
            return 1;
        }
    }

    printf("%s is an external command\n", args[1]);

    return 1;
}
