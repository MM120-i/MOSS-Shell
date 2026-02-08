#pragma once

#define SIZE 4096

struct builtin
{
    char *name;
    int (*func)(char **);
};

extern const struct builtin builtins[];
extern int builtins_count;
extern const int NUM_BUILTINS;

// built in functions
int moss_cd(char **);
int moss_help(char **);
int moss_exit(char **);
int moss_pwd(char **);
int moss_clear(char **);