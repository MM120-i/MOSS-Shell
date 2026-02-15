#pragma once

#include <stdbool.h>

#define TOK_INITIAL_BUFFER 64
#define TOK_MAX_TOKEN_LEN 4096
#define TOK_DELIMETER " \t\r\n\a"
#define DANGEROUS_CHARS ";|&$`\\\"'<>{}[]()!*?#~"
#define ERROR_RATE_LIMIT 5
#define ERROR_RATE_WINDOW 60

#define private static

typedef struct
{
    char **args;
    pid_t pid;
} LaunchContext;

void moss_loop();
char *moss_read_line();
char **moss_split_line(char *);
int moss_execute(char **);
int moss_launch(char **);
