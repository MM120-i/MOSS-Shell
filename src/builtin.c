#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#include "include/builtin.h"

const struct builtin builtins[] = {
    {"cd", moss_cd},
    {"help", moss_help},
    {"exit", moss_exit},
    {"pwd", moss_pwd},
    {"clear", moss_clear},
};

const int NUM_BUILTINS = sizeof(builtins) / sizeof(struct builtin);

int moss_cd(char **args)
{
    if (!args[1])
    {
        char *home = getenv("HOME");

        if (!home)
            fprintf(stderr, "MOSS: cd: home not set.\n");
        else if (chdir(home) != 0)
            perror("MOSS");

        return 1;
    }

    if (chdir(args[1]) != 0)
        perror("MOSS");

    if (args[2])
        fprintf(stderr, "MOSS: cd: too many arguments.\n");

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
        perror("MOSS: malloc");
        return 1;
    }

    if (getcwd(cwd, bufsize) != NULL)
        printf("%s\n", cwd);
    else
    {
        if (errno == ERANGE)
            fprintf(stderr, "MOSS: Current directory path is too long.\n");
        else
            perror("MOSS");
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