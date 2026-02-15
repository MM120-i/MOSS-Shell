#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#include "include/logging.h"

private LogLevel currentLevel = LOG_LEVEL_INFO;
private FILE *logOutput = NULL;
private bool logToFile = false;
private int errorCounter = 0;
private time_t errorWindowStart = 0;

private const char *categoryString[] = {
    "INPUT",
    "EXEC",
    "MEMORY",
    "SYSTEM",
    "PARSE",
    "PATH",
};

private const char *levelString[] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
};

void mossLogSetLevel(LogLevel level)
{
    currentLevel = level;
}

void mossLogSetFile(FILE *fp)
{
    logOutput = fp;
    logToFile = (fp != NULL);
}

void mossLog(ErrorCategory category, LogLevel level, const char *fmt, ...)
{
    if (level < currentLevel)
        return;

    time_t now = time(NULL);
    const struct tm *tmInfo = localtime(&now);
    char timeBuf[BUF_SIZE];
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", tmInfo);
    FILE *output = logToFile ? logOutput : stderr;
    fprintf(output, "[%s] [%s] [%s] ", timeBuf, levelString[level], categoryString[category]);

    va_list args;
    va_start(args, fmt);
    vfprintf(output, fmt, args);
    va_end(args);
    fprintf(output, "\n");
    fflush(output);
}

private bool checkRateLimit(void)
{
    time_t now = time(NULL);

    if (difftime(now, errorWindowStart) > ERROR_RATE_WINDOW)
    {
        errorCounter = 0;
        errorWindowStart = now;
    }

    return errorCounter < ERROR_RATE_LIMIT;
}

void mossSafeError(ErrorCategory category, const char *fmt, ...)
{
    if (!checkRateLimit())
        return;

    errorCounter++;

    time_t now = time(NULL);
    const struct tm *tmInfo = localtime(&now);
    char timeBuf[BUF_SIZE];
    strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", tmInfo);

    fprintf(stderr, "[%s] [ERROR] [%s] MOSS: ", timeBuf, categoryString[category]);
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");

    if (logToFile && logOutput)
    {
        fprintf(logOutput, "[%s] [ERROR] [%s] MOSS: ", timeBuf, categoryString[category]);
        va_list args2;
        va_start(args2, fmt);
        vfprintf(logOutput, fmt, args2);
        va_end(args2);
        fprintf(logOutput, "\n");
        fflush(logOutput);
    }
}