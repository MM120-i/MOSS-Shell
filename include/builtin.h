#pragma once

#include <stdbool.h>

#define SIZE 4096
#define ERROR_RATE_LIMIT 5
#define ERROR_RATE_WINDOW 60

struct builtin
{
    char *name;
    int (*func)(char **);
};

typedef struct
{
    const char *path;
} ChdirContext;

extern const struct builtin builtins[];
extern int builtins_count;
extern const int NUM_BUILTINS;

// built in commands
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