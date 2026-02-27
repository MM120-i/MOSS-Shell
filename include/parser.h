#pragma once

#define private static

#include <stdbool.h>

char **moss_split_line(char *);
bool isSafe(const char *);
char *strip_quotes(char *);
char *expandEnvVar(const char *);
char *strip_comment(char *);