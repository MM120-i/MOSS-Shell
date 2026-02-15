#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <stdbool.h>

#include "include/logging.h"

private LogLevel currentLevel = LOG_LEVEL_INFO;
private FILE *logOutput = NULL;
private bool logToFile = false;

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
    fprintf(output, "[%s] [%s] [%s]", timeBuf, levelString[level], categoryString[category]);

    va_list args;
    va_start(args, fmt);
    vfprintf(output, fmt, args);
    va_end(args);
    fprintf(output, "\n");
    fflush(output);
}