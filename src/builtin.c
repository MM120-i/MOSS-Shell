// Built in Shell functions
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include "include/builtin.h"

const struct builtin builtins[] = {
    {"cd", lsh_cd},
    {"help", lsh_help},
    {"exit", lsh_exit},
    {"pwd", lsh_pwd},
    {"clear", lsh_clear},
};

const int NUM_BUILTINS = sizeof(builtins) / sizeof(struct builtin);

int lsh_cd(char **args)
{
    if (!args[1])
    {
        char *home = getenv("HOME");

        if (!home)
            fprintf(stderr, "lsh: cd: home not set.\n");
        else if (chdir(home) != 0)
            perror("lsh");

        return 1;
    }

    if (chdir(args[1]) != 0)
        perror("lsh");

    if (args[2])
        fprintf(stderr, "lsh: cd: too many arguments.\n");

    return 1;
}

int lsh_help(char **args)
{
    (void)args;
    printf("Type programs names and arguments, and hit enter.\n");
    printf("The following are built in:\n");

    for (size_t i = 0; i < NUM_BUILTINS; i++)
        printf("  %s\n", builtins[i].name);

    printf("Use the 'man' command for more information on other programs.\n");

    return 1;
}

int lsh_exit(char **args)
{
    (void)args;
    return 0;
}

int lsh_pwd(char **args)
{
    (void)args;
    char cwd[SIZE];

    if (getcwd(cwd, sizeof(cwd)) != NULL)
        printf("%s\n", cwd);
    else
        perror("lsh");

    return 1;
}

int lsh_clear(char **args)
{
    (void)args;
    printf("\033[H\033[J");
    return 1;
}