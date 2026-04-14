#pragma once

#include <stddef.h>

#define private static

typedef enum
{
    CONTEXT_COMMAND,
    CONTEXT_CD_ARG,
    CONTEXT_LS_ARG,
    CONTECT_OTHER_ARG,
} CompletionContext;

CompletionContext completion_context_detect(const char *, size_t);
char *completion_context_getPrefix(const char *, size_t);