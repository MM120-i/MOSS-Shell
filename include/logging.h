#pragma once

#include <stdio.h>
#include <time.h>
#include <stdbool.h>

#define private static
#define BUF_SIZE 32

typedef enum
{
    LOG_LEVEL_DEBUG = 0,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel;

typedef enum
{
    ERR_CATEGORY_INPUT,
    ERR_CATEGORY_EXEC,
    ERR_CATEGORY_MEMORY,
    ERR_CATEGORY_SYSTEM,
    ERR_CATEGORY_PARSE,
    ERR_CATEGORY_PATH
} ErrorCategory;

typedef struct
{
    LogLevel level;
    ErrorCategory category;
    const char *message;
    time_t timestamp;
} LogEntry;

void mossLogSetLevel(LogLevel);
void mossLogSetFile(FILE *);
void mossLog(ErrorCategory, LogLevel, const char *, ...);

#define LOG_CATEGORY ERR_CATEGORY_SYSTEM

#define LOG_DEBUG(fmt, ...) mossLog(LOG_CATEGORY, LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) mossLog(LOG_CATEGORY, LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) mossLog(LOG_CATEGORY, LOG_LEVEL_WARN, fmt, ##VA_ARGS__)
#define LOG_ERROR(fmt, ...) mossLog(LOG_CATEGORY, LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
