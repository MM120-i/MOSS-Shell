#pragma once

#include <stdbool.h>

#define SIZE 4096
#define private static

#define ERROR_RATE_LIMIT 5
#define ERROR_RATE_WINDOW 60

struct builtin
{
    char *name;
    int (*func)(char **);
};

extern const struct builtin builtins[];
extern int builtins_count;
extern const int NUM_BUILTINS;

private
bool checkRateLimit();

private
void safeError(const char *);

// built in functions
int moss_cd(char **);
int moss_help(char **);
int moss_exit(char **);
int moss_pwd(char **);
int moss_clear(char **);