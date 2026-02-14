#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>

#include "include/builtin.h"

private int error_count = 0;
private time_t error_window_start = 0;

const struct builtin builtins[] = {
    {"cd", moss_cd},
    {"help", moss_help},
    {"exit", moss_exit},
    {"pwd", moss_pwd},
    {"clear", moss_clear},
};

const int NUM_BUILTINS = sizeof(builtins) / sizeof(struct builtin);

private bool checkRateLimit()
{
    time_t now = time(NULL);

    if (difftime(now, error_window_start) > ERROR_RATE_WINDOW)
    {
        error_count = 0;
        error_window_start = now;
    }

    return error_count < ERROR_RATE_LIMIT;
}

private void safeError(const char *msg)
{
    if (!checkRateLimit())
        return;

    error_count++;
    fprintf(stderr, "MOSS: %s\n", msg);
}

int moss_cd(char **args)
{
    if (!args[1])
    {
        char *home = getenv("HOME");

        if (!home)
            safeError("home directory not set");
        else if (chdir(home) != 0)
            safeError("failed to change to home directory");

        return 1;
    }

    char resolved[PATH_MAX];

    if (realpath(args[1], resolved) == NULL)
    {
        safeError("invalid path");
        return 1;
    }

    if (chdir(resolved) != 0)
        safeError("failed to change directory");

    if (args[2])
        safeError("too many arguments");

    return 1;
}

int moss_help(char **args)
{
    (void)args;
    printf("Type programs names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (size_t i = 0; i < NUM_BUILTINS; i++)
        printf("  %s\n", builtins[i].name);

    printf("Use the 'man' command for more information on other programs.\n");

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
    size_t bufsize = SIZE;
#endif

    char *cwd = (char *)malloc(bufsize);

    if (!cwd)
    {
        safeError("memory allocation failed");
        return 1;
    }

    if (getcwd(cwd, bufsize) != NULL)
        printf("%s\n", cwd);
    else
    {
        if (errno == ERANGE)
            safeError("path too long");
        else
            safeError("failed to get current directory");
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