#pragma once

#define SIZE 1024

struct builtin
{
    char *name;
    int (*func)(char **);
};

extern const struct builtin builtins[];
extern int builtins_count;
extern const int NUM_BUILTINS;

// built in functions
int lsh_cd(char **);
int lsh_help(char **);
int lsh_exit(char **);
int lsh_pwd(char **);
int lsh_clear(char **);