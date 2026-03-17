#pragma once

#include <stdbool.h>

#define private static

typedef struct
{
    const char *path;
} ChdirContext;

struct builtin
{
    char *name;
    int (*func)(char **);
};

extern const struct builtin builtins[];
extern const int NUM_BUILTINS;

int moss_cd(char **);
int moss_help(char **);
int moss_exit(char **);
int moss_pwd(char **);
int moss_clear(char **);
int moss_echo(char **);
int moss_whoami(char **);
int moss_env(char **);
int moss_export(char **);
int moss_unset(char **);
int moss_type(char **);
int moss_ls(char **);
